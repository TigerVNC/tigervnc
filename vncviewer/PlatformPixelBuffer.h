/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __PLATFORMPIXELBUFFER_H__
#define __PLATFORMPIXELBUFFER_H__

#include <rfb/PixelBuffer.h>

#include <FL/fl_draw.H>

class PlatformPixelBuffer: public rfb::ManagedPixelBuffer {
public:
  PlatformPixelBuffer(int width, int height) :
    rfb::ManagedPixelBuffer(rfb::PixelFormat(32, 24, false, true,
                                             255, 255, 255, 0, 8, 16),
                            width, height)
    {};

  inline void draw(int src_x, int src_y, int x, int y, int w, int h);
};

inline void PlatformPixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  int pixel_bytes, stride_bytes;
  const uchar *buf_start;

  pixel_bytes = getPF().bpp/8;
  stride_bytes = pixel_bytes * getStride();
  buf_start = data +
              pixel_bytes * src_x +
              stride_bytes * src_y;

  fl_draw_image(buf_start, x, y, w, h, pixel_bytes, stride_bytes);
}

#endif
