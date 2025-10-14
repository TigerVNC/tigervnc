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

#include <glib.h>
#include <glib-object.h>

#include <pipewire/pipewire.h>
#include <spa/param/video/raw.h>
#include <spa/utils/defs.h>
#include <spa/buffer/buffer.h>

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
  : PipeWireStream(pipewireFd, pipewireId), server(server_),
    lastSequence(0)
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
  core::Rect rect;
  spa_meta_header* header;
  bool frameDropped;

  chunk = buffer->datas[0].chunk;

  if (chunk->size == 0 || chunk->flags  & SPA_CHUNK_FLAG_CORRUPTED)
    return;

  // Check size
  if (chunk->size != (uint32_t) (width() * height() * (pipewirePixelFormat.bpp / 8))) {
    vlog.error("Invalid chunk size: %d", chunk->size);
    return;
  }

  header = (spa_meta_header*)spa_buffer_find_meta_data(buffer,
                                                       SPA_META_Header,
                                                       sizeof(*header));

  // Detect dropped frames. We can't rely on the damage events we've
  // gotten and have to mark the entire frame as damaged
  // https://bugs.kde.org/show_bug.cgi?id=510561
  frameDropped = (header->seq != lastSequence + 1);
  lastSequence = header->seq;

  rect = frameDropped ? getRect() : accumulatedDamage.get_bounding_rect();

  // Clamp damage outside of framebuffer
  if (!rect.enclosed_by(getRect()))
    rect = rect.intersect(getRect());

  srcBuffer = (uint8_t*)buffer->datas[0].data;
  srcStride = chunk->stride / (pipewirePixelFormat.bpp / 8);
  dstBuffer = getBufferRW(getRect(), &dstStride);

  ret = pixman_blt((uint32_t*)srcBuffer, (uint32_t*)dstBuffer,
                   srcStride, dstStride, pipewirePixelFormat.bpp,
                   getPF().bpp, rect.tl.x, rect.tl.y, rect.tl.x,
                   rect.tl.y, rect.width(), rect.height());
  commitBufferRW(rect);

  if (!ret) {
    uint8_t* damagedBuffer;

    damagedBuffer = &srcBuffer[(pipewirePixelFormat.bpp / 8) *
                               (rect.tl.y * srcStride + rect.tl.x)];
    imageRect(pipewirePixelFormat, rect, damagedBuffer, srcStride);
  }

  server->add_changed(rect);
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

  cursorBitmap = SPA_MEMBER(cursorData, cursorData->bitmap_offset,
                            struct spa_meta_bitmap);
  if (!supportedCursorPixelformat(cursorBitmap->format))
    return;

  cursorBuffer = SPA_MEMBER(cursorBitmap, cursorBitmap->offset, uint8_t);

  cursor->w = cursorBitmap->size.width;
  cursor->h = cursorBitmap->size.height;

  setCursor(cursor->w, cursor->h, cursor->hotspotX,
            cursor->hotspotY, cursorBuffer);
}

void PipeWirePixelBuffer::processDamage(spa_buffer* buffer)
{
  spa_meta* damage;
  spa_meta_region* metaRegion;

  damage = spa_buffer_find_meta(buffer, SPA_META_VideoDamage);

  if (!damage)
    return;

  spa_meta_for_each(metaRegion, damage) {
    if (!spa_meta_region_is_valid(metaRegion))
      continue;

    core::Point tl{metaRegion->region.position.x,
                   metaRegion->region.position.y};
    core::Point br{static_cast<int>(tl.x + metaRegion->region.size.width),
                   static_cast<int>(tl.y + metaRegion->region.size.height)};
    accumulatedDamage.assign_union({{tl, br}});
  }
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
