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

#include <pixman.h>

#include <core/LogWriter.h>
#include <core/string.h>
#include <rfb/VNCServer.h>
#include <rfb/PixelFormat.h>

#include "PipeWireSource.h"
#include "PipeWirePixelBuffer.h"

static core::LogWriter vlog("PipewirePixelBuffer");

struct PipeWirecursor {
  uint32_t w;
  uint32_t h;
  int32_t x;
  int32_t y;
  int32_t hotspotX;
  int32_t hotspotY;
  int32_t stride;
};

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd_,
                                         uint32_t pipewireId_,
                                         rfb::VNCServer* server_)
  : pipewireFd(pipewireFd_), pipewireId(pipewireId_), server(server_)
{

  source = new PipeWireSource(this);
  cursor = new PipeWirecursor();
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
   delete source;
   delete cursor;
   close(pipewireFd);
}

void PipeWirePixelBuffer::updatePixelbuffer(int width, int height,
                                            rfb::PixelFormat pf)
{
  setSize(width, height);
  setPF(pf);
  server->setPixelBuffer(this);
}

void PipeWirePixelBuffer::processBuffer(pw_buffer* buffer)
{
  pixman_bool_t ret;
  spa_buffer* buf;
  spa_meta_region* damage;
  int srcStride;
  int dstStride;
  uint8_t* srcBuffer;
  uint8_t* dstBuffer;

  buf = buffer->buffer;
  srcBuffer = (uint8_t*)buf->datas[0].data;
  srcStride = buf->datas[0].chunk->stride / (getPF().bpp / 8);
  dstBuffer = getBufferRW({0, 0, width(), height()}, &dstStride);

  ret = pixman_blt((uint32_t*)srcBuffer, (uint32_t*)dstBuffer,
                   srcStride, dstStride, getPF().bpp,
                   getPF().bpp, 0, 0, 0, 0, width(), height());

  if (!ret)
    imageRect(getPF(), {0, 0, width(), height()}, srcBuffer, srcStride);

  server->add_changed({{0, 0, width(), height()}});
  damage = (spa_meta_region *)spa_buffer_find_meta_data(buf,
                                                        SPA_META_VideoDamage,
                                                        sizeof(*damage));
  if (damage) {
    core::Point tl{damage->region.position.x, damage->region.position.y};
    core::Point br{static_cast<int>(damage->region.position.x + damage->region.size.width),
                   static_cast<int>(damage->region.position.y + damage->region.size.height)};
    server->add_changed({{tl, br}});
  } else {
    server->add_changed({{0, 0, width(), height()}});
  }
}

void PipeWirePixelBuffer::processCursor(pw_buffer* buf)
{
  spa_meta_cursor* cursorData;
  spa_meta_bitmap* cursorBitmap;
  uint8_t* buffer;

  cursorData = (spa_meta_cursor *)spa_buffer_find_meta_data(buf->buffer,
                                                            SPA_META_Cursor,
                                                            sizeof(*cursorData));
  if(!cursorData){
    vlog.error("Could not find cursor metadata");
    return;
  }

  cursor->x = cursorData->position.x;
  cursor->y = cursorData->position.y;
  cursor->hotspotX = cursorData->hotspot.x;
  cursor->hotspotY = cursorData->hotspot.y;

  server->setCursorPos({cursor->x, cursor->y}, true);

  // No new cursor bitmap
  if (!cursorData->bitmap_offset)
    return;

  cursorBitmap = SPA_PTROFF(cursorData, cursorData->bitmap_offset,
                            struct spa_meta_bitmap);
  if (!supportedCursorPixelformat(cursorBitmap->format))
    return;

  buffer = SPA_PTROFF(cursorBitmap, cursorBitmap->offset, uint8_t);

  cursor->w = cursorBitmap->size.width;
  cursor->h = cursorBitmap->size.height;
  cursor->stride = cursorBitmap->stride;

  setCursor(cursor->w, cursor->h, cursor->hotspotX,
            cursor->hotspotY, buffer);
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

void PipeWirePixelBuffer::setCursor(int width, int height, int hotX,
                                    int hotY,
                                    const unsigned char* rgbaData)
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
    vlog.error("PipewirePixelBuffer::setCursor: %s",e.what());
  }

  delete [] cursorData;
}

bool PipeWirePixelBuffer::hasCursorData(pw_buffer* buf)
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

bool PipeWirePixelBuffer::supportedCursorPixelformat(int format_)
{
  return format_ == SPA_VIDEO_FORMAT_RGBx ||
         format_ == SPA_VIDEO_FORMAT_RGBA;
}
