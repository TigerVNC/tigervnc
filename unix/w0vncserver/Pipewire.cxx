#include "Pipewire.h"
#include "pipewire/loop.h"
#include "pipewire/properties.h"
#include "pipewire/stream.h"
#include "spa/param/video/raw.h"
#include "spa/pod/builder.h"
#include "spa/pod/pod.h"
#include "spa/utils/type.h"
#include <spa/param/latency-utils.h>
#include <spa/debug/format.h>
#include <spa/debug/pod.h>

#include <pipewire/pipewire.h>
#include <stdexcept>
#include <assert.h>
#include <core/LogWriter.h>

#include "WDesktop.h"

#include <pixman.h>

#define MAX_BUFFERS 64

static core::LogWriter vlog("Pipewire");

static gboolean
pipewire_loop_source_dispatch (GSource     *source,
                               GSourceFunc  callback,
                               gpointer     user_data)
{
	PipeWireSource *s = (PipeWireSource *) source;
	int result;
  (void)callback;
  (void)user_data;

	result = pw_loop_iterate (s->loop, 0);
	if (result < 0)
		vlog.error("pipewire_loop_iterate failed: %s", g_strerror (result));

	return true;
}

static void registry_event_global(void *data, uint32_t id,
  uint32_t permissions, const char *type, uint32_t version,
  const struct spa_dict *props)
{
  (void)data;
  (void)permissions;
  (void)props;
  printf("object: id:%u type:%s/%d\n", id, type, version);
}


Pipewire::Pipewire(int32_t pipewire_fd, uint32_t pipewire_id,
                   WDesktop* desktop)
  : pipewire_fd_(pipewire_fd), pipewire_id_(pipewire_id)
{
  pw_init(nullptr, nullptr);
  static GSourceFuncs pipewire_source_funcs;
  static const pw_registry_events registry_events{PW_VERSION_REGISTRY_EVENTS,
                                                  &registry_event_global,
                                                  nullptr};

  pipewire_source_funcs.dispatch = pipewire_loop_source_dispatch;

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

  core_ = pw_context_connect_fd(context_, pipewire_fd_, props_, 0);

  registry = pw_core_get_registry(core_, PW_VERSION_REGISTRY, 0);

  spa_zero(registry_listener);

  pw_registry_add_listener(registry, &registry_listener,
    &registry_events, nullptr);

  source->data = new pw_data;
  source->data->buffer = nullptr;
  source->desktop_ = desktop;

  pw_loop_enter(loop);
}

Pipewire::~Pipewire()
{
  // FIXME: There's some cleanup missing here
  delete source->data;
  pw_loop_leave(loop);
  pw_loop_destroy(loop);
  pw_deinit();
}

void Pipewire::on_stream_state_changed(void *_data,
                                       enum pw_stream_state old,
                                       enum pw_stream_state state,
                                       const char *error)
{
  (void)old;
  PipeWireSource* source;

  vlog.debug("on_stream_state_changed");

  source = (PipeWireSource*)_data;
  assert(source);

  switch (state) {
    case PW_STREAM_STATE_UNCONNECTED:
      pw_loop_destroy(source->loop);
      break;
    case PW_STREAM_STATE_PAUSED:
      /* because we started inactive, activate ourselves now */
      pw_stream_set_active(source->stream, true);
      break;
    case PW_STREAM_STATE_ERROR:
      vlog.error("Stream error: %s", error);
      pw_loop_destroy(source->loop);
      break;
    default:
      break;
    }
}

void Pipewire::on_stream_io_changed(void *_data, uint32_t id,
                                    void *area, uint32_t size)
{
  (void)id;
  (void)area;
  (void)size;
  PipeWireSource* source;

  vlog.debug("on_stream_io_changed");

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

void Pipewire::on_stream_param_changed(void *_data, uint32_t id,
                                      const struct spa_pod *param)
{

  PipeWireSource* source;
  pw_stream *stream;
  uint8_t params_buffer[1024];
  spa_pod_builder b;
  int n_params;
  const spa_pod *params[5];
  int32_t mult, size;

  vlog.debug("on_stream_param_changed");

  // FIXME: Handle resizes

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

  spa_pod_builder_init(&b, params_buffer, sizeof(params_buffer));
  spa_debug_format(2, NULL, param);

  switch (source->data->format.media_subtype) {
  case SPA_MEDIA_SUBTYPE_raw:
    spa_format_video_raw_parse(param, &source->data->format.info.raw);
    mult = 1;
    source->data->size = SPA_RECTANGLE(source->data->format.info.raw.size.width,
                               source->data->format.info.raw.size.height);
    source->data->stride = source->data->format.info.raw.size.width * 4;
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

  if (!source->data->buffer)
    source->data->buffer = (uint8_t*)malloc(size * 4);
  else {
    // FIXME: Handle resizes
  }

  n_params = 0;
  /* a SPA_TYPE_OBJECT_ParamBuffers object defines the acceptable size,
   * number, stride etc of the buffers */
  params[n_params++] = (spa_pod*)spa_pod_builder_add_object(
  &b, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
  SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(8, 2, MAX_BUFFERS),
  SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1), SPA_PARAM_BUFFERS_size,
  SPA_POD_Int(size * mult), SPA_PARAM_BUFFERS_stride,
  SPA_POD_Int(source->data->stride * mult), SPA_PARAM_BUFFERS_dataType,
  SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr)));

  /* a header metadata with timing information */
  params[n_params++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_Header), SPA_PARAM_META_size,
    SPA_POD_Int(sizeof(struct spa_meta_header)));

  /* video cropping information */
  params[n_params++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_VideoCrop), SPA_PARAM_META_size,
    SPA_POD_Int(sizeof(struct spa_meta_region)));

#define CURSOR_META_SIZE(w, h)                                         \
  (sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) +   \
   w * h * 4)

  /* cursor information */
  params[n_params++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_Cursor), SPA_PARAM_META_size,
    SPA_POD_CHOICE_RANGE_Int(CURSOR_META_SIZE(64, 64),
                             CURSOR_META_SIZE(1, 1),
                             CURSOR_META_SIZE(256, 256)));

  /* Damage information */
  params[n_params++] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_VideoDamage), SPA_PARAM_META_size,
    SPA_POD_CHOICE_RANGE_Int(sizeof(struct spa_meta_region) * 16,
                             sizeof(struct spa_meta_region) * 1,
                             sizeof(struct spa_meta_region) * 16));

  pw_stream_update_params(stream, params, n_params);
}

void Pipewire::on_process(void *data) {
  PipeWireSource *source;
  pw_buffer *pw_buf;
  spa_buffer* buf;
  spa_meta_region* damage;
  size_t buf_size;
  uint32_t width, height;

  pixman_bool_t ret;
  uint32_t* src_bits;
  uint32_t* dst_bits;
  int src_stride;
  int dst_stride;
  int src_bpp;
  int dst_bpp;

  source = (PipeWireSource*)data;
  assert(source);

  pw_buf = nullptr;
  // Get the latest buffer
  while (true) {
    pw_buffer *next_buf;

    next_buf = pw_stream_dequeue_buffer(source->stream);

    if (!next_buf)
      break;
    if (pw_buf)
      pw_stream_queue_buffer(source->stream, pw_buf);

    pw_buf = next_buf;
  }

  if (!pw_buf) {
    vlog.error("out of buffers");
    return;
  }

  buf = pw_buf->buffer;
  buf_size = buf->datas->chunk->size;
  //  Damage
  if ((damage = (spa_meta_region *)spa_buffer_find_meta_data(
      buf, SPA_META_VideoDamage, sizeof(*damage))) &&
      spa_meta_region_is_valid(damage)) {
        assert(damage->region.size.width != 0);
        assert(damage->region.size.height != 0);
  }

  src_bits = (uint32_t*)buf[0].datas[0].data;
  dst_bits = (uint32_t*)source->data->buffer;
  src_stride = buf->datas->chunk->stride / 4;
  dst_stride = source->data->stride / 4;
  src_bpp = 32;
  dst_bpp = 32;

  width = source->data->size.width;
  height = source->data->size.height;

  // Accelerated copy
  ret = pixman_blt(src_bits, dst_bits, src_stride, dst_stride,
                   src_bpp, dst_bpp, 0, 0, 0, 0, width, height);
  if (!ret) {
    memcpy(source->data->buffer, buf[0].datas[0].data, buf_size);
  }

  if (!source->desktop_->pixelBuffer()) {
    vlog.debug("Creating pixel buffer");
    WPixelBuffer* pb = new WPixelBuffer(width, height,
                                        source->data->buffer,
                                        dst_stride);
    source->desktop_->setPixelBuffer(pb);
  }

  if (damage) {
    core::Rect r{damage->region.position.x, damage->region.position.y,
                (int)damage->region.size.width,
                (int)damage->region.size.height};
    source->desktop_->add_changed(core::Region(r));
  } else {
    core::Rect r{0, 0, (int)width, (int)height};
    source->desktop_->add_changed(core::Region(r));
  }

  pw_stream_queue_buffer(source->stream, pw_buf);
}

void Pipewire::start()
{
  uint8_t buffer[4096];
  struct spa_pod_builder b;
  const struct spa_pod *params[1];
  static pw_stream_events stream_events;
  pw_stream* stream;

  spa_pod_builder_init(&b, buffer, sizeof(buffer));

  stream = pw_stream_new(core_, "w0vncserver", props_);
  if (!stream)
    throw std::runtime_error("Failed to create PipeWire stream");

  source->stream = stream;

  stream_events.state_changed = on_stream_state_changed;
  stream_events.io_changed = on_stream_io_changed;
  stream_events.param_changed = on_stream_param_changed;
  stream_events.process = on_process;

  pw_stream_add_listener(stream, &stream_listener_,
                         &stream_events, source);

  // FIXME: This is a bit ugly
  struct spa_rectangle r_def{1280,720};
  struct spa_rectangle r_min{1,1};
  struct spa_rectangle r_max{4096,4096};
  struct spa_fraction f_def{25,1};
  struct spa_fraction f_min{0,1};
  struct spa_fraction f_max{60,1};

  params[0] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
    SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
    SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
    SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(2,
                                                    SPA_VIDEO_FORMAT_RGBx,
                                                    SPA_VIDEO_FORMAT_BGRx),
    SPA_FORMAT_VIDEO_size,
    SPA_POD_CHOICE_RANGE_Rectangle(&r_def, &r_min, &r_max),
    SPA_FORMAT_VIDEO_framerate,
    SPA_POD_CHOICE_RANGE_Fraction(&f_def, &f_min,&f_max));

  if (pw_stream_connect(stream, PW_DIRECTION_INPUT, pipewire_id_,
                        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS),
                        params, 1) < 0) {
    throw std::runtime_error("Failed to connect PipeWire stream");
  }
}
