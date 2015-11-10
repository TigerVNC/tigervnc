/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
#ifndef __RFB_DECODER_H__
#define __RFB_DECODER_H__

namespace rdr { class InStream; }

namespace rfb {
  class ConnParams;
  class ModifiablePixelBuffer;
  class Rect;

  class Decoder {
  public:
    Decoder();
    virtual ~Decoder();

    // readRect() is the main interface that decodes the given rectangle
    // with data from the given InStream, onto the ModifiablePixelBuffer.
    // The PixelFormat of the PixelBuffer might not match the ConnParams
    // and it is up to the decoder to do any necessary conversion.
    virtual void readRect(const Rect& r, rdr::InStream* is,
                          const ConnParams& cp, ModifiablePixelBuffer* pb)=0;

    static bool supported(int encoding);
    static Decoder* createDecoder(int encoding);
  };
}

#endif
