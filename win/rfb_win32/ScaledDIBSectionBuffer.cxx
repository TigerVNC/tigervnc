/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- ScaledDIBSectionBuffer.cxx

#include <math.h>

#include <rfb_win32/ScaledDIBSectionBuffer.h>

using namespace rfb;
using namespace win32;

const PixelFormat RGB24(32, 24, 0, 1, 255, 255, 255, 16, 8, 0);

ScaledDIBSectionBuffer::ScaledDIBSectionBuffer(HWND window) 
  : DIBSectionBuffer(window), src_buffer(0), scaling(false) {
  scaled_data = &data;
}

ScaledDIBSectionBuffer::~ScaledDIBSectionBuffer() {
  if (src_buffer) delete src_buffer;
}

void ScaledDIBSectionBuffer::setScale(int scale_) {
  if (scale == scale_ || scale_ <= 0) return;
  if (!(getPixelFormat().trueColour) && scale_ != 100) throw rfb::UnsupportedPixelFormatException();
  ScaledPixelBuffer::setScale(scale_);
  if (scale == 100) scaling = false;
  else scaling = true;
  recreateBuffers();
}

void ScaledDIBSectionBuffer::setPF(const PixelFormat &pf_) {
  if (memcmp(&(ScaledPixelBuffer::pf), &pf_, sizeof(pf_)) == 0) return;

  if (!pf_.trueColour && isScaling()) throw rfb::UnsupportedPixelFormatException();

  pf = pf_;
  if (scaling) {
    if (src_buffer) {
      src_buffer->setPF(pf);
    } else {
      src_buffer = new ManagedPixelBuffer(pf, src_width, src_height);
      src_data = &(src_buffer->data);
    }
    if (memcmp(&(DIBSectionBuffer::getPF()), &RGB24, sizeof(PixelFormat)) != 0) {
      DIBSectionBuffer::setPF(RGB24);
    }
  } else {
    DIBSectionBuffer::setPF(pf);
  }
}

void ScaledDIBSectionBuffer::setSize(int src_width_, int src_height_) {
  if (src_width == src_width_ && src_height == src_height_) return;
  src_width = src_width_;
  src_height = src_height_;
  
  // FIXME:
  // Calculate the scale weight tabs must be in the ScalePixelBuffer class
  recreateRowAccum();
  freeWeightTabs();
  calculateScaledBufferSize();
  scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, &xWeightTabs);
  scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, &yWeightTabs);

  recreateBuffers();
}

void ScaledDIBSectionBuffer::setScaleWindowSize(int width, int height) {
  if (scaled_width == width && scaled_height == height) return;

  freeWeightTabs();

  scaled_width = width_ = width;
  scaled_height = height_ = height;

  if (scaled_width == src_width && scaled_height == src_height) scaling = false;
  else scaling = true;
  scale_ratio_x = (double)scaled_width / src_width;
  scale_ratio_y = (double)scaled_height / src_height;
  scale = (int)(scale_ratio_x * 100);
  
  // FIXME:
  // Calculate the scale weight tabs must be in the ScalePixelBuffer class
  scaleFilters.makeWeightTabs(scaleFilterID, src_width, scaled_width, &xWeightTabs);
  scaleFilters.makeWeightTabs(scaleFilterID, src_height, scaled_height, &yWeightTabs);

  recreateBuffers();
}

void ScaledDIBSectionBuffer::recreateScaledBuffer() {
  if (scaling && memcmp(&(DIBSectionBuffer::getPF()), &RGB24, sizeof(PixelFormat)) != 0) {
    DIBSectionBuffer::setPF(RGB24);
  } else if (!scaling && (memcmp(&(DIBSectionBuffer::getPF()), &pf, sizeof(PixelFormat)) != 0)){
    DIBSectionBuffer::setPF(pf);
  } else {
    DIBSectionBuffer::recreateBuffer();
  }
}

void ScaledDIBSectionBuffer::recreateBuffers() {
  // Recreate the source pixel buffer
  if (src_width && src_height && pf.depth > 0) {
    if (scaling) {
      if (src_buffer) { 
        if (src_buffer->width() != src_width || src_buffer->width() != src_height)
          src_buffer->setSize(src_width, src_height);
      } else {
        src_buffer = new ManagedPixelBuffer(pf, src_width, src_height);
        src_data = &(src_buffer->data);
        if (data) memcpy(src_buffer->data, data, src_width * src_height * (getPF().bpp/8));
      }
    }
  }
  // Recreate the scaled pixel buffer
  recreateScaledBuffer();
  if (scaling && src_buffer && data) scaleRect(Rect(0, 0, src_width, src_height));
  else if (!scaling && src_buffer) {
    if (src_buffer->data && data) memcpy(data, src_buffer->data, src_buffer->area() * (getPF().bpp/8));
    delete src_buffer;
    src_buffer = 0;
    src_data = 0;
  }
}

void ScaledDIBSectionBuffer::calculateScaledBufferSize() {
  ScaledPixelBuffer::calculateScaledBufferSize();
  width_ = scaled_width;
  height_ = scaled_height;
}

void ScaledDIBSectionBuffer::fillRect(const Rect &dest, Pixel pix) {
  if (scaling) {
    src_buffer->fillRect(dest, pix);
    scaleRect(dest);
  } else {
    DIBSectionBuffer::fillRect(dest, pix);
  }
}

void ScaledDIBSectionBuffer::imageRect(const Rect &dest, const void* pixels, int stride) {
  if (scaling) {
    src_buffer->imageRect(dest, pixels, stride);
    scaleRect(dest);
  } else {
    DIBSectionBuffer::imageRect(dest, pixels, stride);
  }
}

void ScaledDIBSectionBuffer::copyRect(const Rect &dest, const Point &move_by_delta) {
  if (scaling) {
    src_buffer->copyRect(dest, move_by_delta);
    scaleRect(dest);
  } else {
    DIBSectionBuffer::copyRect(dest, move_by_delta);
  }
}
      
void ScaledDIBSectionBuffer::maskRect(const Rect& r, const void* pixels, const void* mask_) {
  if (scaling) {
    src_buffer->maskRect(r, pixels, mask_);
    scaleRect(r);
  } else {
    DIBSectionBuffer::maskRect(r, pixels, mask_);
  }
}

void ScaledDIBSectionBuffer::maskRect(const Rect& r, Pixel pixel, const void* mask_) {
  if (scaling) {
    src_buffer->maskRect(r, pixel, mask_);
    scaleRect(r);
  } else {
    DIBSectionBuffer::maskRect(r, pixel, mask_);
  }
}
