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

namespace rdr {
  class InStream;
  class OutStream;
}

namespace rfb {
  class ServerParams;
  class ModifiablePixelBuffer;
  class Region;

  struct Rect;

  enum DecoderFlags {
    // A constant for decoders that don't need anything special
    DecoderPlain = 0,
    // All rects for this decoder must be handled in order
    DecoderOrdered = 1 << 0,
    // Only some of the rects must be handled in order,
    // see doesRectsConflict()
    DecoderPartiallyOrdered = 1 << 1,
  };

  class Decoder {
  public:
    Decoder(enum DecoderFlags flags);
    virtual ~Decoder();

    // These functions are the main interface to an individual decoder

    // readRect() transfers data for the given rectangle from the
    // InStream to the OutStream, possibly changing it along the way to
    // make it easier to decode. This function will always be called in
    // a serial manner on the main thread.
    virtual bool readRect(const Rect& r, rdr::InStream* is,
                          const ServerParams& server, rdr::OutStream* os)=0;

    // These functions will be called from any of the worker threads.
    // A lock will be held whilst these are called so it is safe to
    // read and update internal state as necessary.

    // getAffectedRegion() returns the parts of the frame buffer will
    // be either read from or written do when decoding this rect. The
    // default implementation simply returns the given rectangle.
    virtual void getAffectedRegion(const Rect& rect, const void* buffer,
                                   size_t buflen, const ServerParams& server,
                                   Region* region);

    // doesRectsConflict() determines if two rectangles must be decoded
    // in the order they were received. This will only be called if the
    // DecoderPartiallyOrdered flag has been set.
    virtual bool doRectsConflict(const Rect& rectA,
                                 const void* bufferA,
                                 size_t buflenA,
                                 const Rect& rectB,
                                 const void* bufferB,
                                 size_t buflenB,
                                 const ServerParams& server);

    // decodeRect() decodes the given rectangle with data from the
    // given buffer, onto the ModifiablePixelBuffer. The PixelFormat of
    // the PixelBuffer might not match the ConnParams and it is up to
    // the decoder to do any necessary conversion.
    virtual void decodeRect(const Rect& r, const void* buffer,
                            size_t buflen, const ServerParams& server,
                            ModifiablePixelBuffer* pb)=0;

  public:
    static bool supported(int encoding);
    static Decoder* createDecoder(int encoding);

  public:
    const enum DecoderFlags flags;
  };
}

#endif
