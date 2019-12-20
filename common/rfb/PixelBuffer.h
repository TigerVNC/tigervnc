/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

// -=- PixelBuffer.h
//
// The PixelBuffer class encapsulates the PixelFormat and dimensions
// of a block of pixel data.

#ifndef __RFB_PIXEL_BUFFER_H__
#define __RFB_PIXEL_BUFFER_H__

#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>
#include <rfb/Pixel.h>
#include <rfb/util.h>

namespace rfb {

  class Region;

  class PixelBuffer {
  public:
    PixelBuffer(const PixelFormat& pf, int width, int height);
    virtual ~PixelBuffer();

    ///////////////////////////////////////////////
    // Format / Layout
    //

  public:
    // Get pixel format
    const PixelFormat &getPF() const { return format; }

    // Get width, height and number of pixels
    int width()  const { return width_; }
    int height() const { return height_; }
    int area() const { return width_ * height_; }

    // Get rectangle encompassing this buffer
    //   Top-left of rectangle is either at (0,0), or the specified point.
    Rect getRect() const { return Rect(0, 0, width_, height_); }
    Rect getRect(const Point& pos) const {
      return Rect(pos, pos.translate(Point(width_, height_)));
    }

    ///////////////////////////////////////////////
    // Access to pixel data
    //

    // Get a pointer into the buffer
    //   The pointer is to the top-left pixel of the specified Rect.
    //   The buffer stride (in pixels) is returned.
    virtual const rdr::U8* getBuffer(const Rect& r, int* stride) const = 0;

    // Get pixel data for a given part of the buffer
    //   Data is copied into the supplied buffer, with the specified
    //   stride. Try to avoid using this though as getBuffer() will in
    //   most cases avoid the extra memory copy.
    void getImage(void* imageBuf, const Rect& r, int stride=0) const;
    // Get pixel data in a given format
    //   Works just the same as getImage(), but guaranteed to be in a
    //   specific format.
    void getImage(const PixelFormat& pf, void* imageBuf,
                  const Rect& r, int stride=0) const;

    ///////////////////////////////////////////////
    // Framebuffer update methods
    //

    // Ensure that the specified rectangle of buffer is up to date.
    //   Overridden by derived classes implementing framebuffer access
    //   to copy the required display data into place.
    virtual void grabRegion(const Region& __unused_attr region) {}

  protected:
    PixelBuffer();
    virtual void setSize(int width, int height);

  protected:
    PixelFormat format;

  private:
    int width_, height_;
  };

  // ModifiablePixelBuffer
  class ModifiablePixelBuffer : public PixelBuffer {
  public:
    ModifiablePixelBuffer(const PixelFormat& pf, int width, int height);
    virtual ~ModifiablePixelBuffer();

    ///////////////////////////////////////////////
    // Access to pixel data
    //

    // Get a writeable pointer into the buffer
    //   Like getBuffer(), the pointer is to the top-left pixel of the
    //   specified Rect and the stride in pixels is returned.
    virtual rdr::U8* getBufferRW(const Rect& r, int* stride) = 0;
    // Commit the modified contents
    //   Ensures that the changes to the specified Rect is properly
    //   stored away and any temporary buffers are freed. The Rect given
    //   here needs to match the Rect given to the earlier call to
    //   getBufferRW().
    virtual void commitBufferRW(const Rect& r) = 0;

    ///////////////////////////////////////////////
    // Basic rendering operations
    // These operations DO NOT clip to the pixelbuffer area, or trap overruns.

    // Fill a rectangle
    void fillRect(const Rect &dest, const void* pix);

    // Copy pixel data to the buffer
    void imageRect(const Rect &dest, const void* pixels, int stride=0);

    // Copy pixel data from one PixelBuffer location to another
    void copyRect(const Rect &dest, const Point& move_by_delta);

    // Render in a specific format
    //   Does the exact same thing as the above methods, but the given
    //   pixel values are defined by the given PixelFormat.
    void fillRect(const PixelFormat& pf, const Rect &dest, const void* pix);
    void imageRect(const PixelFormat& pf, const Rect &dest,
                   const void* pixels, int stride=0);

  protected:
    ModifiablePixelBuffer();
  };

  // FullFramePixelBuffer

  class FullFramePixelBuffer : public ModifiablePixelBuffer {
  public:
    FullFramePixelBuffer(const PixelFormat& pf, int width, int height,
                         rdr::U8* data_, int stride);
    virtual ~FullFramePixelBuffer();

  public:
    virtual const rdr::U8* getBuffer(const Rect& r, int* stride) const;
    virtual rdr::U8* getBufferRW(const Rect& r, int* stride);
    virtual void commitBufferRW(const Rect& r);

  protected:
    FullFramePixelBuffer();
    virtual void setBuffer(int width, int height, rdr::U8* data, int stride);

  private:
    virtual void setSize(int w, int h);

  private:
    rdr::U8* data;
    int stride;
  };

  // -=- Managed pixel buffer class
  // Automatically allocates enough space for the specified format & area

  class ManagedPixelBuffer : public FullFramePixelBuffer {
  public:
    ManagedPixelBuffer();
    ManagedPixelBuffer(const PixelFormat& pf, int width, int height);
    virtual ~ManagedPixelBuffer();

    // Manage the pixel buffer layout
    virtual void setPF(const PixelFormat &pf);
    virtual void setSize(int w, int h);

  private:
    rdr::U8* data_; // Mirrors FullFramePixelBuffer::data
    unsigned long datasize;
  };

};

#endif // __RFB_PIXEL_BUFFER_H__
