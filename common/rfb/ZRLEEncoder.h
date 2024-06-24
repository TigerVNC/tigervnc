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
#ifndef __RFB_ZRLEENCODER_H__
#define __RFB_ZRLEENCODER_H__

#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>
#include <rfb/Encoder.h>

namespace rfb {

  class ZRLEEncoder : public Encoder {
  public:
    ZRLEEncoder(SConnection* conn);
    virtual ~ZRLEEncoder();

    bool isSupported() override;

    void setCompressLevel(int level) override;

    void writeRect(const PixelBuffer* pb,
                   const Palette& palette) override;
    void writeSolidRect(int width, int height, const PixelFormat& pf,
                        const uint8_t* colour) override;

  protected:
    void writePaletteTile(const Rect& tile, const PixelBuffer* pb,
                          const Palette& palette);
    void writePaletteRLETile(const Rect& tile, const PixelBuffer* pb,
                             const Palette& palette);
    void writeRawTile(const Rect& tile, const PixelBuffer* pb);

    void writePalette(const PixelFormat& pf, const Palette& palette);

    void writePixels(const uint8_t* buffer, const PixelFormat& pf,
                     unsigned int count);

  protected:
    // Templated, optimised methods
    template<class T>
    void writePaletteTile(int width, int height,
                          const T* buffer, int stride,
                          const PixelFormat& pf, const Palette& palette);
    template<class T>
    void writePaletteRLETile(int width, int height,
                             const T* buffer, int stride,
                             const PixelFormat& pf, const Palette& palette);

  protected:
    rdr::ZlibOutStream zos;
    rdr::MemOutStream mos;
  };
}
#endif
