#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdint.h>

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
#include <rfb/VNCServer.h>

#include "PipewirePixelBuffer.h"

#define MAX_BUFFERS 64

static core::LogWriter vlog("Pipewire");

static void onStreamStateChanged(void *_data, enum pw_stream_state old,
                                 enum pw_stream_state state,
                                 const char *error);
static void onStreamParamChanged(void *_data, uint32_t id,
                                 const struct spa_pod *param);
static void onProcess(void *_data);

struct PipeWireData {
  // FIXME: Create a destructor for PipeWireData
  struct Rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
  };

  struct Cursor {
    uint32_t w;
    uint32_t h;
    int32_t x;
    int32_t y;
    int32_t hotspotX;
    int32_t hotspotY;
    int32_t stride;
    uint8_t *data;
  };

  spa_video_info format;
  int32_t stride;
  spa_rectangle size;
  Rect rect;

  uint8_t *buffer;
  Cursor cursor;
};

struct PipeWireSource {
  GSource base;
  pw_loop* loop;
  pw_stream* stream;
  PipeWireData* data;
  rfb::VNCServer* server;
  PipeWirePixelBuffer* instance;

  void setBuffer(int width, int height, uint8_t* data, int stride);
  void setFormat(rfb::PixelFormat format);
};

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

static GSourceFuncs pipewireSourceFuncs {
  .prepare = nullptr,
  .check = nullptr,
  .dispatch = sourceLoopDispatch,
  .finalize =nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

// FIXME: Some of these callbacks are not present in all version
// of pipewire.
static pw_stream_events streamEvents {
  .version = PW_VERSION_STREAM_EVENTS,
  .destroy = nullptr,
  .state_changed = onStreamStateChanged,
  .control_info = nullptr,
  .io_changed = nullptr,
  .param_changed = onStreamParamChanged,
  .add_buffer = nullptr,
  .remove_buffer = nullptr,
  .process = onProcess,
  .drained = nullptr,
  .command = nullptr,
  .trigger_done = nullptr
};

void setCursor(int width, int height, int hotX, int hotY,
               const unsigned char *rgbaData, rfb::VNCServer* server)
{
  // Copied from XserverDesktop.cc
  uint8_t* cursorData;

  uint8_t *out;
  const unsigned char *in;

  cursorData = new uint8_t[width * height * 4];

  // Un-premultiply alpha
  in = rgbaData;
  out = cursorData;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t alpha;

      alpha = in[3];
      if (alpha == 0)
        alpha = 1; // Avoid division by zero

      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = *in++;
    }
  }

  try {
    server->setCursor(width, height, {hotX, hotY}, cursorData);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.what());
  }

  delete [] cursorData;
}

static void processCursor(PipeWireSource *source, pw_buffer* buf)
{
  spa_meta_cursor* mcs;
  spa_meta_bitmap* mb;
  uint8_t* src;

  mcs = (spa_meta_cursor *)spa_buffer_find_meta_data(buf->buffer,
                                                     SPA_META_Cursor,
                                                     sizeof(*mcs));

  assert(mcs);

  source->data->cursor.x = mcs->position.x;
  source->data->cursor.y = mcs->position.y;
  source->data->cursor.hotspotX = mcs->hotspot.x;
  source->data->cursor.hotspotY = mcs->hotspot.y;

  source->server->setCursorPos({source->data->cursor.x,
                                source->data->cursor.y}, false);

  // No new cursor bitmap
  if (!mcs->bitmap_offset)
    return;

  mb = SPA_PTROFF(mcs, mcs->bitmap_offset, struct spa_meta_bitmap);

  // If cursor size has changed
  if (mb->size.width != source->data->cursor.w ||
      mb->size.height != source->data->cursor.h ||
      mb->stride != source->data->cursor.stride) {
    uint32_t cursor_size;

    delete [] source->data->cursor.data;
    source->data->cursor.data = nullptr;

    cursor_size = mb->stride * mb->size.height;
    source->data->cursor.data = new uint8_t[cursor_size];
  }

  src = SPA_PTROFF(mb, mb->offset, uint8_t);
  memcpy(source->data->cursor.data, src, mb->stride * mb->size.height);

  source->data->cursor.w = mb->size.width;
  source->data->cursor.h = mb->size.height;
  source->data->cursor.stride = mb->stride;

  setCursor(source->data->cursor.w,
            source->data->cursor.h,
            source->data->cursor.hotspotX,
            source->data->cursor.hotspotY,
            source->data->cursor.data, source->server);
}

static bool hasCursorData(pw_buffer* buf)
{
  spa_meta_cursor* mcs;

  if ((mcs = (spa_meta_cursor *)spa_buffer_find_meta_data(
    buf->buffer, SPA_META_Cursor, sizeof(*mcs))) &&
    spa_meta_cursor_is_valid(mcs)) {
      // We got new cursor position / new cursor bitmap
      return true;
    }

  return false;
}


static const rfb::PixelFormat pfBGRX(32, 24, false, true, 255, 255, 255,
                                     16, 8, 0);

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server)
  : pipewireFd_(pipewireFd), pipewireId_(pipewireId)
{
  pw_init(nullptr, nullptr);

  loop = pw_loop_new(nullptr);
  source = (PipeWireSource *)g_source_new(&pipewireSourceFuncs,
                                          sizeof(PipeWireSource));
  source->loop = loop;
  g_source_add_unix_fd (&source->base,
                        pw_loop_get_fd (loop),
                        (GIOCondition)(G_IO_IN | G_IO_ERR));
  g_source_attach (&source->base, NULL);
  g_source_unref (&source->base);

  props = pw_properties_new(PW_KEY_REMOTE_NAME, "pipewire-0",
    PW_KEY_MEDIA_TYPE, "Video",
    PW_KEY_MEDIA_CATEGORY, "Capture",
    PW_KEY_MEDIA_ROLE, "Screen",
    nullptr);

  context = pw_context_new(loop, nullptr, 0);
  core = pw_context_connect_fd(context, pipewireFd, props, 0);
  registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);

  source->data = new PipeWireData;
  source->data->buffer = nullptr;
  source->data->cursor.data = nullptr;
  source->server = server;
  source->instance = this;

  pw_loop_enter(loop);
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
  // FIXME: There's some cleanup missing here
  delete source->data->cursor.data;
  delete source->data;
  pw_loop_leave(loop);
  pw_loop_destroy(loop);
  pw_deinit();
}

void PipeWireSource::setBuffer(int width, int height, uint8_t *buffer,
                               int stride)
{
  instance->setBuffer(width, height, buffer, stride);
}

void PipeWireSource::setFormat(rfb::PixelFormat format)
{
  instance->format = format;
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
      mult = 4;
      source->data->stride = source->data->format.info.raw.size.width * mult;
      source->setFormat(pfBGRX);
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
    SPA_PARAM_BUFFERS_size, SPA_POD_Int(size),
    SPA_PARAM_BUFFERS_stride, SPA_POD_Int(source->data->stride),
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

  #define CURSOR_META_SIZE(w, h)                                         \
  (sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) +   \
   w * h * 4)

  /* cursor information */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(&b,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
    SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(
                             CURSOR_META_SIZE(64, 64),
                             CURSOR_META_SIZE(1, 1),
                             CURSOR_META_SIZE(512, 512)));

  pw_stream_update_params(stream, params, nParams);
}

void onProcess(void *data) {
  PipeWireSource *source;
  pw_buffer* lastFrameBuf;
  pw_buffer* lastCursorBuf;
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

  lastFrameBuf = nullptr;
  lastCursorBuf = nullptr;
  // Pipewire can handle sending cursor metadata in the same stream as
  // the framebuffer data. With cursor metadata enabled,
  // we need to check whether:
  // 1. The buffer has frame data & cursor data
  // 2. The buffer has frame data & no cursor data
  // 3. The buffer has no frame data & cursor data
  // 4. The buffer has no frame data & no cursor data (corrupted)
  // We only care about the most up-to-date buffer containing frame or
  // cursor data
  while (true) {
    pw_buffer *nextBuf;
    spa_meta_header *header;

    nextBuf = pw_stream_dequeue_buffer(source->stream);

    if (!nextBuf)
      break;

    header = (spa_meta_header *)spa_buffer_find_meta_data(nextBuf->buffer,
                                                          SPA_META_Header,
                                                          sizeof(*header));
    // Corrupted buffers contain no frame or cursor data
    if (header && header->flags & SPA_META_HEADER_FLAG_CORRUPTED) {
      pw_stream_queue_buffer(source->stream, nextBuf);
      continue;
    }

    if (hasCursorData(nextBuf)) {
      if (lastCursorBuf == lastFrameBuf)
        lastCursorBuf = nullptr;

      if (lastCursorBuf)
        pw_stream_queue_buffer(source->stream, lastCursorBuf);
      lastCursorBuf = nextBuf;
    }

    // Frame data available
    if (!(nextBuf->buffer->datas[0].chunk->flags &
        SPA_CHUNK_FLAG_CORRUPTED)) {
      if (lastCursorBuf == lastFrameBuf)
        lastFrameBuf = nullptr;

      if (lastFrameBuf)
        pw_stream_queue_buffer(source->stream, lastFrameBuf);
      lastFrameBuf = nextBuf;
    }

    if (nextBuf != lastCursorBuf && nextBuf != lastFrameBuf)
          pw_stream_queue_buffer(source->stream, nextBuf);
  }

  // No new data available
  if (!lastCursorBuf && !lastFrameBuf) {
    return;
  }

  if (lastCursorBuf) {
    processCursor(source, lastCursorBuf);
    if (lastCursorBuf != lastFrameBuf) {
      pw_stream_queue_buffer(source->stream, lastCursorBuf);
    }
  }

  if (!lastFrameBuf)
    return;

  assert(lastFrameBuf->buffer->datas[0].chunk->size > 0);

  bufSize = lastFrameBuf->buffer->datas[0].chunk->size;
  if (bufSize == 0) {
    pw_stream_queue_buffer(source->stream, lastFrameBuf);
    vlog.debug("Skipping empty buffer");
    return;
  }

  buf = lastFrameBuf->buffer;

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
    source->server->add_changed({{damage->region.position.x,
                                  damage->region.position.y,
                                  (int)damage->region.size.width,
                                  (int)damage->region.size.height}});
  } else {
    source->server->add_changed({{0, 0, (int)width, (int)height}});
  }

  pw_stream_queue_buffer(source->stream, lastFrameBuf);
}

void PipeWirePixelBuffer::start()
{
  uint8_t buffer[4096];
  struct spa_pod_builder b;
  const struct spa_pod *params[1];
  pw_stream* stream;

  spa_pod_builder_init(&b, buffer, sizeof(buffer));

  stream = pw_stream_new(core, "w0vncserver", props);
  if (!stream)
    throw std::runtime_error("Failed to create PipeWire stream");

  source->stream = stream;

  pw_stream_add_listener(stream, &streamListener,
                         &streamEvents, source);

  // FIXME: This is a bit ugly
  spa_rectangle defaultVideoSize{1280,720};
  spa_rectangle minVideoSize{1,1};
  spa_rectangle maxVideoSize{4096,4096};
  spa_fraction defaultFramerate{25,1};
  spa_fraction minFramerate{0,1};
  spa_fraction maxFramerate{60,1};

  params[0] = (spa_pod *)spa_pod_builder_add_object(
    &b, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
    SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
    SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
    SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(2,
                                                    SPA_VIDEO_FORMAT_RGBx,
                                                    SPA_VIDEO_FORMAT_BGRx),
    SPA_FORMAT_VIDEO_size,
    SPA_POD_CHOICE_RANGE_Rectangle(&defaultVideoSize, &minVideoSize,
                                   &maxVideoSize),
    SPA_FORMAT_VIDEO_framerate,
    SPA_POD_CHOICE_RANGE_Fraction(&defaultFramerate, &minFramerate,
                                  &maxFramerate));

  if (pw_stream_connect(stream, PW_DIRECTION_INPUT, pipewireId_,
                        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS),
                        params, 1) < 0) {
    throw std::runtime_error("Failed to connect PipeWire stream");
  }
}
