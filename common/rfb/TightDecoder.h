/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
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
#ifndef __RFB_TIGHTDECODER_H__
#define __RFB_TIGHTDECODER_H__

#include <rdr/ZlibInStream.h>
#include <rfb/Decoder.h>
#include <rfb/JpegDecompressor.h>

namespace rfb {

  class TightDecoder : public Decoder {

  public:
    TightDecoder();
    virtual ~TightDecoder();
    bool readRect(const Rect& r, rdr::InStream* is,
                  const ServerParams& server,
                  rdr::OutStream* os) override;
    bool doRectsConflict(const Rect& rectA,
                         const uint8_t* bufferA, size_t buflenA,
                         const Rect& rectB,
                         const uint8_t* bufferB, size_t buflenB,
                         const ServerParams& server) override;
    void decodeRect(const Rect& r, const uint8_t* buffer,
                    size_t buflen, const ServerParams& server,
                    ModifiablePixelBuffer* pb) override;

  private:
    uint32_t readCompact(rdr::InStream* is);

    void FilterGradient24(const uint8_t* inbuf, const PixelFormat& pf,
                          uint32_t* outbuf, int stride, const Rect& r);

    template<class T>
    void FilterGradient(const uint8_t* inbuf, const PixelFormat& pf,
                        T* outbuf, int stride, const Rect& r);

    template<class T>
    void FilterPalette(const T* palette, int palSize,
                       const uint8_t* inbuf, T* outbuf,
                       int stride, const Rect& r);

  private:
    rdr::ZlibInStream zis[4];
  };
}

#endif
