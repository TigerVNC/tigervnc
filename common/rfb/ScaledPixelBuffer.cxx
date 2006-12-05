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
#include <stdlib.h>

using namespace rdr;
using namespace rfb;

ScaledPixelBuffer::ScaledPixelBuffer(U8 **src_data_, int src_width_,
                                     int src_height_, int scale, PixelFormat pf_)
  : scale_ratio(1), scaleFilterID(scaleFilterBicubic),
    xWeightTabs(0), yWeightTabs(0), scaled_data(0) {

  setSourceBuffer(src_data_, src_width_, src_height_);
  setPF(pf_);
}

ScaledPixelBuffer::ScaledPixelBuffer() 
  : src_width(0), src_height(0), scaled_width(0), scaled_height(0),
    scale_ratio(1), scaleFilterID(scaleFilterBicubic),
    xWeightTabs(0), yWeightTabs(0), src_data(0), scaled_data(0) {
  memset(&pf, 0, sizeof(pf));
}

ScaledPixelBuffer::~ScaledPixelBuffer() {
  freeWeightTabs();
}

void ScaledPixelBuffer::freeWeightTabs() {
  if (xWeightTabs) {
    for (int i = 0; i < scaled_width; i++) delete [] xWeightTabs[i].weight;
    delete [] xWeightTabs;
    xWeightTabs = 0;
  }
  if (yWeightTabs) {
    for (int i = 0; i < scaled_height; i++) delete [] yWeightTabs[i].weight;
    delete [] yWeightTabs;
    yWeightTabs = 0;
  }
}

void ScaledPixelBuffer::setSourceBuffer(U8 **src_data_, int w, int h) {
  freeWeightTabs();
  src_data = src_data_;
  src_width  = w;
  src_height = h;
  calculateScaledBufferSize();
  scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, scale_ratio, &xWeightTabs);
  scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, scale_ratio, &yWeightTabs);
}

void ScaledPixelBuffer::setPF(const PixelFormat &pf_) {
  ///if (pf_.depth != 24) throw rfb::UnsupportedPixelFormatException();
  pf = pf_;
}

void ScaledPixelBuffer::setScaleRatio(double scale_ratio_) {
  if (scale_ratio != scale_ratio_) {
    freeWeightTabs();
    scale_ratio = scale_ratio_;
    calculateScaledBufferSize();
    scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, scale_ratio, &xWeightTabs);
    scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, scale_ratio, &yWeightTabs);
  }
}

inline void ScaledPixelBuffer::rgbFromPixel(U32 p, int &r, int &g, int &b) {
  r = (((p >> pf.redShift  ) & pf.redMax  ) * 255 + pf.redMax  /2) / pf.redMax;
  g = (((p >> pf.greenShift) & pf.greenMax) * 255 + pf.greenMax/2) / pf.greenMax;
  b = (((p >> pf.blueShift ) & pf.blueMax ) * 255 + pf.blueMax /2) / pf.blueMax;
}

inline U32 ScaledPixelBuffer::getSourcePixel(int x, int y) {
  int bytes_per_pixel = pf.bpp / 8;
  U8 *ptr = &(*src_data)[(x + y*src_width)*bytes_per_pixel];
  if (bytes_per_pixel == 1) {
    return *ptr;
  } else if (bytes_per_pixel == 2) {
    int b0 = *ptr++; int b1 = *ptr;
    return b1 << 8 | b0;
  } else if (bytes_per_pixel == 4) {
    int b0 = *ptr++; int b1 = *ptr++;
    int b2 = *ptr++; int b3 = *ptr;
    return b3 << 24 | b2 << 16 | b1 << 8 | b0;
  } else {
    return 0;
  }
}

void ScaledPixelBuffer::scaleRect(const Rect& rect) {
  Rect changed_rect;
  U8 *ptr, *ptrs, *px, *pxs;
  double rx, gx, bx, red, green, blue, *xweight, *yweight, xWeight, yWeight;
  int r, g, b, xwi, ywi;

  // Calculate the changed pixel rect in the scaled image
  changed_rect = calculateScaleBoundary(rect);

  int bytesPerSrcPixel = pf.bpp / 8;
  int bytesPerSrcRow = src_width * bytesPerSrcPixel;
  int bytesPerScaledRow = scaled_width * 4;

  ptrs = &(*scaled_data)[(changed_rect.tl.x + changed_rect.tl.y*scaled_width) * 4];
  for (int y = changed_rect.tl.y; y < changed_rect.br.y; y++) {
    ptr = ptrs;
    yweight = yWeightTabs[y].weight;

    for (int x = changed_rect.tl.x; x < changed_rect.br.x; x++) {
      ywi = 0; red = 0; green = 0; blue = 0;
      xweight = xWeightTabs[x].weight;
    
      // Calculate the scaled pixel value at (x, y) coordinates by
      // convolution the matrix from source image:
      // [(xWeight.i0,yWeight.i0)......(xWeight.i1-1,yWeight.i0)]
      // [......................................................]
      // [(xWeight.i0,yWeight.i1-1)..(xWeight.i1-1,yWeight.i1-1)],
      // where [i0, i1) is the scaled filter interval.
      pxs = &(*src_data)[(xWeightTabs[x].i0 + yWeightTabs[y].i0*src_width) * bytesPerSrcPixel];
      for (int ys = yWeightTabs[y].i0; ys < yWeightTabs[y].i1; ys++) {
        xwi = 0; rx = 0; gx = 0; bx = 0; px = pxs;
        for (int xs = xWeightTabs[x].i0; xs < xWeightTabs[x].i1; xs++) {
          rgbFromPixel(*((U32*)px), r, g, b);
          xWeight = xweight[xwi++];
          rx += r * xWeight;
          gx += g * xWeight;
          bx += b * xWeight;
          px += bytesPerSrcPixel;
        }
        yWeight = yweight[ywi++];
        red += rx * yWeight;
        green += gx * yWeight;
        blue += bx * yWeight;
        pxs += bytesPerSrcRow;
      }
      *ptr++ = U8(blue);
      *ptr++ = U8(green);
      *ptr++ = U8(red);
      ptr++;
    }
    ptrs += bytesPerScaledRow;
  }
}

Rect ScaledPixelBuffer::calculateScaleBoundary(const Rect& r) {
  int x_start, y_start, x_end, y_end;
  double radius = scaleFilters[scaleFilterID].radius;
  double translate = 0.5*scale_ratio - 0.5;
  x_start = (int)ceil(scale_ratio*(r.tl.x-radius) + translate);
  y_start = (int)ceil(scale_ratio*(r.tl.y-radius) + translate);
  x_end = (int)ceil(scale_ratio*(r.br.x+radius) + translate);
  y_end = (int)ceil(scale_ratio*(r.br.y+radius) + translate);
  if (x_start < 0) x_start = 0;
  if (y_start < 0) y_start = 0;
  if (x_end > scaled_width) x_end = scaled_width;
  if (y_end > scaled_height) y_end = scaled_height;
  return Rect(x_start, y_start, x_end, y_end);
}

void ScaledPixelBuffer::calculateScaledBufferSize() {
  scaled_width  = (int)ceil(src_width  * scale_ratio);
  scaled_height = (int)ceil(src_height * scale_ratio);
}
