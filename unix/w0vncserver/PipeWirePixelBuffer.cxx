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

#include "PipeWireStream.h"
#include "PipeWirePixelBuffer.h"

struct PipeWireCursor {
  uint32_t w;
  uint32_t h;
  int32_t x;
  int32_t y;
  int32_t hotspotX;
  int32_t hotspotY;
};

static core::LogWriter vlog("PipewirePixelBuffer");

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server_)
  : PipeWireStream(pipewireFd, pipewireId), server(server_)
{
  cursor = new PipeWireCursor();
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
  delete cursor;
}

void PipeWirePixelBuffer::processBuffer(pw_buffer* buffer)
{
  spa_buffer* spaBuffer;

  spaBuffer = buffer->buffer;

  processDamage(spaBuffer);
  processCursor(spaBuffer);
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
  int dstStride;
  uint8_t* srcBuffer;
  spa_chunk* chunk;
  uint8_t* dstBuffer;
  pixman_bool_t ret;
  int x;
  int y;
  int w;
  int h;

  chunk = buffer->datas[0].chunk;

  if (chunk->size == 0 || chunk->flags  & SPA_CHUNK_FLAG_CORRUPTED)
    return;

  // Check size
  if (chunk->size != (uint32_t) (width() * height() * (pipewirePixelFormat.bpp / 8))) {
    vlog.error("Invalid chunk size: %d", chunk->size);
    return;
  }

  // Clamp values as they can be outside of the buffer
  x = std::max(0, accumulatedDamage.get_bounding_rect().tl.x);
  y = std::max(0, accumulatedDamage.get_bounding_rect().tl.y);
  w = accumulatedDamage.get_bounding_rect().width();
  h = accumulatedDamage.get_bounding_rect().height();

  if (x + w > width())
    w = width() - x;
  if (y + h > height())
    h = height() - y;

  srcBuffer = (uint8_t*)buffer->datas[0].data;
  srcStride = chunk->stride / (pipewirePixelFormat.bpp / 8);
  dstBuffer = getBufferRW({0, 0, width(), height()}, &dstStride);

  ret = pixman_blt((uint32_t*)srcBuffer, (uint32_t*)dstBuffer,
                   srcStride, dstStride, pipewirePixelFormat.bpp,
                   getPF().bpp, x, y, x, y, w, h);

  if (ret)
    commitBufferRW({x, y, x + w, y + h});
  else
    imageRect(pipewirePixelFormat, {x, y, x + w, y + h}, srcBuffer, srcStride);

  server->add_changed({{x, y, x + w, y + h}});
  accumulatedDamage.clear();
}

void PipeWirePixelBuffer::processCursor(spa_buffer* buffer)
{
  spa_meta_cursor* cursorData;
  spa_meta_bitmap* cursorBitmap;
  uint8_t* cursorBuffer;

  if (!hasCursorData(buffer))
    return;

  cursorData = (spa_meta_cursor*)spa_buffer_find_meta_data(buffer,
                                                           SPA_META_Cursor,
                                                           sizeof(*cursorData));
  assert(cursorData);

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

  cursorBuffer = SPA_PTROFF(cursorBitmap, cursorBitmap->offset, uint8_t);

  cursor->w = cursorBitmap->size.width;
  cursor->h = cursorBitmap->size.height;

  setCursor(cursor->w, cursor->h, cursor->hotspotX,
            cursor->hotspotY, cursorBuffer);
}

void PipeWirePixelBuffer::processDamage(spa_buffer* buffer)
{
  spa_meta* damage;
  core::Region damagedRegion;
  spa_meta_region* metaRegion;

  damage = spa_buffer_find_meta(buffer, SPA_META_VideoDamage);

  if (!damage)
    return;

  spa_meta_for_each(metaRegion, damage) {
    core::Point tl{metaRegion->region.position.x,
                    metaRegion->region.position.y};
    core::Point br{static_cast<int>
                      (tl.x + metaRegion->region.size.width),
                    static_cast<int>
                      (tl.y + metaRegion->region.size.height)};
    damagedRegion.assign_union({{tl, br}});
  }

  accumulatedDamage.assign_union(damagedRegion);
}

bool PipeWirePixelBuffer::hasCursorData(spa_buffer* buffer)
{
  spa_meta_cursor* mcs;

  mcs = (spa_meta_cursor*)spa_buffer_find_meta_data(buffer,
                                                    SPA_META_Cursor,
                                                    sizeof(*mcs));

  if (mcs && spa_meta_cursor_is_valid(mcs)) {
      // We got new cursor position / new cursor bitmap
      return true;
    }

  return false;
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

bool PipeWirePixelBuffer::supportedCursorPixelformat(int format_)
{
  return format_ == SPA_VIDEO_FORMAT_RGBx ||
         format_ == SPA_VIDEO_FORMAT_RGBA;
}
