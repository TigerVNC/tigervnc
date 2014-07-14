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

#include <rfb/Rect.h>

namespace rfb {
  class SConnection;
  class PixelBuffer;

  class Encoder {
  public:
    Encoder(SConnection* conn);
    virtual ~Encoder();

    virtual void setCompressLevel(int level) {};
    virtual void setQualityLevel(int level) {};
    virtual void setFineQualityLevel(int quality, int subsampling) {};
    virtual int getNumRects(const Rect &r) { return 1; }

    // writeRect() is the main interface that encodes the given rectangle
    // with data from the PixelBuffer onto the SConnection given at
    // encoder creation. The PixelFormat of the PixelBuffer might not
    // match the ConnParams and it is up ot the encoder to do
    // any necessary conversion.
    virtual void writeRect(const Rect& r, PixelBuffer* pb)=0;

    static bool supported(int encoding);
    static Encoder* createEncoder(int encoding, SConnection* conn);

  protected:
    SConnection* conn;
  };
}

#endif
