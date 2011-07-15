/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>

namespace rfb {
  typedef void (*transFnType)(void* table_,
                              const PixelFormat& inPF, void* inPtr,
                              int inStride,
                              const PixelFormat& outPF, void* outPtr,
                              int outStride, int width, int height);

  class SMsgWriter;
  class ColourMap;
  class PixelBuffer;
  class ColourCube;

  typedef void (*setCMFnType)(int firstColour, int nColours, ColourMap* cm, void* data);

  class PixelTransformer {
  public:

    PixelTransformer(bool econ=false);
    virtual ~PixelTransformer();

    // init() is called to initialise the translation tables.  The inPF and
    // inCM arguments give the source format details, outPF gives the
    // target pixel format.  If the target has a colour map, then the you
    // must specify either a colour map callback or a colour cube to indicate
    // how the target colour map should be handled. If both are specified
    // then the cube will be used.

    void init(const PixelFormat& inPF, ColourMap* inCM,
              const PixelFormat& outPF, ColourCube* cube = NULL,
              setCMFnType cmCallback = NULL, void *cbData = NULL);

    const PixelFormat &getInPF() const;
    const ColourMap *getInColourMap() const;

    const PixelFormat &getOutPF() const;
    const ColourCube *getOutColourCube() const;

    // setColourMapEntries() is called when the colour map specified to init()
    // has changed.  firstColour and nColours specify which part of the
    // colour map has changed.  If nColours is 0, this means the rest of the
    // colour map. If the target also has a colour map, then the callback or
    // cube specified to init() will be used. If the target is true colour
    // then instead we update the internal translation table - in this case
    // the caller should also make sure that the target surface receives an
    // update of the relevant parts (the simplest thing to do is just update
    // the whole framebuffer, though it is possible to be smarter than this).

    void setColourMapEntries(int firstColour, int nColours);

    // translatePixels() translates the given number of pixels from inPtr,
    // putting it into the buffer pointed to by outPtr.  The pixels at inPtr
    // should be in the format given by inPF to init(), and the translated
    // pixels will be in the format given by the outPF argument to init().
    void translatePixels(void* inPtr, void* outPtr, int nPixels) const;

    // Similar to translatePixels() but handles an arbitrary region of
    // two pixel buffers.
    void translateRect(void* inPtr, int inStride, Rect inRect,
                       void* outPtr, int outStride, Point outCoord) const;

  private:
    bool economic;

    PixelFormat inPF;
    ColourMap* inCM;

    PixelFormat outPF;
    setCMFnType cmCallback;
    void *cbData;
    ColourCube* cube;

    rdr::U8* table;
    transFnType transFn;
  };
}
#endif
