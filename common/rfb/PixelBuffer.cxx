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

// -=- PixelBuffer.cxx
//
// The PixelBuffer class encapsulates the PixelFormat and dimensions
// of a block of pixel data.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/PixelBuffer.h>

using namespace rfb;
using namespace rdr;

static LogWriter vlog("PixelBuffer");

// We do a lot of byte offset calculations that assume the result fits
// inside a signed 32 bit integer. Limit the maximum size of pixel
// buffers so that these calculations never overflow.

const int maxPixelBufferWidth = 16384;
const int maxPixelBufferHeight = 16384;
const int maxPixelBufferStride = 16384;


// -=- Generic pixel buffer class

PixelBuffer::PixelBuffer(const PixelFormat& pf, int w, int h)
  : format(pf), width_(0), height_(0)
{
  setSize(w, h);
}

PixelBuffer::PixelBuffer() : width_(0), height_(0)
{
}

PixelBuffer::~PixelBuffer() {}


void
PixelBuffer::getImage(void* imageBuf, const Rect& r, int outStride) const
{
  int inStride;
  const U8* data;
  int bytesPerPixel, inBytesPerRow, outBytesPerRow, bytesPerMemCpy;
  U8* imageBufPos;
  const U8* end;

  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(),
                         r.tl.x, r.tl.y, width(), height());

  data = getBuffer(r, &inStride);

  bytesPerPixel = format.bpp/8;
  inBytesPerRow = inStride * bytesPerPixel;

  if (!outStride)
    outStride = r.width();
  outBytesPerRow = outStride * bytesPerPixel;
  bytesPerMemCpy = r.width() * bytesPerPixel;

  imageBufPos = (U8*)imageBuf;
  end = data + (inBytesPerRow * r.height());

  while (data < end) {
    memcpy(imageBufPos, data, bytesPerMemCpy);
    imageBufPos += outBytesPerRow;
    data += inBytesPerRow;
  }
}

void PixelBuffer::getImage(const PixelFormat& pf, void* imageBuf,
                           const Rect& r, int stride) const
{
  const rdr::U8* srcBuffer;
  int srcStride;

  if (format.equal(pf)) {
    getImage(imageBuf, r, stride);
    return;
  }

  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(),
                         r.tl.x, r.tl.y, width(), height());

  if (stride == 0)
    stride = r.width();

  srcBuffer = getBuffer(r, &srcStride);

  pf.bufferFromBuffer((U8*)imageBuf, format, srcBuffer, r.width(), r.height(),
                      stride, srcStride);
}

void PixelBuffer::setSize(int width, int height)
{
  if ((width < 0) || (width > maxPixelBufferWidth))
    throw rfb::Exception("Invalid PixelBuffer width of %d pixels requested", width);
  if ((height < 0) || (height > maxPixelBufferHeight))
    throw rfb::Exception("Invalid PixelBuffer height of %d pixels requested", height);

  width_ = width;
  height_ = height;
}

// -=- Modifiable generic pixel buffer class

ModifiablePixelBuffer::ModifiablePixelBuffer(const PixelFormat& pf,
                                             int w, int h)
  : PixelBuffer(pf, w, h)
{
}

ModifiablePixelBuffer::ModifiablePixelBuffer()
{
}

ModifiablePixelBuffer::~ModifiablePixelBuffer()
{
}

void ModifiablePixelBuffer::fillRect(const Rect& r, const void* pix)
{
  int stride;
  U8 *buf;
  int w, h, b;

  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(), r.tl.x, r.tl.y, width(), height());

  w = r.width();
  h = r.height();
  b = format.bpp/8;

  if (h == 0)
    return;

  buf = getBufferRW(r, &stride);

  if (b == 1) {
    while (h--) {
      memset(buf, *(const U8*)pix, w);
      buf += stride * b;
    }
  } else {
    U8 *start;
    int w1;

    start = buf;

    w1 = w;
    while (w1--) {
      memcpy(buf, pix, b);
      buf += b;
    }
    buf += (stride - w) * b;
    h--;

    while (h--) {
      memcpy(buf, start, w * b);
      buf += stride * b;
    }
  }

  commitBufferRW(r);
}

void ModifiablePixelBuffer::imageRect(const Rect& r,
                                      const void* pixels, int srcStride)
{
  U8* dest;
  int destStride;
  int bytesPerPixel, bytesPerDestRow, bytesPerSrcRow, bytesPerFill;
  const U8* src;
  U8* end;

  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(),
                         r.tl.x, r.tl.y, width(), height());

  bytesPerPixel = getPF().bpp/8;

  dest = getBufferRW(r, &destStride);

  bytesPerDestRow = bytesPerPixel * destStride;

  if (!srcStride)
    srcStride = r.width();
  bytesPerSrcRow = bytesPerPixel * srcStride;
  bytesPerFill = bytesPerPixel * r.width();

  src = (const U8*)pixels;
  end = dest + (bytesPerDestRow * r.height());

  while (dest < end) {
    memcpy(dest, src, bytesPerFill);
    dest += bytesPerDestRow;
    src += bytesPerSrcRow;
  }

  commitBufferRW(r);
}

void ModifiablePixelBuffer::copyRect(const Rect &rect,
                                     const Point &move_by_delta)
{
  int srcStride, dstStride;
  int bytesPerPixel;
  const U8* srcData;
  U8* dstData;

  Rect drect, srect;

  drect = rect;
  if (!drect.enclosed_by(getRect()))
    throw rfb::Exception("Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         drect.width(), drect.height(),
                         drect.tl.x, drect.tl.y, width(), height());

  srect = drect.translate(move_by_delta.negate());
  if (!srect.enclosed_by(getRect()))
    throw rfb::Exception("Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         srect.width(), srect.height(),
                         srect.tl.x, srect.tl.y, width(), height());

  bytesPerPixel = format.bpp/8;

  srcData = getBuffer(srect, &srcStride);
  dstData = getBufferRW(drect, &dstStride);

  if (move_by_delta.y == 0) {
    // Possible overlap. Be careful and use memmove().
    int h = drect.height();
    while (h--) {
      memmove(dstData, srcData, drect.width() * bytesPerPixel);
      dstData += dstStride * bytesPerPixel;
      srcData += srcStride * bytesPerPixel;
    }
  } else if (move_by_delta.y < 0) {
    // The data shifted upwards. Copy from top to bottom.
    int h = drect.height();
    while (h--) {
      memcpy(dstData, srcData, drect.width() * bytesPerPixel);
      dstData += dstStride * bytesPerPixel;
      srcData += srcStride * bytesPerPixel;
    }
  } else {
    // The data shifted downwards. Copy from bottom to top.
    int h = drect.height();
    dstData += (h-1) * dstStride * bytesPerPixel;
    srcData += (h-1) * srcStride * bytesPerPixel;
    while (h--) {
      memcpy(dstData, srcData, drect.width() * bytesPerPixel);
      dstData -= dstStride * bytesPerPixel;
      srcData -= srcStride * bytesPerPixel;
    }
  }

  commitBufferRW(drect);
}

void ModifiablePixelBuffer::fillRect(const PixelFormat& pf, const Rect &dest,
                                     const void* pix)
{
  rdr::U8 buf[4];
  format.bufferFromBuffer(buf, pf, (const rdr::U8*)pix, 1);
  fillRect(dest, buf);
}

void ModifiablePixelBuffer::imageRect(const PixelFormat& pf, const Rect &dest,
                                      const void* pixels, int stride)
{
  rdr::U8* dstBuffer;
  int dstStride;

  if (!dest.enclosed_by(getRect()))
    throw rfb::Exception("Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
                         dest.width(), dest.height(),
                         dest.tl.x, dest.tl.y, width(), height());

  if (stride == 0)
    stride = dest.width();

  dstBuffer = getBufferRW(dest, &dstStride);
  format.bufferFromBuffer(dstBuffer, pf, (const rdr::U8*)pixels,
                          dest.width(), dest.height(),
                          dstStride, stride);
  commitBufferRW(dest);
}

// -=- Simple pixel buffer with a continuous block of memory

FullFramePixelBuffer::FullFramePixelBuffer(const PixelFormat& pf, int w, int h,
                                           rdr::U8* data_, int stride_)
  : ModifiablePixelBuffer(pf, w, h), data(data_), stride(stride_)
{
}

FullFramePixelBuffer::FullFramePixelBuffer() : data(0) {}

FullFramePixelBuffer::~FullFramePixelBuffer() {}

rdr::U8* FullFramePixelBuffer::getBufferRW(const Rect& r, int* stride_)
{
  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Pixel buffer request %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(),
                         r.tl.x, r.tl.y, width(), height());

  *stride_ = stride;
  return &data[(r.tl.x + (r.tl.y * stride)) * (format.bpp/8)];
}

void FullFramePixelBuffer::commitBufferRW(const Rect& r)
{
}

const rdr::U8* FullFramePixelBuffer::getBuffer(const Rect& r, int* stride_) const
{
  if (!r.enclosed_by(getRect()))
    throw rfb::Exception("Pixel buffer request %dx%d at %d,%d exceeds framebuffer %dx%d",
                         r.width(), r.height(),
                         r.tl.x, r.tl.y, width(), height());

  *stride_ = stride;
  return &data[(r.tl.x + (r.tl.y * stride)) * (format.bpp/8)];
}

void FullFramePixelBuffer::setBuffer(int width, int height,
                                     rdr::U8* data_, int stride_)
{
  if ((width < 0) || (width > maxPixelBufferWidth))
    throw rfb::Exception("Invalid PixelBuffer width of %d pixels requested", width);
  if ((height < 0) || (height > maxPixelBufferHeight))
    throw rfb::Exception("Invalid PixelBuffer height of %d pixels requested", height);
  if ((stride_ < 0) || (stride_ > maxPixelBufferStride) || (stride_ < width))
    throw rfb::Exception("Invalid PixelBuffer stride of %d pixels requested", stride_);
  if ((width != 0) && (height != 0) && (data_ == NULL))
    throw rfb::Exception("PixelBuffer requested without a valid memory area");

  ModifiablePixelBuffer::setSize(width, height);
  stride = stride_;
  data = data_;
}

void FullFramePixelBuffer::setSize(int w, int h)
{
  // setBuffer() should be used
  throw rfb::Exception("Invalid call to FullFramePixelBuffer::setSize()");
}

// -=- Managed pixel buffer class
// Automatically allocates enough space for the specified format & area

ManagedPixelBuffer::ManagedPixelBuffer()
  : data_(NULL), datasize(0)
{
}

ManagedPixelBuffer::ManagedPixelBuffer(const PixelFormat& pf, int w, int h)
  : FullFramePixelBuffer(pf, 0, 0, NULL, 0), data_(NULL), datasize(0)
{
  setSize(w, h);
}

ManagedPixelBuffer::~ManagedPixelBuffer()
{
  if (data_)
    delete [] data_;
}

void ManagedPixelBuffer::setPF(const PixelFormat &pf)
{
  format = pf;
  setSize(width(), height());
}

void ManagedPixelBuffer::setSize(int w, int h)
{
  unsigned long new_datasize = w * h * (format.bpp/8);

  new_datasize = w * h * (format.bpp/8);
  if (datasize < new_datasize) {
    if (data_) {
      delete [] data_;
      data_ = NULL;
      datasize = 0;
    }
    if (new_datasize) {
      data_ = new U8[new_datasize];
      datasize = new_datasize;
    }
  }

  setBuffer(w, h, data_, w);
}
