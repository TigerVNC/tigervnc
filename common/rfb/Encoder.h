/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#ifndef __RFB_ENCODER_H__
#define __RFB_ENCODER_H__

#include <rdr/types.h>
#include <rfb/Rect.h>

namespace rfb {
  class SConnection;
  class PixelBuffer;
  class Palette;
  class PixelFormat;

  enum EncoderFlags {
    // A constant for encoders that don't need anything special
    EncoderPlain = 0,
    // Give us the raw frame buffer, and not something converted to
    // the what the client is asking for.
    EncoderUseNativePF = 1 << 0,
    // Encoder does not encode pixels perfectly accurate
    EncoderLossy = 1 << 1,
  };

  class Encoder {
  public:
    Encoder(SConnection* conn, int encoding,
            enum EncoderFlags flags, unsigned int maxPaletteSize=-1,
            int losslessQuality=-1);
    virtual ~Encoder();

    // isSupported() should return a boolean indicating if this encoder
    // is okay to use with the current connection. This usually involves
    // checking the list of encodings in the connection parameters.
    virtual bool isSupported()=0;

    virtual void setCompressLevel(int level) {};
    virtual void setQualityLevel(int level) {};
    virtual void setFineQualityLevel(int quality, int subsampling) {};

    virtual int getCompressLevel() { return -1; };
    virtual int getQualityLevel() { return -1; };

    // writeRect() is the main interface that encodes the given rectangle
    // with data from the PixelBuffer onto the SConnection given at
    // encoder creation.
    //
    // The PixelBuffer will be in the PixelFormat specified in ConnParams
    // unless the flag UseNativePF is specified. In that case the
    // PixelBuffer will remain in its native format and encoder will have
    // to handle any conversion itself.
    //
    // The Palette will always be in the PixelFormat specified in
    // ConnParams. An empty palette indicates a large number of colours,
    // but could still be less than maxPaletteSize.
    virtual void writeRect(const PixelBuffer* pb, const Palette& palette)=0;

    // writeSolidRect() is a short cut in order to encode single colour
    // rectangles efficiently without having to create a fake single
    // colour PixelBuffer. The colour argument follows the same semantics
    // as the PixelBuffer for writeRect().
    //
    // Note that there is a default implementation that can be called
    // using Encoder::writeSolidRect() in the event that there is no
    // efficient short cut.
    virtual void writeSolidRect(int width, int height,
                                const PixelFormat& pf,
                                const rdr::U8* colour)=0;

  protected:
    // Helper method for redirecting a single colour palette to the
    // short cut method.
    void writeSolidRect(const PixelBuffer* pb, const Palette& palette);

  public:
    const int encoding;
    const enum EncoderFlags flags;

    // Maximum size of the palette per rect
    const unsigned int maxPaletteSize;

    // Minimum level where the quality loss will not be noticed by
    // most users (only relevant with EncoderLossy flag)
    const int losslessQuality;

  protected:
    SConnection* conn;
  };
}

#endif
