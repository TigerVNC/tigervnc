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
#include <stdint.h>

#include <stdexcept>

#include <glib.h>
#include <glib-object.h>

#include <pipewire/pipewire.h>
#include <pipewire/loop.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>

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
#include <core/string.h>
#include <rfb/VNCServer.h>
#include <rfb/PixelFormat.h>

#include "PipeWireStream.h"
#include "PipeWirePixelBuffer.h"

static core::LogWriter vlog("PipewirePixelBuffer");

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server_)
  : PipeWireStream(pipewireFd, pipewireId), server(server_)
{
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
}

void PipeWirePixelBuffer::processBuffer(pw_buffer* buffer)
{
  spa_buffer* spaBuffer;

  spaBuffer = buffer->buffer;

  processFrame(spaBuffer);
}

void PipeWirePixelBuffer::setParameters(int width, int height,
                                        rfb::PixelFormat pf)
{
  setSize(width, height);
  setPF(pf);
  pipewirePixelFormat = pf;
  server->setPixelBuffer(this);
}

void PipeWirePixelBuffer::processFrame(spa_buffer* buffer)
{
  int srcStride;
  uint8_t* srcBuffer;
  spa_chunk* chunk;

  chunk = buffer->datas[0].chunk;

  if (chunk->size == 0 || chunk->flags  & SPA_CHUNK_FLAG_CORRUPTED)
    return;

  // Check size
  if (chunk->size != (uint32_t) (width() * height() * (pipewirePixelFormat.bpp / 8))) {
    vlog.error("Invalid chunk size: %d", chunk->size);
    return;
  }

  srcBuffer = (uint8_t*)buffer->datas[0].data;
  srcStride = chunk->stride / (pipewirePixelFormat.bpp / 8);

  imageRect(pipewirePixelFormat, {0, 0, width(), height()}, srcBuffer,
            srcStride);

  server->add_changed({{0, 0, width(), height()}});
}

rfb::PixelFormat PipeWirePixelBuffer::convertPixelformat(int spaFormat)
{
  switch (spaFormat) {
  case SPA_VIDEO_FORMAT_BGRx:
  case SPA_VIDEO_FORMAT_BGRA:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            8, 16, 24);
  case SPA_VIDEO_FORMAT_RGBx:
  case SPA_VIDEO_FORMAT_RGBA:
    return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                            24, 16, 8);
  case SPA_VIDEO_FORMAT_xBGR:
  case SPA_VIDEO_FORMAT_ABGR:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  case SPA_VIDEO_FORMAT_xRGB:
  case SPA_VIDEO_FORMAT_ARGB:
    return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_RGB:
    return rfb::PixelFormat(24, 24, false, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_BGR:
    return rfb::PixelFormat(24, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  default:
    throw std::runtime_error(core::format("format %d not supported",
                                          spaFormat));
  }
}
