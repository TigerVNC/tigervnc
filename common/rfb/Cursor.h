/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2023 Pierre Ossman for Cendio AB
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
//
// Cursor - structure containing information describing
//          the current cursor shape
//

#ifndef __RFB_CURSOR_H__
#define __RFB_CURSOR_H__

#include <vector>

#include <rfb/PixelBuffer.h>

namespace rfb {

  class Cursor {
  public:
    Cursor(int width, int height, const Point& hotspot, const uint8_t* data);
    Cursor(const Cursor& other);
    ~Cursor();

    int width() const { return width_; };
    int height() const { return height_; };
    const Point& hotspot() const { return hotspot_; };
    const uint8_t* getBuffer() const { return data; };

    // getBitmap() returns a monochrome version of the cursor
    std::vector<uint8_t> getBitmap() const;
    // getMask() returns a simple mask version of the alpha channel
    std::vector<uint8_t> getMask() const;

    // crop() crops the cursor down to the smallest possible size, based on the
    // mask.
    void crop();

  protected:
    int width_, height_;
    Point hotspot_;
    uint8_t* data;
  };

  class RenderedCursor : public PixelBuffer {
  public:
    RenderedCursor();

    Rect getEffectiveRect() const { return buffer.getRect(offset); }

    const uint8_t* getBuffer(const Rect& r, int* stride) const override;

    void update(PixelBuffer* framebuffer, Cursor* cursor, const Point& pos);

  protected:
    ManagedPixelBuffer buffer;
    Point offset;
  };

}
#endif
