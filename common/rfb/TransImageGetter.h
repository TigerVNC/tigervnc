/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
//
// TransImageGetter - class to perform translation between pixel formats,
// implementing the ImageGetter interface.
//

#ifndef __RFB_TRANSIMAGEGETTER_H__
#define __RFB_TRANSIMAGEGETTER_H__

#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>
#include <rfb/PixelTransformer.h>
#include <rfb/ImageGetter.h>

namespace rfb {

  class SMsgWriter;
  class PixelBuffer;

  class TransImageGetter : public ImageGetter,
                           public PixelTransformer {
  public:

    TransImageGetter(bool econ=false);
    virtual ~TransImageGetter();

    // init() is called to initialise the translation tables.  The PixelBuffer
    // argument gives the source data and format details, outPF gives the
    // client's pixel format.

    void init(PixelBuffer* pb, const PixelFormat& outPF, SMsgWriter* writer=0);

    // getImage() gets the given rectangle of data from the PixelBuffer,
    // translates it into the client's pixel format and puts it in the buffer
    // pointed to by the outPtr argument.  The optional outStride argument can
    // be used where padding is required between the output scanlines (the
    // padding will be outStride-r.width() pixels).
    void getImage(void* outPtr, const Rect& r, int outStride=0);

    // getRawBufferR() gets the given rectangle of data directly from the
    // underlying PixelBuffer, bypassing the translation logic. Only use
    // this when doing something that's independent of the client's pixel
    // format.
    const rdr::U8 *getRawBufferR(const Rect &r, int *stride);

    // setPixelBuffer() changes the pixel buffer to be used.  The new pixel
    // buffer MUST have the same pixel format as the old one - if not you
    // should call init() instead.
    void setPixelBuffer(PixelBuffer* pb_) { pb = pb_; }

    PixelBuffer *getPixelBuffer(void) { return pb; }

    // setOffset() sets an offset which is subtracted from the coordinates of
    // the rectangle given to getImage().
    void setOffset(const Point& offset_) { offset = offset_; }

  private:
    bool economic;
    PixelBuffer* pb;
    PixelFormat outPF;
    SMsgWriter* writer;
    rdr::U8* table;
    transFnType transFn;
    Point offset;
  };
}
#endif
