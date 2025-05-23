#include <cstdint>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <stdexcept>

#include <pipewire/pipewire.h>
#include <pipewire/loop.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>

#include <spa/buffer/buffer.h>
#include <spa/buffer/meta.h>
#include <spa/param/video/raw.h>
#include <spa/param/latency-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/pod.h>
#include <spa/utils/defs.h>
#include <spa/utils/type.h>
#include <spa/debug/format.h>
#include <spa/debug/pod.h>

#include <pixman.h>

#include <core/LogWriter.h>

#include "PipewirePixelBuffer.h"

#define MAX_BUFFERS 64

static core::LogWriter vlog("Pipewire");

static void onStreamStateChanged(void *_data, enum pw_stream_state old,
                                 enum pw_stream_state state,
                                 const char *error);
static void onStreamIoChanged(void *_data, uint32_t id, void *area,
                              uint32_t size);
static void onStreamParamChanged(void *_data, uint32_t id,
                                 const struct spa_pod *param);
static void onProcess(void *_data);

static int sourceLoopDispatch(GSource* source, GSourceFunc callback,
                              void* userData)
{
	PipeWireSource *s = (PipeWireSource *) source;
	int result;
  (void)callback;
  (void)userData;

	result = pw_loop_iterate (s->loop, 0);
	if (result < 0)
		vlog.error("pipewire_loop_iterate failed: %s", g_strerror (result));

	return true;
}

static void globalRegistryEvent(void *data, uint32_t id,
                                uint32_t permissions, const char *type,
                                uint32_t version,
                                const struct spa_dict *props)
{
  (void)data;
  (void)permissions;
  (void)props;
  printf("object: id:%u type:%s/%d\n", id, type, version);
}

static const rfb::PixelFormat pfBGRX(32, 24, false, true, 255, 255, 255,
                                     16, 8, 0);

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server)
  : pipewireFd_(pipewireFd), pipewireId_(pipewireId)
{
  pw_init(nullptr, nullptr);
  static GSourceFuncs pipewire_source_funcs;
  static const pw_registry_events registry_events{PW_VERSION_REGISTRY_EVENTS,
                                                  &globalRegistryEvent,
                                                  nullptr};

  pipewire_source_funcs.dispatch = sourceLoopDispatch;

  loop = pw_loop_new(nullptr);
  source = (PipeWireSource *)g_source_new(&pipewire_source_funcs,
                                          sizeof(PipeWireSource));
  source->loop = loop;
  g_source_add_unix_fd (&source->base,
                        pw_loop_get_fd (loop),
                        (GIOCondition)(G_IO_IN | G_IO_ERR));
  g_source_attach (&source->base, NULL);
  g_source_unref (&source->base);

  props_ = pw_properties_new(PW_KEY_REMOTE_NAME, "pipewire-0",
    PW_KEY_MEDIA_TYPE, "Video",
    PW_KEY_MEDIA_CATEGORY, "Capture",
    PW_KEY_MEDIA_ROLE, "Screen",
    nullptr);

  context_ = pw_context_new(loop, nullptr, 0);
  core_ = pw_context_connect_fd(context_, pipewireFd, props_, 0);
  registry = pw_core_get_registry(core_, PW_VERSION_REGISTRY, 0);

  spa_zero(registryListener);
  pw_registry_add_listener(registry, &registryListener,
    &registry_events, nullptr);

  source->data = new PipeWireData;
  source->data->buffer = nullptr;
  source->server = server;
  source->instance = this;

  pw_loop_enter(loop);
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
  // FIXME: There's some cleanup missing here
  delete source->data;
  pw_loop_leave(loop);
  pw_loop_destroy(loop);
  pw_deinit();
}

void PipeWireSource::setBuffer(int width, int height, uint8_t *buffer,
                               int stride)
{
  this->instance->setBuffer(width, height, buffer, stride);
}

void PipeWireSource::setFormat(rfb::PixelFormat format)
{
  this->instance->format = format;
}

void onStreamStateChanged(void *_data, enum pw_stream_state old,
                          enum pw_stream_state state, const char *error)
{
  (void)old;
  PipeWireSource* source;

  vlog.debug("onStreamStateChanged");

  source = (PipeWireSource*)_data;
  assert(source);

  switch (state) {
  case PW_STREAM_STATE_UNCONNECTED:
    pw_loop_destroy(source->loop);
    break;
  case PW_STREAM_STATE_PAUSED:
    pw_stream_set_active(source->stream, true);
    break;
  case PW_STREAM_STATE_ERROR:
    vlog.error("Stream error: %s", error);
    pw_loop_destroy(source->loop);
    break;
  default:
    vlog.debug("Unhandled stream state: %d", state);
    break;
  }
}

void onStreamIoChanged(void *_data, uint32_t id, void *area,
                       uint32_t size)
{
  (void)size;
  PipeWireSource* source;

  vlog.debug("onStreamIoChanged");

  source = (PipeWireSource*)_data;
  assert(source);

  switch (id) {
  case SPA_IO_Position:
    source->data->position = (spa_io_position*)area;
    break;
  case SPA_IO_Clock:
    source->data->clock = (spa_io_clock*)area;
    break;
  }
}

void onStreamParamChanged(void *_data, uint32_t id,
                          const struct spa_pod *param)
{
  PipeWireSource* source;
  pw_stream *stream;
  uint8_t paramsBuffer[1024];
  spa_pod_builder b;
  int nParams;
  const spa_pod *params[5];
  int32_t mult, size;

  vlog.debug("onStreamParamChanged");

  if (param == nullptr || id != SPA_PARAM_Format) {
    vlog.debug("clear format");
    return;
  }
  if (param != nullptr && id == SPA_PARAM_Tag) {
    spa_debug_pod(0, nullptr, param);
    return;
  }
  if (param != nullptr && id == SPA_PARAM_Latency) {
    struct spa_latency_info info;
    if (spa_latency_parse(param, &info) >= 0)
      vlog.error("got latency: %" PRIu64 "\n",
                 (info.min_ns + info.max_ns) / 2);
    return;
  }

  source = (PipeWireSource *)_data;
  stream = source->stream;

  if (spa_format_parse(param, &source->data->format.media_type,
                       &source->data->format.media_subtype) < 0)
    return;

  if (source->data->format.media_type != SPA_MEDIA_TYPE_video)
    return;

  spa_pod_builder_init(&b, paramsBuffer, sizeof(paramsBuffer));
  spa_debug_format(2, NULL, param);

  switch (source->data->format.media_subtype) {
  case SPA_MEDIA_SUBTYPE_raw:
    spa_format_video_raw_parse(param, &source->data->format.info.raw);
    source->data->size = SPA_RECTANGLE(source->data->format.info.raw.size.width,
    source->data->format.info.raw.size.height);

    if (source->data->format.info.raw.format == SPA_VIDEO_FORMAT_BGRx) {
      source->data->stride = source->data->format.info.raw.size.width * 4;
      source->setFormat(pfBGRX);
      mult = 4;
    } else {
      throw std::runtime_error("unsupported pixel format :" +
        std::to_string(source->data->format.info.raw.format));
    }
    break;
  case SPA_VIDEO_FORMAT_UNKNOWN:
    pw_stream_set_error(stream, -EINVAL, "unknown pixel format");
    return;
  default:
    pw_stream_set_error(stream, -EINVAL, "unsupported pixel format");
    return;
  }

  if (!source->data->size.width || !source->data->size.height) {
    pw_stream_set_error(stream, -EINVAL, "invalid size");
    return;
  }

  size = source->data->size.width * source->data->size.height * mult;
  source->data->rect.x = 0;
  source->data->rect.y = 0;
  source->data->rect.w = source->data->size.width;
  source->data->rect.h = source->data->size.height;

  // FIXME: Handle resize
  if (!source->data->buffer) {
    source->data->buffer = (uint8_t*)malloc(size);
    source->setBuffer(source->data->size.width,
                      source->data->size.height, source->data->buffer,
                      source->data->stride / mult);
  }

  nParams = 0;
  /* a SPA_TYPE_OBJECT_ParamBuffers object defines the acceptable size,
   * number, stride etc of the buffers */
  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&b,
    SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
    SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(8, 2, MAX_BUFFERS),
    SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
    SPA_PARAM_BUFFERS_size, SPA_POD_Int(size * mult),
    SPA_PARAM_BUFFERS_stride, SPA_POD_Int(source->data->stride * mult),
    SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr)));

  /* a header metadata with timing information */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(&b,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
    SPA_PARAM_META_size, SPA_POD_Int(sizeof(struct spa_meta_header)));

  /* video cropping information */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_VideoCrop), SPA_PARAM_META_size,
    SPA_POD_Int(sizeof(struct spa_meta_region)));

  /* Damage information */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_VideoDamage), SPA_PARAM_META_size,
    SPA_POD_CHOICE_RANGE_Int(sizeof(struct spa_meta_region) * 16,
                             sizeof(struct spa_meta_region) * 1,
                             sizeof(struct spa_meta_region) * 16));

  pw_stream_update_params(stream, params, nParams);
}

void onProcess(void *data) {
  PipeWireSource *source;
  pw_buffer* buffer;
  spa_buffer* buf;
  spa_meta_region* damage;
  size_t bufSize;
  uint32_t width, height;

  pixman_bool_t ret;
  uint32_t* srcBits;
  uint32_t* dstBits;
  int srcStride;
  int dstStride;
  int srcBpp;
  int dstBpp;

  source = (PipeWireSource*)data;
  assert(source);

  buffer = nullptr;
  while (true) {
    pw_buffer *next_buf;

    next_buf = pw_stream_dequeue_buffer(source->stream);

    if (!next_buf)
      break;
    if (buffer)
      pw_stream_queue_buffer(source->stream, buffer);

    buffer = next_buf;
  }

  if (!buffer) {
    vlog.debug("Out of buffers");
    return;
  }

  assert(buffer->buffer->datas[0].chunk->size > 0);

  bufSize = buffer->buffer->datas[0].chunk->size;
  if (bufSize == 0) {
    pw_stream_queue_buffer(source->stream, buffer);
    vlog.debug("Skipping empty buffer");
    return;
  }

  buf = buffer->buffer;

  if ((damage = (spa_meta_region *)spa_buffer_find_meta_data(
      buf, SPA_META_VideoDamage, sizeof(*damage))) &&
      spa_meta_region_is_valid(damage)) {
        assert(damage->region.size.width != 0);
        assert(damage->region.size.height != 0);
  }

  srcBits = (uint32_t*)buf[0].datas[0].data;
  dstBits = (uint32_t*)source->data->buffer;
  srcStride = buf->datas->chunk->stride / 4;
  dstStride = source->data->stride / 4;
  srcBpp = 32;
  dstBpp = 32;

  width = source->data->size.width;
  height = source->data->size.height;

  // Accelerated copy
  // FIXME: Only copy damaged region
  ret = pixman_blt(srcBits, dstBits, srcStride, dstStride,
                  srcBpp, dstBpp, 0, 0, 0, 0, width, height);
  if (!ret)
    memcpy(source->data->buffer, buf[0].datas[0].data, bufSize);

  if (damage) {
    core::Rect r{damage->region.position.x, damage->region.position.y,
                (int)damage->region.size.width,
                (int)damage->region.size.height};
    source->server->add_changed(core::Region(r));
  } else {
    core::Rect r{0, 0, (int)width, (int)height};
    source->server->add_changed(core::Region(r));
  }

  pw_stream_queue_buffer(source->stream, buffer);
}

void PipeWirePixelBuffer::start()
{
  uint8_t buffer[4096];
  struct spa_pod_builder b;
  const struct spa_pod *params[1];
  static pw_stream_events streamEvents;
  pw_stream* stream;

  spa_pod_builder_init(&b, buffer, sizeof(buffer));

  stream = pw_stream_new(core_, "w0vncserver", props_);
  if (!stream)
    throw std::runtime_error("Failed to create PipeWire stream");

  source->stream = stream;

  streamEvents.state_changed = onStreamStateChanged;
  streamEvents.io_changed = onStreamIoChanged;
  streamEvents.param_changed = onStreamParamChanged;
  streamEvents.process = onProcess;

  pw_stream_add_listener(stream, &streamListener,
                         &streamEvents, source);

  // FIXME: This is a bit ugly
  struct spa_rectangle rDef{1280,720};
  struct spa_rectangle rMin{1,1};
  struct spa_rectangle rMax{4096,4096};
  struct spa_fraction fDef{25,1};
  struct spa_fraction fMin{0,1};
  struct spa_fraction fMax{60,1};

  params[0] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
    SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
    SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
    SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(2,
                                                    SPA_VIDEO_FORMAT_RGBx,
                                                    SPA_VIDEO_FORMAT_BGRx),
    SPA_FORMAT_VIDEO_size,
    SPA_POD_CHOICE_RANGE_Rectangle(&rDef, &rMin, &rMax),
    SPA_FORMAT_VIDEO_framerate,
    SPA_POD_CHOICE_RANGE_Fraction(&fDef, &fMin,&fMax));

  if (pw_stream_connect(stream, PW_DIRECTION_INPUT, pipewireId_,
                        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS),
                        params, 1) < 0) {
    throw std::runtime_error("Failed to connect PipeWire stream");
  }
}
