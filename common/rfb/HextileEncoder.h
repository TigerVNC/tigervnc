/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2022 Pierre Ossman for Cendio AB
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
#ifndef __RFB_HEXTILEENCODER_H__
#define __RFB_HEXTILEENCODER_H__

#include <rfb/Encoder.h>

namespace rfb {

  class HextileEncoder : public Encoder {
  public:
    HextileEncoder(SConnection* conn);
    virtual ~HextileEncoder();
    bool isSupported() override;
    void writeRect(const PixelBuffer* pb,
                   const Palette& palette) override;
    void writeSolidRect(int width, int height, const PixelFormat& pf,
                        const uint8_t* colour) override;
  private:
    template<class T>
    inline void writePixel(rdr::OutStream* os, T pixel);

    template<class T>
    void hextileEncode(rdr::OutStream* os, const PixelBuffer* pb);
    template<class T>
    int hextileEncodeTile(T* data, int w, int h, int tileType,
                          uint8_t* encoded, T bg);
    template<class T>
    int testTileType(T* data, int w, int h, T* bg, T* fg);

    template<class T>
    void hextileEncodeBetter(rdr::OutStream* os, const PixelBuffer* pb);
  };
}
#endif
