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

#include <unistd.h>

#include <stdexcept>

#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/debug/format.h>
#include <spa/debug/pod.h>

#include <core/LogWriter.h>
#include <core/string.h>
#include <rfb/PixelFormat.h>

#include "PipeWireSource.h"
#include "PipeWireStream.h"
#include "w0vncserver.h"

static core::LogWriter vlog("PipeWireStream");

const pw_stream_events PipeWireStream::streamEventsHandler {
  .version = PW_VERSION_STREAM_EVENTS,
  .destroy = nullptr,
  .state_changed = [](void* self, enum pw_stream_state old,
                      enum pw_stream_state state, const char* error) {
    ((PipeWireStream*)self)->handleStreamStateChanged(old, state, error);
  },
  .control_info = nullptr,
  .io_changed = nullptr,
  .param_changed = [](void* self, uint32_t id, const spa_pod* param) {
    ((PipeWireStream*)self)->handleStreamParamChanged(id, param);
   },
  .add_buffer = nullptr,
  .remove_buffer = nullptr,
  .process = [](void* self) {
    ((PipeWireStream*)self)->handleProcess();
  },
  .drained = nullptr,
#if PW_CHECK_VERSION(0,3,39)
  .command = nullptr,
#endif
#if PW_CHECK_VERSION(0,3,40)
  .trigger_done = nullptr
#endif
};

PipeWireStream::PipeWireStream(int pipeWireFd_, int nodeId)
  : pipeWireFd(pipeWireFd_)
{
  source = new PipeWireSource();

  context = pw_context_new(source->getLoop(), nullptr, 0);
  core = pw_context_connect_fd(context, pipeWireFd_, nullptr, 0);
  if (!core)
    throw std::runtime_error("Failed to connect to PipeWire FD");

  start(nodeId);
}

PipeWireStream::~PipeWireStream()
{
  pw_stream_disconnect(stream);
  // Iterate the loop once after disconnect to ensure proper cleanup.
  // Without this, we saw issues when re-initializing PipeWire
  pw_loop_iterate(source->getLoop(), 0);

  pw_stream_destroy(stream);

  pw_core_disconnect(core);
  pw_context_destroy(context);

  close(pipeWireFd);

  delete source;
}

void PipeWireStream::start(int nodeId)
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

  if (pw_stream_connect(stream, PW_DIRECTION_INPUT, nodeId,
                        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT |
                        PW_STREAM_FLAG_MAP_BUFFERS),
                        params, 1) < 0) {
    throw std::runtime_error("Failed to connect PipeWire stream");
  }
}

void PipeWireStream::handleStreamStateChanged(enum pw_stream_state /* old */,
                                              enum pw_stream_state state,
                                              const char* error)
{
  switch (state) {
  case PW_STREAM_STATE_UNCONNECTED:
    vlog.debug("PipeWire stream disconnected");
    break;
  case PW_STREAM_STATE_PAUSED:
    pw_stream_set_active(stream, true);
    break;
  case PW_STREAM_STATE_ERROR:
    fatal_error("%s", error);
    return;
  default:
    vlog.debug("Unhandled stream state: %d", state);
    break;
  }
}

void PipeWireStream::handleStreamParamChanged(uint32_t id,
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

  if (spa_format_parse(param, &spaFormat.media_type,
                       &spaFormat.media_subtype) < 0)
    return;

  if (spaFormat.media_type != SPA_MEDIA_TYPE_video)
    return;

  spa_pod_builder_init(&builder, paramsBuffer, sizeof(paramsBuffer));

#ifdef _DEBUG
  spa_debug_format(2, nullptr, param);
#endif

  switch (spaFormat.media_subtype) {
  case SPA_MEDIA_SUBTYPE_raw:
    spa_format_video_raw_parse(param, &spaFormat.info.raw);
    fbSize = SPA_RECTANGLE(spaFormat.info.raw.size.width,
                         spaFormat.info.raw.size.height);

    try {
      pf = convertPixelformat(spaFormat.info.raw.format);
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

  setParameters(fbSize.width, fbSize.height, pf);

  nParams = 0;

  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
    SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(4, 2, 8),
    SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
    SPA_PARAM_BUFFERS_size, SPA_POD_Int(fbSize.width * fbSize.height * mult),
    SPA_PARAM_BUFFERS_stride, SPA_POD_Int(fbStride),
    SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int((1 << SPA_DATA_MemFd)));

  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Header),
    SPA_PARAM_META_size, SPA_POD_Int(sizeof(spa_meta_header)));

  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta, SPA_PARAM_META_type,
    SPA_POD_Id(SPA_META_VideoDamage), SPA_PARAM_META_size,
    SPA_POD_CHOICE_RANGE_Int(sizeof(struct spa_meta_region) * 16,
                            sizeof(struct spa_meta_region) * 1,
                            sizeof(struct spa_meta_region) * 16));

  #define CURSOR_META_SIZE(w, h)                                       \
    (sizeof(struct spa_meta_cursor) + sizeof(struct spa_meta_bitmap) + \
            w * h * 4)

  params[nParams++] = (spa_pod*)spa_pod_builder_add_object(&builder,
    SPA_TYPE_OBJECT_ParamMeta, SPA_PARAM_Meta,
    SPA_PARAM_META_type, SPA_POD_Id(SPA_META_Cursor),
    SPA_PARAM_META_size, SPA_POD_CHOICE_RANGE_Int(
                             CURSOR_META_SIZE(64, 64),
                             CURSOR_META_SIZE(1, 1),
                             CURSOR_META_SIZE(512, 512)));

  pw_stream_update_params(stream, params, nParams);
}

void PipeWireStream::handleProcess()
{
  pw_buffer* buffer;
  spa_meta_header* header;

  buffer = pw_stream_dequeue_buffer(stream);

  if (!buffer)
    return;

  header = (spa_meta_header*)spa_buffer_find_meta_data(buffer->buffer,
                                                       SPA_META_Header,
                                                       sizeof(*header));

  // Frame is corrupted and does not contain any valid data
  if (header && header->flags & SPA_META_HEADER_FLAG_CORRUPTED) {
    vlog.debug("Skipping corrupted frame");
    pw_stream_queue_buffer(stream, buffer);
    return;
  }

  processBuffer(buffer);

  pw_stream_queue_buffer(stream, buffer);
}

rfb::PixelFormat PipeWireStream::convertPixelformat(int spaFormat)
{
  switch (spaFormat) {
  case SPA_VIDEO_FORMAT_BGRx:
  case SPA_VIDEO_FORMAT_BGRA:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            8, 16, 24);
  case SPA_VIDEO_FORMAT_RGBx:
  case SPA_VIDEO_FORMAT_RGBA:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            24, 16, 8);
  case SPA_VIDEO_FORMAT_xBGR:
  case SPA_VIDEO_FORMAT_ABGR:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  case SPA_VIDEO_FORMAT_xRGB:
  case SPA_VIDEO_FORMAT_ARGB:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_RGB:
    return rfb::PixelFormat(24, 24, true, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_BGR:
    return rfb::PixelFormat(24, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  default:
    throw std::runtime_error(core::format("format %d not supported",
                                          spaFormat));
  }
}
