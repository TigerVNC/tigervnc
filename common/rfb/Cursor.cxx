/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2023 Pierre Ossman for Cendio AB
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
#include <string.h>

#include <rfb/Cursor.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

using namespace rfb;

static LogWriter vlog("Cursor");

Cursor::Cursor(int width, int height, const Point& hotspot,
               const uint8_t* data_) :
  width_(width), height_(height), hotspot_(hotspot)
{
  data = new uint8_t[width_*height_*4];
  memcpy(data, data_, width_*height_*4);
}

Cursor::Cursor(const Cursor& other) :
  width_(other.width_), height_(other.height_),
  hotspot_(other.hotspot_)
{
  data = new uint8_t[width_*height_*4];
  memcpy(data, other.data, width_*height_*4);
}

Cursor::~Cursor()
{
  delete [] data;
}

static unsigned short pow223[] = { 0, 30, 143, 355, 676, 1113, 1673,
                                   2361, 3181, 4139, 5237, 6479, 7869,
                                   9409, 11103, 12952, 14961, 17130,
                                   19462, 21960, 24626, 27461, 30467,
                                   33647, 37003, 40535, 44245, 48136,
                                   52209, 56466, 60907, 65535 };

static unsigned short ipow(unsigned short val, unsigned short lut[])
{
  int idx = val >> (16-5);
  int a, b;

  if (val < 0x8000) {
    a = lut[idx];
    b = lut[idx+1];
  } else {
    a = lut[idx-1];
    b = lut[idx];
  }

  return (val & 0x7ff) * (b-a) / 0x7ff + a;
}

static unsigned short srgb_to_lin(unsigned char srgb)
{
  return ipow((unsigned)srgb * 65535 / 255, pow223);
}

// Floyd-Steinberg dithering
static void dither(int width, int height, int32_t* data)
{
  for (int y = 0; y < height; y++) {
    for (int x_ = 0; x_ < width; x_++) {
      int x = (y & 1) ? (width - x_ - 1) : x_;
      int error;

      if (data[x] > 32767) {
        error = data[x] - 65535;
        data[x] = 65535;
      } else {
        error = data[x] - 0;
        data[x] = 0;
      }

      if (y & 1) {
        if (x > 0) {
          data[x - 1] += error * 7 / 16;
        }
        if ((y + 1) < height) {
          if (x > 0)
            data[x - 1 + width] += error * 3 / 16;
          data[x + width] += error * 5 / 16;
          if ((x + 1) < width)
            data[x + 1] += error * 1 / 16;
        }
      } else {
        if ((x + 1) < width) {
          data[x + 1] += error * 7 / 16;
        }
        if ((y + 1) < height) {
          if ((x + 1) < width)
            data[x + 1 + width] += error * 3 / 16;
          data[x + width] += error * 5 / 16;
          if (x > 0)
            data[x - 1] += error * 1 / 16;
        }
      }
    }
    data += width;
  }
}

std::vector<uint8_t> Cursor::getBitmap() const
{
  // First step is converting to luminance
  std::vector<int32_t> luminance(width()*height());
  int32_t *lum_ptr = luminance.data();
  const uint8_t *data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int32_t lum;

      // Use BT.709 coefficients for grayscale
      lum = 0;
      lum += (uint32_t)srgb_to_lin(data_ptr[0]) * 6947;  // 0.2126
      lum += (uint32_t)srgb_to_lin(data_ptr[1]) * 23436; // 0.7152
      lum += (uint32_t)srgb_to_lin(data_ptr[2]) * 2366;  // 0.0722
      lum /= 32768;

      *lum_ptr++ = lum;
      data_ptr += 4;
    }
  }

  // Then diterhing
  dither(width(), height(), luminance.data());

  // Then conversion to a bit mask
  std::vector<uint8_t> source((width()+7)/8*height());
  memset(source.data(), 0, source.size());
  int maskBytesPerRow = (width() + 7) / 8;
  lum_ptr = luminance.data();
  data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (*lum_ptr > 32767)
        source[byte] |= (1 << bit);
      lum_ptr++;
      data_ptr += 4;
    }
  }

  return source;
}

std::vector<uint8_t> Cursor::getMask() const
{
  // First step is converting to integer array
  std::vector<int32_t> alpha(width()*height());
  int32_t *alpha_ptr = alpha.data();
  const uint8_t *data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      *alpha_ptr++ = (uint32_t)data_ptr[3] * 65535 / 255;
      data_ptr += 4;
    }
  }

  // Then diterhing
  dither(width(), height(), alpha.data());

  // Then conversion to a bit mask
  std::vector<uint8_t> mask((width()+7)/8*height());
  memset(mask.data(), 0, mask.size());
  int maskBytesPerRow = (width() + 7) / 8;
  alpha_ptr = alpha.data();
  data_ptr = data;
  for (int y = 0; y < height(); y++) {
    for (int x = 0; x < width(); x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;
      if (*alpha_ptr > 32767)
        mask[byte] |= (1 << bit);
      alpha_ptr++;
      data_ptr += 4;
    }
  }

  return mask;
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
  uint8_t *data_ptr = data;
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
  uint8_t* newData = new uint8_t[newDataLen];
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

const uint8_t* RenderedCursor::getBuffer(const Rect& _r, int* stride) const
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

  const uint8_t* data;
  int stride;

  assert(framebuffer);
  assert(cursor);

  format = framebuffer->getPF();
  setSize(framebuffer->width(), framebuffer->height());

  rawOffset = pos.subtract(cursor->hotspot());
  clippedRect = Rect(0, 0, cursor->width(), cursor->height())
                .translate(rawOffset)
                .intersect(framebuffer->getRect());
  offset = clippedRect.tl;

  buffer.setPF(format);
  buffer.setSize(clippedRect.width(), clippedRect.height());

  // Bail out early to avoid pestering the framebuffer with
  // bogus coordinates
  if (clippedRect.area() == 0)
    return;

  data = framebuffer->getBuffer(buffer.getRect(offset), &stride);
  buffer.imageRect(buffer.getRect(), data, stride);

  diff = offset.subtract(rawOffset);
  for (int y = 0;y < buffer.height();y++) {
    for (int x = 0;x < buffer.width();x++) {
      size_t idx;
      uint8_t bg[4], fg[4];
      uint8_t rgb[3];

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
