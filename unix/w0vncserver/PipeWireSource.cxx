/* Copyright 2025 Adam Halim for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <stdexcept>

#include <pipewire/pipewire.h>
#include <pipewire/loop.h>
#include <spa/pod/builder.h>
#include <spa/buffer/buffer.h>
#include <spa/buffer/meta.h>
#include <spa/param/video/raw.h>
#include <spa/param/latency-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/pod.h>
#include <spa/utils/defs.h>
#include <spa/utils/type.h>
#include <spa/debug/format.h>
#include <spa/debug/pod.h>

#include <core/LogWriter.h>
#include <pipewire/stream.h>

#include <rfb/PixelFormat.h>
#include <rfb/VNCServer.h>

#include "w0vncserver.h"
#include "PipeWirePixelBuffer.h"
#include "PipeWireSource.h"

core::BoolParameter
  damageEnabled("experimentalDamageEnabled",
                "Enable framebuffer damage tracking", false);

#define MAX_BUFFERS 64

static core::LogWriter vlog("PipewireSource");

static int pwInitCount = 0;

struct _PipeWireSource {
  GSource base;
  PipeWireSource* instance;
};

GSourceFuncs PipeWireSource::pipewireSourceFuncs {
  .prepare = nullptr,
  .check = nullptr,
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    return ((_PipeWireSource*)source)->instance->sourceLoopDispatch();
  },
  .finalize =nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

const pw_stream_events PipeWireSource::streamEventsHandler {
  .version = PW_VERSION_STREAM_EVENTS,
  .destroy = nullptr,
  .state_changed = [](void* self, enum pw_stream_state old,
                      enum pw_stream_state state, const char* error) {
    ((PipeWireSource*)self)->handleStreamStateChanged(old, state, error);
  },
  .control_info = nullptr,
  .io_changed = nullptr,
  .param_changed = [](void* self, uint32_t id, const spa_pod* param) {
    ((PipeWireSource*)self)->handleStreamParamChanged(id, param);
   },
  .add_buffer = nullptr,
  .remove_buffer = nullptr,
  .process = [](void* self) {
    ((PipeWireSource*)self)->handleProcess();
  },
  .drained = nullptr,
#if PW_CHECK_VERSION(0,3,39)
  .command = nullptr,
#endif
#if PW_CHECK_VERSION(0,3,40)
  .trigger_done = nullptr
#endif
};


PipeWireSource::PipeWireSource(PipeWirePixelBuffer* pb_)
  : pb(pb_)
{
  // pw_init() and pw_deinit() may only be called once prior to version
  // 0.3.49.
  if (pwInitCount++ == 0)
    pw_init(nullptr, nullptr);

  loop = pw_loop_new(nullptr);
  source = (_PipeWireSource *)g_source_new(&pipewireSourceFuncs,
                                           sizeof(_PipeWireSource));
  g_source_add_unix_fd(&source->base, pw_loop_get_fd (loop),
                       (GIOCondition)(G_IO_IN | G_IO_ERR));
  g_source_attach (&source->base, nullptr);

  context = pw_context_new(loop, nullptr, 0);
  core = pw_context_connect_fd(context, pb->fd(), nullptr, 0);
  if (!core)
    throw std::runtime_error("Failed to connect to PipeWire FD");

  source->instance = this;

  start();
}


PipeWireSource::~PipeWireSource()
{
  spa_hook_remove(&streamListener);
  spa_zero(streamListener);

  g_source_unref (&source->base);
  g_source_destroy(&source->base);

  pw_stream_disconnect(stream);
  // Iterate the loop once after disconnect to ensure proper cleanup.
  // Without this, we saw issues when re-initializing PipeWire
  pw_loop_iterate(loop, 0);

  pw_stream_destroy(stream);
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_loop_destroy(loop);

  assert(pwInitCount > 0);
  if (--pwInitCount == 0)
    pw_deinit();
}

void PipeWireSource::start()
{
  uint8_t buffer[4096];
  const spa_pod *params[1];
  spa_pod_builder builder;
  pw_properties* props;

  spa_pod_builder_init(&builder, buffer, sizeof(buffer));

  props = pw_properties_new(PW_KEY_REMOTE_NAME, "pipewire-0",
                            PW_KEY_MEDIA_TYPE, "Video",
                            PW_KEY_MEDIA_CATEGORY, "Capture",
                            PW_KEY_MEDIA_ROLE, "Screen",
                            nullptr);

  stream = pw_stream_new(core, "w0vncserver", props);
  if (!stream)
    throw std::runtime_error("Failed to create PipeWire stream");

  pw_stream_add_listener(stream, &streamListener, &streamEventsHandler, this);

  // FIXME: This is a bit ugly
  spa_rectangle defaultVideoSize{1280,720};
  spa_rectangle minVideoSize{1,1};
  spa_rectangle maxVideoSize{4096,4096};
  spa_fraction defaultFramerate{25,1};
  spa_fraction minFramerate{0,1};
  spa_fraction maxFramerate{60,1};

  params[0] = (spa_pod *)spa_pod_builder_add_object(
    &builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
    SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
    SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
    SPA_FORMAT_VIDEO_format, SPA_POD_CHOICE_ENUM_Id(2, SPA_VIDEO_FORMAT_RGBx,
                                                       SPA_VIDEO_FORMAT_BGRx),
    SPA_FORMAT_VIDEO_size,
    SPA_POD_CHOICE_RANGE_Rectangle(&defaultVideoSize, &minVideoSize,
                                   &maxVideoSize),
    SPA_FORMAT_VIDEO_framerate,
    SPA_POD_CHOICE_RANGE_Fraction(&defaultFramerate, &minFramerate,
                                  &maxFramerate));

  if (pw_stream_connect(stream, PW_DIRECTION_INPUT, pb->id(),
                        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS),
                        params, 1) < 0) {
    throw std::runtime_error("Failed to connect PipeWire stream");
  }
}

int PipeWireSource::sourceLoopDispatch()
{
  int result;

  result = pw_loop_iterate (loop, 0);
  if (result < 0)
    vlog.error("pipewire_loop_iterate failed: %s", g_strerror (result));

  return G_SOURCE_CONTINUE;
}

void PipeWireSource::handleStreamStateChanged(enum pw_stream_state old,
                                              enum pw_stream_state state,
                                              const char *error)
{
  (void)old;

  switch (state) {
  case PW_STREAM_STATE_UNCONNECTED:
    pw_loop_destroy(loop);
    break;
  case PW_STREAM_STATE_PAUSED:
    pw_stream_set_active(stream, true);
    break;
  case PW_STREAM_STATE_ERROR:
    fatal_error(error);
    return;
  default:
    vlog.debug("Unhandled stream state: %d", state);
    break;
  }
}

void PipeWireSource::handleStreamParamChanged(uint32_t id,
                                              const spa_pod *param)
{
  uint8_t paramsBuffer[1024];
  spa_pod_builder builder;
  int nParams;
  const spa_pod *params[5];
  int32_t mult;
  spa_video_info spaFormat;
  spa_rectangle fbSize;
  int32_t fbStride;
  rfb::PixelFormat pf;

  if (param == nullptr || id != SPA_PARAM_Format) {
    vlog.debug("clear format");
    return;
  }
  if (param != nullptr && id == SPA_PARAM_Tag) {
    spa_debug_pod(0, nullptr, param);
    return;
  }

  if (spa_format_parse(param, &spaFormat.media_type,
                       &spaFormat.media_subtype) < 0)
    return;

  if (spaFormat.media_type != SPA_MEDIA_TYPE_video)
    return;

  spa_pod_builder_init(&builder, paramsBuffer, sizeof(paramsBuffer));
  spa_debug_format(2, NULL, param);

  switch (spaFormat.media_subtype) {
  case SPA_MEDIA_SUBTYPE_raw:
    spa_format_video_raw_parse(param, &spaFormat.info.raw);
    fbSize = SPA_RECTANGLE(spaFormat.info.raw.size.width,
                         spaFormat.info.raw.size.height);

    try {
      pf = pb->convertPixelformat(SPA_VIDEO_FORMAT_BGRx);
    } catch (std::runtime_error& e) {
        pw_stream_set_error(stream, -EINVAL, "%s", e.what());
        return;
    }

    mult = pf.bpp / 8;
    fbStride = spaFormat.info.raw.size.width * mult;
    break;
  case SPA_VIDEO_FORMAT_UNKNOWN:
    pw_stream_set_error(stream, -EINVAL, "unknown pixel format");
    return;
  default:
    pw_stream_set_error(stream, -EINVAL, "unsupported pixel format");
    return;
  }

  if (!fbSize.width || !fbSize.height) {
    pw_stream_set_error(stream, -EINVAL, "invalid framebuffer size: %dx%d",
                                          fbSize.width, fbSize.height);
    return;
  }

  pb->updatePixelbuffer(fbSize.width, fbSize.height, pf);

  nParams = 0;
  /* a SPA_TYPE_OBJECT_ParamBuffers object defines the acceptable size,
   * number, stride etc of the buffers */
  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
    SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(8, 2, MAX_BUFFERS),
    SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
    SPA_PARAM_BUFFERS_size, SPA_POD_Int(fbSize.width * fbSize.height * mult),
    SPA_PARAM_BUFFERS_stride, SPA_POD_Int(fbStride),
    SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemPtr)));

  /* header metadata */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
    SPA_PARAM_META_size, SPA_POD_Int(sizeof(spa_meta_header)));

  if (damageEnabled) {
    /* Damage information */
    params[nParams++] = (spa_pod *)spa_pod_builder_add_object(&builder,
      SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
      SPA_POD_Id(SPA_META_VideoDamage), SPA_PARAM_META_size,
      SPA_POD_CHOICE_RANGE_Int(sizeof(struct spa_meta_region) * 16,
                              sizeof(struct spa_meta_region) * 1,
                              sizeof(struct spa_meta_region) * 16));
  }

  #define CURSOR_META_SIZE(w, h)                                       \
  (sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) +   \
   w * h * 4)

  /* cursor information */
  params[nParams++] = (spa_pod *)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
    SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(
                             CURSOR_META_SIZE(64, 64),
                             CURSOR_META_SIZE(1, 1),
                             CURSOR_META_SIZE(512, 512)));

  pw_stream_update_params(stream, params, nParams);
}

void PipeWireSource::handleProcess()
{
  pw_buffer* lastFrameBuf;
  pw_buffer* lastCursorBuf;
  size_t bufSize;

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

    nextBuf = pw_stream_dequeue_buffer(stream);

    if (!nextBuf)
      break;

    header = (spa_meta_header *)spa_buffer_find_meta_data(nextBuf->buffer,
                                                          SPA_META_Header,
                                                          sizeof(*header));
    // Corrupted buffers contain no frame or cursor data
    if (header && header->flags & SPA_META_HEADER_FLAG_CORRUPTED) {
      pw_stream_queue_buffer(stream, nextBuf);
      continue;
    }

    if (pb->hasCursorData(nextBuf)) {
      if (lastCursorBuf == lastFrameBuf)
        lastCursorBuf = nullptr;

      if (lastCursorBuf)
        pw_stream_queue_buffer(stream, lastCursorBuf);
      lastCursorBuf = nextBuf;
    }

    // Frame data available
    if (!(nextBuf->buffer->datas[0].chunk->flags &
        SPA_CHUNK_FLAG_CORRUPTED)) {
      if (lastCursorBuf == lastFrameBuf)
        lastFrameBuf = nullptr;

      if (lastFrameBuf)
        pw_stream_queue_buffer(stream, lastFrameBuf);
      lastFrameBuf = nextBuf;
    }

    if (nextBuf != lastCursorBuf && nextBuf != lastFrameBuf)
          pw_stream_queue_buffer(stream, nextBuf);
  }

  if (lastCursorBuf) {
    pb->processCursor(lastCursorBuf);
    if (lastCursorBuf != lastFrameBuf) {
      pw_stream_queue_buffer(stream, lastCursorBuf);
    }
  }

  if (!lastFrameBuf)
    return;

  assert(lastFrameBuf->buffer->datas[0].chunk->size > 0);

  bufSize = lastFrameBuf->buffer->datas[0].chunk->size;
  if (bufSize == 0) {
    pw_stream_queue_buffer(stream, lastFrameBuf);
    vlog.debug("Skipping empty buffer");
    return;
  }

  pb->processBuffer(lastFrameBuf);

  pw_stream_queue_buffer(stream, lastFrameBuf);
}
