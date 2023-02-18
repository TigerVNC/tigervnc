/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2022 Pierre Ossman for Cendio AB
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
// methods for reading & writing to streams, etc. Conversion to and from
// other formats are also handled by this class. We have three different
// representations that we refer to:
//
// a) Pixels - Unsigned native integers in the format specified by this
//             PixelFormat object.
// b) Buffer - Same thing as pixels, but in the appropriate byte stream
//             format. This involves endian conversion and padding.
// c) RGB - A byte stream of 8 bit red, green and blue elements, in that
//          order.
//

#ifndef __RFB_PIXELFORMAT_H__
#define __RFB_PIXELFORMAT_H__

#include <stdint.h>

namespace rdr { class InStream; class OutStream; }

namespace rfb {

  typedef uint32_t Pixel; // must be big enough to hold any pixel value

  class PixelFormat {
  public:
    PixelFormat(int b, int d, bool e, bool t,
                int rm, int gm, int bm, int rs, int gs, int bs);
    PixelFormat();

    // Checks if the formats have identical buffer representation.
    // They might still have different pixel representation, endianness
    // or true colour state.
    bool operator==(const PixelFormat& other) const;
    bool operator!=(const PixelFormat& other) const;

    void read(rdr::InStream* is);
    void write(rdr::OutStream* os) const;

    bool is888(void) const;
    bool isBigEndian(void) const;
    bool isLittleEndian(void) const;

    inline Pixel pixelFromBuffer(const uint8_t* buffer) const;
    inline void bufferFromPixel(uint8_t* buffer, Pixel pixel) const;

    inline Pixel pixelFromRGB(uint16_t red, uint16_t green, uint16_t blue) const;
    inline Pixel pixelFromRGB(uint8_t red, uint8_t green, uint8_t blue) const;

    void bufferFromRGB(uint8_t *dst, const uint8_t* src, int pixels) const;
    void bufferFromRGB(uint8_t *dst, const uint8_t* src,
                       int w, int stride, int h) const;

    inline void rgbFromPixel(Pixel pix, uint16_t *r, uint16_t *g, uint16_t *b) const;
    inline void rgbFromPixel(Pixel pix, uint8_t *r, uint8_t *g, uint8_t *b) const;

    void rgbFromBuffer(uint8_t* dst, const uint8_t* src, int pixels) const;
    void rgbFromBuffer(uint8_t* dst, const uint8_t* src,
                       int w, int stride, int h) const;

    Pixel pixelFromPixel(const PixelFormat &srcPF, Pixel src) const;

    void bufferFromBuffer(uint8_t* dst, const PixelFormat &srcPF,
                          const uint8_t* src, int pixels) const;
    void bufferFromBuffer(uint8_t* dst, const PixelFormat &srcPF,
                          const uint8_t* src, int w, int h,
                          int dstStride, int srcStride) const;

    void print(char* str, int len) const;
    bool parse(const char* str);

  protected:
    void updateState(void);
    bool isSane(void);

  private:
    // Templated, optimised methods
    template<class T>
    void directBufferFromBufferFrom888(T* dst, const PixelFormat &srcPF,
                                       const uint8_t* src, int w, int h,
                                       int dstStride, int srcStride) const;
    template<class T>
    void directBufferFromBufferTo888(uint8_t* dst, const PixelFormat &srcPF,
                                     const T* src, int w, int h,
                                     int dstStride, int srcStride) const;

  public:
    int bpp;
    int depth;

    // This only tracks if the client thinks it is in colour map mode.
    // In practice we are always in true colour mode.
    bool trueColour;

  protected:
    bool bigEndian;
    int redMax;
    int greenMax;
    int blueMax;
    int redShift;
    int greenShift;
    int blueShift;

  protected:
    /* Pre-computed values to keep algorithms simple */
    int redBits, greenBits, blueBits;
    int maxBits, minBits;
    bool endianMismatch;

    static uint8_t upconvTable[256*8];
    static uint8_t downconvTable[256*8];

    class Init;
    friend class Init;
    static Init _init;

    /* Only for testing this class */
    friend void makePixel(const rfb::PixelFormat &, uint8_t *);
    friend bool verifyPixel(const rfb::PixelFormat &,
                            const rfb::PixelFormat &,
                            const uint8_t *);
  };
}

#include <rfb/PixelFormat.inl>

#endif
