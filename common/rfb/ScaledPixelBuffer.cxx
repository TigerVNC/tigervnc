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
                                     int src_height_, int scale_, PixelFormat pf_)
  : scale(scale_), scale_ratio_x(1), scale_ratio_y(1), scaleFilterID(scaleFilterBilinear),
    xWeightTabs(0), yWeightTabs(0), raccum(0), gaccum(0), baccum(0), scaled_data(0) {

  setSourceBuffer(src_data_, src_width_, src_height_);
  setPF(pf_);
}

ScaledPixelBuffer::ScaledPixelBuffer() 
  : src_width(0), src_height(0), scaled_width(0), scaled_height(0), scale(100), 
    scale_ratio_x(1), scale_ratio_y(1), scaleFilterID(scaleFilterBilinear),
    xWeightTabs(0), yWeightTabs(0), raccum(0), gaccum(0), baccum(0),
    src_data(0), scaled_data(0) {
  memset(&pf, 0, sizeof(pf));
}

ScaledPixelBuffer::~ScaledPixelBuffer() {
  freeWeightTabs();
  if (raccum) delete [] raccum;
  if (gaccum) delete [] gaccum;
  if (baccum) delete [] baccum;
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

void ScaledPixelBuffer::recreateRowAccum() {
  if (raccum) delete [] raccum;
  if (gaccum) delete [] gaccum;
  if (baccum) delete [] baccum;
  raccum = new int[src_width];
  gaccum = new int[src_width];
  baccum = new int[src_width];
}

void ScaledPixelBuffer::setSourceBuffer(U8 **src_data_, int w, int h) {
  if (w > 0 && h > 0 && src_data != NULL) {
    freeWeightTabs();
    src_data = src_data_;
    src_width  = w;
    src_height = h;
    recreateRowAccum();
    calculateScaledBufferSize();
    scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, &xWeightTabs);
    scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, &yWeightTabs);
  }
}

void ScaledPixelBuffer::setPF(const PixelFormat &pf_) {
  ///if (pf_.depth != 24) throw rfb::UnsupportedPixelFormatException();
  pf = pf_;
}

void ScaledPixelBuffer::setScale(int scale_) {
  if (scale != scale_ && scale_ > 0) {
    scale = scale_;
    freeWeightTabs();
    calculateScaledBufferSize();
    scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, &xWeightTabs);
    scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, &yWeightTabs);
  }
}

void ScaledPixelBuffer::setScaleFilter(unsigned int scaleFilterID_) {
  if (scaleFilterID == scaleFilterID_ || scaleFilterID_ > scaleFilterMaxNumber) return;

  scaleFilterID = scaleFilterID_;
  
  if (src_width && src_height && scaled_width && scaled_height) {
    freeWeightTabs();
    scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, &xWeightTabs);
    scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, &yWeightTabs);
    if (scale != 100 && pf.depth > 0 && scaled_data) scaleRect(Rect(0, 0, src_width, src_height));
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
  int r, g, b, red, green, blue;
  short *xweight, *yweight, weight;

  // Calculate the changed pixel rect in the scaled image
  changed_rect = calculateScaleBoundary(rect);

  int bytesPerSrcPixel = pf.bpp / 8;
  int bytesPerSrcRow = src_width * bytesPerSrcPixel;
  int bytesPerScaledRow = scaled_width * 4;

  int bytesPerAccumRow = src_width * sizeof(int);

  ptrs = &(*scaled_data)[(changed_rect.tl.x + changed_rect.tl.y*scaled_width) * 4];
  for (int y = changed_rect.tl.y; y < changed_rect.br.y; y++) {
    ptr = ptrs;
    yweight = yWeightTabs[y].weight;

    // Clear the color accumulators
    memset(raccum, 0, bytesPerAccumRow);
    memset(gaccum, 0, bytesPerAccumRow);
    memset(baccum, 0, bytesPerAccumRow);

    // Make the convolution the source image with scale filter weights
    // by y axis and save results to the color accumulators.
    pxs = &(*src_data)[(xWeightTabs[changed_rect.tl.x].i0 + yWeightTabs[y].i0*src_width) * bytesPerSrcPixel];
    for (int ys = yWeightTabs[y].i0; ys < yWeightTabs[y].i1; ys++) {
      px = pxs;
      for (int xs = xWeightTabs[changed_rect.tl.x].i0; xs < xWeightTabs[changed_rect.br.x-1].i1; xs++) {
        rgbFromPixel(*((U32*)px), r, g, b);
        weight = *yweight;
        raccum[xs] += (int)(weight) * r;
        gaccum[xs] += (int)(weight) * g;
        baccum[xs] += (int)(weight) * b;
        px += bytesPerSrcPixel;
      }
      yweight++;
      pxs += bytesPerSrcRow;
    }

    // Make the convolution the color accumulators with scale filter weights
    // by x axis and save results to the scaled image.
    for (int x = changed_rect.tl.x; x < changed_rect.br.x; x++) {
      // Init the sum of colors with (1 << (shift-1)) for rounding.
      red = green = blue = 1 << (FINALSHIFT-1);
      xweight = xWeightTabs[x].weight;
      for (int xs = xWeightTabs[x].i0; xs < xWeightTabs[x].i1; xs++) {
        weight = *xweight;
        red   += (int)(weight) * (raccum[xs] >> BITS_OF_CHANEL);
        green += (int)(weight) * (gaccum[xs] >> BITS_OF_CHANEL);
        blue  += (int)(weight) * (baccum[xs] >> BITS_OF_CHANEL);
        xweight++;
      }
      *ptr++ = U8(blue  >> FINALSHIFT);
      *ptr++ = U8(green >> FINALSHIFT);
      *ptr++ = U8(red   >> FINALSHIFT);
      ptr++;
    }
    ptrs += bytesPerScaledRow;
  }
}

Rect ScaledPixelBuffer::calculateScaleBoundary(const Rect& r) {
  int x_start, y_start, x_end, y_end;
  double translate_x = 0.5*scale_ratio_x - 0.5;
  double translate_y = 0.5*scale_ratio_y - 0.5;
  double sourceXScale  = __rfbmax(1.0, 1.0/scale_ratio_x);
  double sourceYScale  = __rfbmax(1.0, 1.0/scale_ratio_y);
  double sourceXRadius = __rfbmax(0.5, sourceXScale*scaleFilters[scaleFilterID].radius);
  double sourceYRadius = __rfbmax(0.5, sourceYScale*scaleFilters[scaleFilterID].radius);
  x_start = (int)ceil(scale_ratio_x*(r.tl.x-sourceXRadius) + translate_x + SCALE_ERROR);
  y_start = (int)ceil(scale_ratio_y*(r.tl.y-sourceYRadius) + translate_y + SCALE_ERROR);
  x_end   = (int)floor(scale_ratio_x*((r.br.x-1)+sourceXRadius) + translate_x - SCALE_ERROR) + 1;
  y_end   = (int)floor(scale_ratio_y*((r.br.y-1)+sourceXRadius) + translate_y - SCALE_ERROR) + 1;
  if (x_start < 0) x_start = 0;
  if (y_start < 0) y_start = 0;
  if (x_end > scaled_width) x_end = scaled_width;
  if (y_end > scaled_height) y_end = scaled_height;
  return Rect(x_start, y_start, x_end, y_end);
}

void ScaledPixelBuffer::calculateScaledBufferSize() {
  double scale_ratio = (double)scale / 100;
  scaled_width  = (int)ceil(src_width  * scale_ratio);
  scaled_height = (int)ceil(src_height * scale_ratio);
  scale_ratio_x = (double)scaled_width / src_width;
  scale_ratio_y = (double)scaled_height / src_height;
}
