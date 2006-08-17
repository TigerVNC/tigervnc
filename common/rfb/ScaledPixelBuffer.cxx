/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- ScaledPixelBuffer.cxx

#include <rfb/Exception.h>
#include <rfb/ScaledPixelBuffer.h>

#include <math.h>
#include <memory.h>

using namespace rdr;
using namespace rfb;

ScaledPixelBuffer::ScaledPixelBuffer(U8 **src_data_, int src_width_,
                                     int src_height_, int scale, PixelFormat pf_)
  : scaled_data(0), scale_ratio(1) {

  setSourceBuffer(src_data_, src_width_, src_height_);
  setPF(pf_);
}

ScaledPixelBuffer::ScaledPixelBuffer() 
  : src_data(0), src_width(0), src_height(0), scale_ratio(1), scaled_width(0), 
    scaled_height(0), pf(PixelFormat(32,24,0,1,255,255,255,0,8,16)), 
    scaled_data(0) {
}

ScaledPixelBuffer::~ScaledPixelBuffer() {
}

void ScaledPixelBuffer::setSourceBuffer(U8 **src_data_, int w, int h) {
  src_data = src_data_;
  src_width  = w;
  src_height = h;
  calculateScaledBufferSize();
}

void ScaledPixelBuffer::setPF(const PixelFormat &pf_) {
  if (pf_.depth != 24) throw rfb::UnsupportedPixelFormatException();
  pf = pf_;
}

void ScaledPixelBuffer::setScaleRatio(double scale_ratio_) {
  if (scale_ratio != scale_ratio_) {
    scale_ratio = scale_ratio_;
    calculateScaledBufferSize();
  }
}

void ScaledPixelBuffer::scaleRect(const Rect& r) {
  static U8 *src_ptr, *ptr;
  static U8 r0, r1, r2, r3;
  static U8 g0, g1, g2, g3;
  static U8 b0, b1, b2, b3;
  static double c1_sub_dx, c1_sub_dy;
  static double dx, dy;
  static int i, j;
  static Rect changed_rect;

  // Calculate the changed pixel rect in the scaled image
  changed_rect = calculateScaleBoundary(r);

  // Scale the source rect to the destination image buffer using
  // bilinear interplation
  for (int y = changed_rect.tl.y; y < changed_rect.br.y; y++) {
    j = (int)(dy = y / scale_ratio);
    dy -= j;
    c1_sub_dy = 1 - dy;

    for (int x = changed_rect.tl.x; x < changed_rect.br.x; x++) {
      ptr = &scaled_data[(x + y*scaled_width) * 4];

      i = (int)(dx = x / scale_ratio);
      dx -= i;
      c1_sub_dx = 1 - dx;

      src_ptr = &(*src_data)[(i + (j*src_width))*4];
      b0 = *src_ptr; g0 = *(src_ptr+1); r0 = *(src_ptr+2);
      if (i+1 < src_width) {
        b1 = *(src_ptr+4); g1 = *(src_ptr+5); r1 = *(src_ptr+6);
      } else {
        b1 = b0; r1 = r0; g1 = g0;
      }
      if (j+1 < src_height) {
        src_ptr += src_width * 4;
        b3 = *src_ptr; g3 = *(src_ptr+1); r3 = *(src_ptr+2);
      } else {
        b3 = b0; r3 = r0; g3 = g0;
      }
      if ((i+1 < src_width) && (j+1 < src_height)) {
        b2 = *(src_ptr+4); g2 = *(src_ptr+5); r2 = *(src_ptr+6);
      } else if (i+1 >= src_width) {
        b2 = b3; r2 = r3; g2 = g3;
      } else {
        b2 = b1; r2 = r1; g2 = g1;
      }
      *ptr++ = (U8)((b0*c1_sub_dx+b1*dx)*c1_sub_dy + (b3*c1_sub_dx+b2*dx)*dy);
      *ptr++ = (U8)((g0*c1_sub_dx+g1*dx)*c1_sub_dy + (g3*c1_sub_dx+g2*dx)*dy);
      *ptr   = (U8)((r0*c1_sub_dx+r1*dx)*c1_sub_dy + (r3*c1_sub_dx+r2*dx)*dy);
    }
  }
}

Rect ScaledPixelBuffer::calculateScaleBoundary(const Rect& r) {
  static int x_start, y_start, x_end, y_end;
  x_start = r.tl.x == 0 ? 0 : ceil((r.tl.x-1) * scale_ratio);
  y_start = r.tl.y == 0 ? 0 : ceil((r.tl.y-1) * scale_ratio);
  x_end = ceil(r.br.x * scale_ratio - 1); 
  x_end = x_end < scaled_width ? x_end + 1 : scaled_width;
  y_end = ceil(r.br.y * scale_ratio - 1);
  y_end = y_end < scaled_height ? y_end + 1 : scaled_height;
  return Rect(x_start, y_start, x_end, y_end);
}

void ScaledPixelBuffer::calculateScaledBufferSize() {
  scaled_width  = (int)ceil(src_width  * scale_ratio);
  scaled_height = (int)ceil(src_height * scale_ratio);
}
