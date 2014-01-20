/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __RFB_PIXELTRANSFORMER_H__
#define __RFB_PIXELTRANSFORMER_H__

#include <stdlib.h>
#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>

namespace rfb {
  typedef void (*transFnType)(void* table_,
                              const PixelFormat& inPF, const void* inPtr,
                              int inStride,
                              const PixelFormat& outPF, void* outPtr,
                              int outStride, int width, int height);

  class SMsgWriter;
  class PixelBuffer;

  class PixelTransformer {
  public:

    PixelTransformer(bool econ=false);
    virtual ~PixelTransformer();

    // init() is called to initialise the translation tables.  The inPF and
    // inCM arguments give the source format details, outPF gives the
    // target pixel format.

    void init(const PixelFormat& inPF, const PixelFormat& outPF);

    const PixelFormat &getInPF() const;
    const PixelFormat &getOutPF() const;

    // translatePixels() translates the given number of pixels from inPtr,
    // putting it into the buffer pointed to by outPtr.  The pixels at inPtr
    // should be in the format given by inPF to init(), and the translated
    // pixels will be in the format given by the outPF argument to init().
    void translatePixels(const void* inPtr, void* outPtr, int nPixels) const;

    // Similar to translatePixels() but handles an arbitrary region of
    // two pixel buffers.
    void translateRect(const void* inPtr, int inStride, Rect inRect,
                       void* outPtr, int outStride, Point outCoord) const;

    bool willTransform(void);

  private:
    bool economic;

    PixelFormat inPF;
    PixelFormat outPF;

    rdr::U8* table;
    transFnType transFn;
  };
}
#endif
