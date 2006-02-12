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

// -=- ScaledPixelBuffer.h
//
// The ScaledPixelBuffer class allows to scale the image data 
// from the source buffer to destination buffer using bilinear 
// interpolation.

#include <rdr/types.h>
#include <rfb/Rect.h>

using namespace rdr;

namespace rfb {

  class ScaledPixelBuffer {
  public:
    ScaledPixelBuffer(U8 *data, int width, int height, int scale);
    ScaledPixelBuffer();
    virtual ~ScaledPixelBuffer();

    // Get width, height, number of pixels and scale
    int width()  const { return width_; }
    int height() const { return height_; }
    int area() const { return width_ * height_; }
    int scale() const { return (int)(scale_ratio * 100); }

    // Get rectangle encompassing this buffer
    //   Top-left of rectangle is either at (0,0), or the specified point.
    Rect getRect() const { return Rect(0, 0, width_, height_); }
    Rect getRect(const Point& pos) const {
      return Rect(pos, pos.translate(Point(width_, height_)));
    }

    // Get the number of pixels per row in the actual pixel buffer data area
    virtual int getStride() const { return width(); } 

    // Get a pointer into the buffer
    //   The pointer is to the top-left pixel of the specified Rect.
    //   The buffer stride (in pixels) is returned.
    virtual const U8* getPixelsR(const Rect& r, int* stride);

    // Get pixel data for a given part of the buffer
    //   Data is copied into the supplied buffer, with the specified
    //   stride.
    virtual void getImage(void* imageBuf, const Rect& r, int stride=0);

    // Set the new scale, in percent
    virtual void setScale(int scale);

    // Scale rect from the source image buffer to the destination buffer
    // using bilinear interpolation
    virtual void scaleRect(const Rect& r);

  protected:
    int width_;
    int height_;
    int src_width;
    int src_height;
    int bpp;
    double scale_ratio;
    U8 *src_data;
    U8 *data;
  };

};
