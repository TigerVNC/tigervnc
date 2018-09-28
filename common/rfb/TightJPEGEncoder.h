/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander
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
#ifndef __RFB_TIGHTJPEGENCODER_H__
#define __RFB_TIGHTJPEGENCODER_H__

#include <rfb/Encoder.h>
#include <rfb/JpegCompressor.h>

namespace rfb {

  class TightJPEGEncoder : public Encoder {
  public:
    TightJPEGEncoder(SConnection* conn);
    virtual ~TightJPEGEncoder();

    virtual bool isSupported();

    virtual void setQualityLevel(int level);
    virtual void setFineQualityLevel(int quality, int subsampling);

    virtual int getQualityLevel();

    virtual void writeRect(const PixelBuffer* pb, const Palette& palette);
    virtual void writeSolidRect(int width, int height,
                                const PixelFormat& pf,
                                const rdr::U8* colour);

  protected:
    void writeCompact(rdr::U32 value, rdr::OutStream* os);

  protected:
    JpegCompressor jc;

    int qualityLevel;
    int fineQuality;
    int fineSubsampling;
  };
}
#endif
