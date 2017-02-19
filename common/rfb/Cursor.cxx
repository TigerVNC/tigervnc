/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2017 Pierre Ossman for Cendio AB
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
#include <assert.h>
#include <string.h>
#include <rfb/Cursor.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

using namespace rfb;

static LogWriter vlog("Cursor");

Cursor::Cursor(int width, int height, const Point& hotspot,
               const rdr::U8* data) :
  width_(width), height_(height), hotspot_(hotspot)
{
  this->data = new rdr::U8[width_*height_*4];
  memcpy(this->data, data, width_*height_*4);
}

Cursor::Cursor(const Cursor& other) :
  width_(other.width_), height_(other.height_),
  hotspot_(other.hotspot_)
{
  data = new rdr::U8[width_*height_*4];
  memcpy(data, other.data, width_*height_*4);
}

Cursor::~Cursor()
{
  delete [] data;
}

rdr::U8* Cursor::getBitmap() const
{
  rdr::U8Array source((width()+7)/8*height());
  memset(source.buf, 0, (width()+7)/8*height());

  int maskBytesPerRow = (width() + 7) / 8;
  const rdr::U8 *data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (data_ptr[3] >= 0x80) {
        // Use Luma with BT.709 coefficients for grayscale
        unsigned luma;

        luma = 0;
        luma += (unsigned)data_ptr[0] * 13933; // 0.2126
        luma += (unsigned)data_ptr[1] * 46871; // 0.7152
        luma += (unsigned)data_ptr[2] * 4732;  // 0.0722
        luma /= 65536;

        // Gamma compensated half intensity gray
        if (luma > 187)
          source.buf[byte] |= (1 << bit);
      }
      data_ptr += 4;
    }
  }

  return source.takeBuf();
}

rdr::U8* Cursor::getMask() const
{
  rdr::U8Array mask((width()+7)/8*height());
  memset(mask.buf, 0, (width()+7)/8*height());

  int maskBytesPerRow = (width() + 7) / 8;
  const rdr::U8 *data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (data_ptr[3] >= 0x80)
        mask.buf[byte] |= (1 << bit);
      data_ptr += 4;
    }
  }

  return mask.takeBuf();
}

// crop() determines the "busy" rectangle for the cursor - the minimum bounding
// rectangle containing actual pixels.  This isn't the most efficient algorithm
// but it's short.  For sanity, we make sure that the busy rectangle always
// includes the hotspot (the hotspot is unsigned on the wire so otherwise it
// would cause problems if it was above or left of the actual pixels)

void Cursor::crop()
{
  Rect busy = Rect(0, 0, width_, height_);
  busy = busy.intersect(Rect(hotspot_.x, hotspot_.y,
                             hotspot_.x+1, hotspot_.y+1));
  int x, y;
  rdr::U8 *data_ptr = data;
  for (y = 0; y < height(); y++) {
    for (x = 0; x < width(); x++) {
      if (data_ptr[3] > 0) {
        if (x < busy.tl.x) busy.tl.x = x;
        if (x+1 > busy.br.x) busy.br.x = x+1;
        if (y < busy.tl.y) busy.tl.y = y;
        if (y+1 > busy.br.y) busy.br.y = y+1;
      }
      data_ptr += 4;
    }
  }

  if (width() == busy.width() && height() == busy.height()) return;

  // Copy the pixel data
  int newDataLen = busy.area() * 4;
  rdr::U8* newData = new rdr::U8[newDataLen];
  data_ptr = newData;
  for (y = busy.tl.y; y < busy.br.y; y++) {
    memcpy(data_ptr, data + y*width()*4 + busy.tl.x*4, busy.width()*4);
    data_ptr += busy.width()*4;
  }

  // Set the size and data to the new, cropped cursor.
  width_ = busy.width();
  height_ = busy.height();
  hotspot_ = hotspot_.subtract(busy.tl);
  delete [] data;
  data = newData;
}

RenderedCursor::RenderedCursor()
{
}

const rdr::U8* RenderedCursor::getBuffer(const Rect& _r, int* stride) const
{
  Rect r;

  r = _r.translate(offset.negate());
  if (!r.enclosed_by(buffer.getRect()))
    throw Exception("RenderedCursor: Invalid area requested");

  return buffer.getBuffer(r, stride);
}

void RenderedCursor::update(PixelBuffer* framebuffer,
                            Cursor* cursor, const Point& pos)
{
  Point rawOffset, diff;
  Rect clippedRect;

  const rdr::U8* data;
  int stride;

  assert(framebuffer);
  assert(cursor);

  format = framebuffer->getPF();
  width_ = framebuffer->width();
  height_ = framebuffer->height();

  rawOffset = pos.subtract(cursor->hotspot());
  clippedRect = Rect(0, 0, cursor->width(), cursor->height())
                .translate(rawOffset)
                .intersect(framebuffer->getRect());
  offset = clippedRect.tl;

  buffer.setPF(format);
  buffer.setSize(clippedRect.width(), clippedRect.height());

  data = framebuffer->getBuffer(buffer.getRect(offset), &stride);
  buffer.imageRect(buffer.getRect(), data, stride);

  diff = offset.subtract(rawOffset);
  for (int y = 0;y < buffer.height();y++) {
    for (int x = 0;x < buffer.width();x++) {
      size_t idx;
      rdr::U8 bg[4], fg[4];
      rdr::U8 rgb[3];

      idx = (y+diff.y)*cursor->width() + (x+diff.x);
      memcpy(fg, cursor->getBuffer() + idx*4, 4);

      if (fg[3] == 0x00)
        continue;
      else if (fg[3] == 0xff) {
        memcpy(rgb, fg, 3);
      } else {
        buffer.getImage(bg, Rect(x, y, x+1, y+1));
        format.rgbFromBuffer(rgb, bg, 1);
        // FIXME: Gamma aware blending
        for (int i = 0;i < 3;i++) {
          rgb[i] = (unsigned)rgb[i]*(255-fg[3])/255 +
                   (unsigned)fg[i]*fg[3]/255;
        }
      }

      format.bufferFromRGB(bg, rgb, 1);
      buffer.imageRect(Rect(x, y, x+1, y+1), bg);
    }
  }
}
