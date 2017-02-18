/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

void Cursor::setSize(int w, int h) {
  int oldMaskLen = maskLen();
  ManagedPixelBuffer::setSize(w, h);
  if (maskLen() > oldMaskLen) {
    delete [] mask.buf;
    mask.buf = new rdr::U8[maskLen()];
  }
}

void Cursor::drawOutline(const Pixel& c)
{
  Cursor outlined;
  rdr::U8 cbuf[4];

  // Create a mirror of the existing cursor
  outlined.setPF(getPF());
  outlined.setSize(width(), height());
  outlined.hotspot = hotspot;

  // Clear the mirror's background to the outline colour
  outlined.getPF().bufferFromPixel(cbuf, c);
  outlined.fillRect(getRect(), cbuf);

  // Blit the existing cursor, using its mask
  outlined.maskRect(getRect(), data, mask.buf);

  // Now just adjust the mask to add the outline.  The outline pixels
  // will already be the right colour. :)
  int maskBytesPerRow = (width() + 7) / 8;
  for (int y = 0; y < height(); y++) {
    for (int byte=0; byte<maskBytesPerRow; byte++) {
      rdr::U8 m8 = mask.buf[y*maskBytesPerRow + byte];

      // Handle above & below outline
      if (y > 0) m8 |= mask.buf[(y-1)*maskBytesPerRow + byte];
      if (y < height()-1) m8 |= mask.buf[(y+1)*maskBytesPerRow + byte];

      // Left outline
      m8 |= mask.buf[y*maskBytesPerRow + byte] << 1;
      if (byte < maskBytesPerRow-1)
        m8 |= (mask.buf[y*maskBytesPerRow + byte + 1] >> 7) & 1;

      // Right outline
      m8 |= mask.buf[y*maskBytesPerRow + byte] >> 1;
      if (byte > 0)
        m8 |= (mask.buf[y*maskBytesPerRow + byte - 1] << 7) & 128;

      outlined.mask.buf[y*maskBytesPerRow + byte] = m8;
    }
  }

  // Replace the existing cursor & mask with the new one
  delete [] data;
  delete [] mask.buf;
  data = outlined.data; outlined.data = 0;
  mask.buf = outlined.mask.buf; outlined.mask.buf = 0;
}

rdr::U8* Cursor::getBitmap(Pixel* pix0, Pixel* pix1) const
{
  bool gotPix0 = false;
  bool gotPix1 = false;
  *pix0 = *pix1 = 0;
  rdr::U8Array source(maskLen());
  memset(source.buf, 0, maskLen());

  int maskBytesPerRow = (width() + 7) / 8;
  const rdr::U8 *data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (mask.buf[byte] & (1 << bit)) {
        Pixel pix = getPF().pixelFromBuffer(data_ptr);
        if (!gotPix0 || pix == *pix0) {
          gotPix0 = true;
          *pix0 = pix;
        } else if (!gotPix1 || pix == *pix1) {
          gotPix1 = true;
          *pix1 = pix;
          source.buf[byte] |= (1 << bit);
        } else {
          // not a bitmap
          return 0;
        }
      }
      data_ptr += getPF().bpp/8;
    }
  }
  return source.takeBuf();
}

// crop() determines the "busy" rectangle for the cursor - the minimum bounding
// rectangle containing actual pixels.  This isn't the most efficient algorithm
// but it's short.  For sanity, we make sure that the busy rectangle always
// includes the hotspot (the hotspot is unsigned on the wire so otherwise it
// would cause problems if it was above or left of the actual pixels)

void Cursor::crop()
{
  Rect busy = getRect().intersect(Rect(hotspot.x, hotspot.y,
                                       hotspot.x+1, hotspot.y+1));
  int maskBytesPerRow = (width() + 7) / 8;
  int x, y;
  for (y = 0; y < height(); y++) {
    for (x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (mask.buf[byte] & (1 << bit)) {
        if (x < busy.tl.x) busy.tl.x = x;
        if (x+1 > busy.br.x) busy.br.x = x+1;
        if (y < busy.tl.y) busy.tl.y = y;
        if (y+1 > busy.br.y) busy.br.y = y+1;
      }
    }
  }

  if (width() == busy.width() && height() == busy.height()) return;

  vlog.debug("cropping %dx%d to %dx%d", width(), height(),
             busy.width(), busy.height());

  // Copy the pixel data
  int newDataLen = busy.area() * (getPF().bpp/8);
  rdr::U8* newData = new rdr::U8[newDataLen];
  getImage(newData, busy);

  // Copy the mask
  int newMaskBytesPerRow = (busy.width()+7)/8;
  int newMaskLen = newMaskBytesPerRow * busy.height();
  rdr::U8* newMask = new rdr::U8[newMaskLen];
  memset(newMask, 0, newMaskLen);
  for (y = 0; y < busy.height(); y++) {
    int newByte, newBit;
    for (x = 0; x < busy.width(); x++) {
      int oldByte = (y+busy.tl.y) * maskBytesPerRow + (x+busy.tl.x) / 8;
      int oldBit = 7 - (x+busy.tl.x) % 8;
      newByte = y * newMaskBytesPerRow + x / 8;
      newBit = 7 - x % 8;
      if (mask.buf[oldByte] & (1 << oldBit))
        newMask[newByte] |= (1 << newBit);
    }
  }

  // Set the size and data to the new, cropped cursor.
  setSize(busy.width(), busy.height());
  hotspot = hotspot.subtract(busy.tl);
  delete [] data;
  delete [] mask.buf;
  datasize = newDataLen;
  data = newData;
  mask.buf = newMask;
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

  if (!framebuffer->getPF().equal(cursor->getPF()))
    throw Exception("RenderedCursor: Trying to render cursor on incompatible frame buffer");

  format = framebuffer->getPF();
  width_ = framebuffer->width();
  height_ = framebuffer->height();

  rawOffset = pos.subtract(cursor->hotspot);
  clippedRect = cursor->getRect(rawOffset).intersect(framebuffer->getRect());
  offset = clippedRect.tl;

  buffer.setPF(cursor->getPF());
  buffer.setSize(clippedRect.width(), clippedRect.height());

  data = framebuffer->getBuffer(buffer.getRect(offset), &stride);
  buffer.imageRect(buffer.getRect(), data, stride);

  diff = offset.subtract(rawOffset);
  data = cursor->getBuffer(buffer.getRect(diff), &stride);

  buffer.maskRect(buffer.getRect(), data, cursor->mask.buf, diff,
                  stride, (cursor->width() + 7) / 8);
}
