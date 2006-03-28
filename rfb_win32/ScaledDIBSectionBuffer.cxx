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
  : DIBSectionBuffer(window) {
  scaled_data = data;
}

void ScaledDIBSectionBuffer::setSrcPixelBuffer(U8 **src_data_, int w, int h, PixelFormat pf) {
  src_data = src_data_;
  src_width  = w;
  src_height = h;
  scaled_width  = width_  = (int)ceil(src_width  * scale_ratio);
  scaled_height = height_ = (int)ceil(src_height * scale_ratio);
  setPF(pf); //recreateScaledBuffer() was run
}

void ScaledDIBSectionBuffer::setPF(const PixelFormat &pf) {
  DIBSectionBuffer::setPF(pf);
  scaled_data = data;
}

void ScaledDIBSectionBuffer::setSize(int src_width_, int src_height_) {
  src_width = src_width_;
  src_height = src_height_;
  setScale(scale_ratio * 100);
}

void ScaledDIBSectionBuffer::recreateScaledBuffer() {
  width_  = scaled_width;
  height_ = scaled_height;
  recreateBuffer();
  scaled_data = data;
}
