/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2016-2019 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

// -=- Generic pixel buffer class

package com.tigervnc.rfb;

import java.awt.*;
import java.awt.image.*;
import java.awt.Color;
import java.nio.*;
import java.util.concurrent.atomic.*;

public abstract class PixelBuffer {

  // We do a lot of byte offset calculations that assume the result fits
  // inside a signed 32 bit integer. Limit the maximum size of pixel
  // buffers so that these calculations never overflow.

  static final int maxPixelBufferWidth = 16384;
  static final int maxPixelBufferHeight = 16384;

  public PixelBuffer(PixelFormat pf, int w, int h) {
    format = pf;
    width_ = w;
    height_= h;
    setSize(w, h);
  }

  protected PixelBuffer() { width_ = 0; height_ = 0; }

  // Get pixel format
  public final PixelFormat getPF() { return format; }

  // Get width, height and number of pixels
  public final int width() { return width_; }
  public final int height() { return height_; }
  public final int area() { return width_ * height_; }

  // Get rectangle encompassing this buffer
  //   Top-left of rectangle is either at (0,0), or the specified point.
  public final Rect getRect() { return new Rect(0, 0, width_, height_); }
  public final Rect getRect(Point pos) {
    return new Rect(pos, pos.translate(new Point(width_, height_)));
  }

  ///////////////////////////////////////////////
  // Access to pixel data
  //

  // Get a pointer into the buffer
  //   The pointer is to the top-left pixel of the specified Rect.
  public abstract Raster getBuffer(Rect r);

  // Get pixel data for a given part of the buffer
  //   Data is copied into the supplied buffer, with the specified
  //   stride. Try to avoid using this though as getBuffer() will in
  //   most cases avoid the extra memory copy.
  //void getImage(void* imageBuf, const Rect& r, int stride=0) const;
  // Get pixel data in a given format
  //   Works just the same as getImage(), but guaranteed to be in a
  //   specific format.
  //void getImage(const PixelFormat& pf, void* imageBuf,
  //                const Rect& r, int stride=0) const;
  public Image getImage() { return image; }

  public void setSize(int width, int height)
  {
    if ((width < 0) || (width > maxPixelBufferWidth))
      throw new Exception("Invalid PixelBuffer width of "+width+" pixels requested");
    if ((height < 0) || (height > maxPixelBufferHeight))
      throw new Exception("Invalid PixelBuffer height of "+height+" pixels requested");

    width_ = width;
    height_ = height;
  }

  ///////////////////////////////////////////////
  // Framebuffer update methods
  //

  // Ensure that the specified rectangle of buffer is up to date.
  //   Overridden by derived classes implementing framebuffer access
  //   to copy the required display data into place.
  //public abstract void grabRegion(Region& region) {}

  protected PixelFormat format;
  protected int width_, height_;
  protected Image image;
}
