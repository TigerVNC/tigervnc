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

ScaledDIBSectionBuffer::ScaledDIBSectionBuffer(HWND window) 
  : src_buffer(0), scaling(false), DIBSectionBuffer(window) {
  scaled_data = data;
}

ScaledDIBSectionBuffer::~ScaledDIBSectionBuffer() {
  if (src_buffer) delete src_buffer;
}

void ScaledDIBSectionBuffer::setScaleRatio(double scale_ratio_) {
  if (scale_ratio == scale_ratio_) return;

  if (format.depth != 24) throw rfb::UnsupportedPixelFormatException();

  if (scale_ratio_ != 1) {
    scaling = true;
    if (!src_buffer) {
      src_buffer = new ManagedPixelBuffer(format, src_width, src_height);
      src_data = &(src_buffer->data);
      memcpy(src_buffer->data, data, area() * (getPF().bpp/8));
    }
  } else {
    scaling = false;
  }
  ScaledPixelBuffer::setScaleRatio(scale_ratio_);
  recreateScaledBuffer();
  if (scaling) {
    scaleRect(Rect(0, 0, src_width, src_height));
  } else {
    memcpy(data, src_buffer->data, src_buffer->area() * (src_buffer->getPF().bpp/8));
    if (src_buffer) {
      delete src_buffer;
      src_buffer = 0;
      src_data = 0;
    }
  }
}

void ScaledDIBSectionBuffer::setPF(const PixelFormat &pf_) {
  if (scaling) {
    ScaledPixelBuffer::setPF(pf_);
    src_buffer->setPF(pf_);
  }
  DIBSectionBuffer::setPF(pf_);
  scaled_data = data;
}

void ScaledDIBSectionBuffer::setSize(int src_width_, int src_height_) {
  src_width = src_width_;
  src_height = src_height_;
  if (scaling) {
    src_buffer->setSize(src_width, src_height);
  }
  calculateScaledBufferSize();
  recreateScaledBuffer();
  scaled_data = data;
}

void ScaledDIBSectionBuffer::recreateScaledBuffer() {
  width_  = scaled_width;
  height_ = scaled_height;
  DIBSectionBuffer::recreateBuffer();
  scaled_data = data;
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
