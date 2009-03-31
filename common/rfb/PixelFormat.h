/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
// PixelFormat - structure to represent a pixel format.  Also has useful
// methods for reading & writing to streams, etc.
//

#ifndef __RFB_PIXELFORMAT_H__
#define __RFB_PIXELFORMAT_H__

#include <rfb/Pixel.h>
#include <rfb/ColourMap.h>

namespace rdr { class InStream; class OutStream; }

namespace rfb {

  class PixelFormat {
  public:
    PixelFormat(int b, int d, bool e, bool t,
                int rm=0, int gm=0, int bm=0, int rs=0, int gs=0, int bs=0);
    PixelFormat();

    bool equal(const PixelFormat& other) const;

    void read(rdr::InStream* is);
    void write(rdr::OutStream* os) const;

    bool is888(void) const;
    bool isBigEndian(void) const;
    bool isLittleEndian(void) const;

    inline Pixel pixelFromBuffer(const rdr::U8* buffer) const;
    inline void bufferFromPixel(rdr::U8* buffer, Pixel pixel) const;

    Pixel pixelFromRGB(rdr::U16 red, rdr::U16 green, rdr::U16 blue, ColourMap* cm=0) const;
    Pixel pixelFromRGB(rdr::U8 red, rdr::U8 green, rdr::U8 blue, ColourMap* cm=0) const;

    void bufferFromRGB(rdr::U8 *dst, const rdr::U8* src, int pixels, ColourMap* cm=0) const;

    void rgbFromPixel(Pixel pix, ColourMap* cm, Colour* rgb) const;
    inline void rgbFromPixel(Pixel pix, ColourMap* cm, rdr::U16 *r, rdr::U16 *g, rdr::U16 *b) const;
    inline void rgbFromPixel(Pixel pix, ColourMap* cm, rdr::U8 *r, rdr::U8 *g, rdr::U8 *b) const;

    void rgbFromBuffer(rdr::U16* dst, const rdr::U8* src, int pixels, ColourMap* cm=0) const;
    void rgbFromBuffer(rdr::U8* dst, const rdr::U8* src, int pixels, ColourMap* cm=0) const;

    void print(char* str, int len) const;
    bool parse(const char* str);

  protected:
    void updateShifts(void);

  public:
    int bpp;
    int depth;
    bool trueColour;

  // FIXME: These should be protected, but we need to fix TransImageGetter first.
  public:
    bool bigEndian;
    int redMax;
    int greenMax;
    int blueMax;
    int redShift;
    int greenShift;
    int blueShift;

  protected:
    int redConvShift;
    int greenConvShift;
    int blueConvShift;
  };
}

#include <rfb/PixelFormat.inl>

#endif
