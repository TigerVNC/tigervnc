/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
    TightDecoder(CConnection* conn);
    virtual ~TightDecoder();
    virtual void readRect(const Rect& r, ModifiablePixelBuffer* pb);

  private:
    rdr::U32 readCompact(rdr::InStream* is);

    void tightDecode8(const Rect& r);
    void tightDecode16(const Rect& r);
    void tightDecode32(const Rect& r);

    void DecompressJpegRect8(const Rect& r);
    void DecompressJpegRect16(const Rect& r);
    void DecompressJpegRect32(const Rect& r);

    void FilterGradient8(rdr::U8 *netbuf, rdr::U8* buf, int stride, 
                         const Rect& r);
    void FilterGradient16(rdr::U8 *netbuf, rdr::U16* buf, int stride, 
                          const Rect& r);
    void FilterGradient24(rdr::U8 *netbuf, rdr::U32* buf, int stride, 
                          const Rect& r);
    void FilterGradient32(rdr::U8 *netbuf, rdr::U32* buf, int stride, 
                          const Rect& r);

    void directFillRect8(const Rect& r, Pixel pix);
    void directFillRect16(const Rect& r, Pixel pix);
    void directFillRect32(const Rect& r, Pixel pix);

    ModifiablePixelBuffer* pb;
    rdr::InStream* is;
    rdr::ZlibInStream zis[4];
    JpegDecompressor jd;
    PixelFormat clientpf;
    PixelFormat serverpf;
    bool directDecode;
  };
}

#endif
