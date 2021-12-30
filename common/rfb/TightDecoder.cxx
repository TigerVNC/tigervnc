/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2009-2015 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <rdr/InStream.h>
#include <rdr/MemInStream.h>
#include <rdr/OutStream.h>

#include <rfb/ServerParams.h>
#include <rfb/Exception.h>
#include <rfb/PixelBuffer.h>
#include <rfb/TightConstants.h>
#include <rfb/TightDecoder.h>

using namespace rfb;

static const int TIGHT_MAX_WIDTH = 2048;
static const int TIGHT_MIN_TO_COMPRESS = 12;

#define BPP 8
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 16
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 32
#include <rfb/tightDecode.h>
#undef BPP

TightDecoder::TightDecoder() : Decoder(DecoderPartiallyOrdered)
{
}

TightDecoder::~TightDecoder()
{
}

bool TightDecoder::readRect(const Rect& r, rdr::InStream* is,
                            const ServerParams& server, rdr::OutStream* os)
{
  rdr::U8 comp_ctl;

  if (!is->hasData(1))
    return false;

  is->setRestorePoint();

  comp_ctl = is->readU8();
  os->writeU8(comp_ctl);

  comp_ctl >>= 4;

  // "Fill" compression type.
  if (comp_ctl == tightFill) {
    if (server.pf().is888()) {
      if (!is->hasDataOrRestore(3))
        return false;
      os->copyBytes(is, 3);
    } else {
      if (!is->hasDataOrRestore(server.pf().bpp/8))
        return false;
      os->copyBytes(is, server.pf().bpp/8);
    }
    is->clearRestorePoint();
    return true;
  }

  // "JPEG" compression type.
  if (comp_ctl == tightJpeg) {
    rdr::U32 len;

    // FIXME: Might be less than 3 bytes
    if (!is->hasDataOrRestore(3))
      return false;

    len = readCompact(is);
    os->writeOpaque32(len);

    if (!is->hasDataOrRestore(len))
      return false;

    os->copyBytes(is, len);

    is->clearRestorePoint();

    return true;
  }

  // Quit on unsupported compression type.
  if (comp_ctl > tightMaxSubencoding)
    throw Exception("TightDecoder: bad subencoding value received");

  // "Basic" compression type.

  int palSize = 0;

  if (r.width() > TIGHT_MAX_WIDTH)
    throw Exception("TightDecoder: too large rectangle (%d pixels)", r.width());

  // Possible palette
  if ((comp_ctl & tightExplicitFilter) != 0) {
    rdr::U8 filterId;

    if (!is->hasDataOrRestore(1))
      return false;

    filterId = is->readU8();
    os->writeU8(filterId);

    switch (filterId) {
    case tightFilterPalette:
      if (!is->hasDataOrRestore(1))
        return false;

      palSize = is->readU8() + 1;
      os->writeU8(palSize - 1);

      if (server.pf().is888()) {
        if (!is->hasDataOrRestore(palSize * 3))
          return false;
        os->copyBytes(is, palSize * 3);
      } else {
        if (!is->hasDataOrRestore(palSize * server.pf().bpp/8))
          return false;
        os->copyBytes(is, palSize * server.pf().bpp/8);
      }
      break;
    case tightFilterGradient:
      if (server.pf().bpp == 8)
        throw Exception("TightDecoder: invalid BPP for gradient filter");
      break;
    case tightFilterCopy:
      break;
    default:
      throw Exception("TightDecoder: unknown filter code received");
    }
  }

  size_t rowSize, dataSize;

  if (palSize != 0) {
    if (palSize <= 2)
      rowSize = (r.width() + 7) / 8;
    else
      rowSize = r.width();
  } else if (server.pf().is888()) {
    rowSize = r.width() * 3;
  } else {
    rowSize = r.width() * server.pf().bpp/8;
  }

  dataSize = r.height() * rowSize;

  if (dataSize < TIGHT_MIN_TO_COMPRESS) {
    if (!is->hasDataOrRestore(dataSize))
      return false;
    os->copyBytes(is, dataSize);
  } else {
    rdr::U32 len;

    // FIXME: Might be less than 3 bytes
    if (!is->hasDataOrRestore(3))
      return false;

    len = readCompact(is);
    os->writeOpaque32(len);

    if (!is->hasDataOrRestore(len))
      return false;

    os->copyBytes(is, len);
  }

  is->clearRestorePoint();

  return true;
}

bool TightDecoder::doRectsConflict(const Rect& rectA,
                                   const void* bufferA,
                                   size_t buflenA,
                                   const Rect& rectB,
                                   const void* bufferB,
                                   size_t buflenB,
                                   const ServerParams& server)
{
  rdr::U8 comp_ctl_a, comp_ctl_b;

  assert(buflenA >= 1);
  assert(buflenB >= 1);

  comp_ctl_a = *(const rdr::U8*)bufferA;
  comp_ctl_b = *(const rdr::U8*)bufferB;

  // Resets or use of zlib pose the same problem, so merge them
  if ((comp_ctl_a & 0x80) == 0x00)
    comp_ctl_a |= 1 << ((comp_ctl_a >> 4) & 0x03);
  if ((comp_ctl_b & 0x80) == 0x00)
    comp_ctl_b |= 1 << ((comp_ctl_b >> 4) & 0x03);

  if (((comp_ctl_a & 0x0f) & (comp_ctl_b & 0x0f)) != 0)
    return true;

  return false;
}

void TightDecoder::decodeRect(const Rect& r, const void* buffer,
                              size_t buflen, const ServerParams& server,
                              ModifiablePixelBuffer* pb)
{
  const rdr::U8* bufptr;
  const PixelFormat& pf = server.pf();

  rdr::U8 comp_ctl;

  bufptr = (const rdr::U8*)buffer;

  assert(buflen >= 1);

  comp_ctl = *bufptr;
  bufptr += 1;
  buflen -= 1;

  // Reset zlib streams if we are told by the server to do so.
  for (int i = 0; i < 4; i++) {
    if (comp_ctl & 1) {
      zis[i].reset();
    }
    comp_ctl >>= 1;
  }

  // "Fill" compression type.
  if (comp_ctl == tightFill) {
    if (pf.is888()) {
      rdr::U8 pix[4];

      assert(buflen >= 3);

      pf.bufferFromRGB(pix, bufptr, 1);
      pb->fillRect(pf, r, pix);
    } else {
      assert(buflen >= (size_t)pf.bpp/8);
      pb->fillRect(pf, r, bufptr);
    }
    return;
  }

  // "JPEG" compression type.
  if (comp_ctl == tightJpeg) {
    rdr::U32 len;

    int stride;
    rdr::U8 *buf;

    JpegDecompressor jd;

    assert(buflen >= 4);

    memcpy(&len, bufptr, 4);
    bufptr += 4;
    buflen -= 4;

    // We always use direct decoding with JPEG images
    buf = pb->getBufferRW(r, &stride);
    jd.decompress(bufptr, len, buf, stride, r, pb->getPF());
    pb->commitBufferRW(r);
    return;
  }

  // Quit on unsupported compression type.
  assert(comp_ctl <= tightMaxSubencoding);

  // "Basic" compression type.

  int palSize = 0;
  rdr::U8 palette[256 * 4];
  bool useGradient = false;

  if ((comp_ctl & tightExplicitFilter) != 0) {
    rdr::U8 filterId;

    assert(buflen >= 1);

    filterId = *bufptr;
    bufptr += 1;
    buflen -= 1;

    switch (filterId) {
    case tightFilterPalette:
      assert(buflen >= 1);

      palSize = *bufptr + 1;
      bufptr += 1;
      buflen -= 1;

      if (pf.is888()) {
        size_t len = palSize * 3;
        rdr::U8Array tightPalette(len);

        assert(buflen >= len);

        memcpy(tightPalette.buf, bufptr, len);
        bufptr += len;
        buflen -= len;

        pf.bufferFromRGB(palette, tightPalette.buf, palSize);
      } else {
        size_t len;

        len = palSize * pf.bpp/8;

        assert(buflen >= len);

        memcpy(palette, bufptr, len);
        bufptr += len;
        buflen -= len;
      }
      break;
    case tightFilterGradient:
      useGradient = true;
      break;
    case tightFilterCopy:
      break;
    default:
      assert(false);
    }
  }

  // Determine if the data should be decompressed or just copied.
  size_t rowSize, dataSize;
  rdr::U8* netbuf;

  netbuf = NULL;

  if (palSize != 0) {
    if (palSize <= 2)
      rowSize = (r.width() + 7) / 8;
    else
      rowSize = r.width();
  } else if (pf.is888()) {
    rowSize = r.width() * 3;
  } else {
    rowSize = r.width() * pf.bpp/8;
  }

  dataSize = r.height() * rowSize;

  if (dataSize < TIGHT_MIN_TO_COMPRESS)
    assert(buflen >= dataSize);
  else {
    rdr::U32 len;
    int streamId;
    rdr::MemInStream* ms;

    assert(buflen >= 4);

    memcpy(&len, bufptr, 4);
    bufptr += 4;
    buflen -= 4;

    assert(buflen >= len);

    streamId = comp_ctl & 0x03;
    ms = new rdr::MemInStream(bufptr, len);
    zis[streamId].setUnderlying(ms, len);

    // Allocate buffer and decompress the data
    netbuf = new rdr::U8[dataSize];

    if (!zis[streamId].hasData(dataSize))
      throw Exception("Tight decode error");
    zis[streamId].readBytes(netbuf, dataSize);

    zis[streamId].flushUnderlying();
    zis[streamId].setUnderlying(NULL, 0);
    delete ms;

    bufptr = netbuf;
    buflen = dataSize;
  }

  // Time to decode the actual data
  bool directDecode;

  rdr::U8* outbuf;
  int stride;

  if (pb->getPF().equal(pf)) {
    // Decode directly into the framebuffer (fast path)
    directDecode = true;
  } else {
    // Decode into an intermediate buffer and use pixel translation
    directDecode = false;
  }

  if (directDecode)
    outbuf = pb->getBufferRW(r, &stride);
  else {
    outbuf = new rdr::U8[r.area() * (pf.bpp/8)];
    stride = r.width();
  }

  if (palSize == 0) {
    // Truecolor data
    if (useGradient) {
      if (pf.is888())
        FilterGradient24(bufptr, pf, (rdr::U32*)outbuf, stride, r);
      else {
        switch (pf.bpp) {
        case 8:
          assert(false);
          break;
        case 16:
          FilterGradient(bufptr, pf, (rdr::U16*)outbuf, stride, r);
          break;
        case 32:
          FilterGradient(bufptr, pf, (rdr::U32*)outbuf, stride, r);
          break;
        }
      }
    } else {
      // Copy
      rdr::U8* ptr = outbuf;
      const rdr::U8* srcPtr = bufptr;
      int w = r.width();
      int h = r.height();
      if (pf.is888()) {
        while (h > 0) {
          pf.bufferFromRGB(ptr, srcPtr, w);
          ptr += stride * pf.bpp/8;
          srcPtr += w * 3;
          h--;
        }
      } else {
        while (h > 0) {
          memcpy(ptr, srcPtr, w * pf.bpp/8);
          ptr += stride * pf.bpp/8;
          srcPtr += w * pf.bpp/8;
          h--;
        }
      }
    }
  } else {
    // Indexed color
    switch (pf.bpp) {
    case 8:
      FilterPalette((const rdr::U8*)palette, palSize,
                    bufptr, (rdr::U8*)outbuf, stride, r);
      break;
    case 16:
      FilterPalette((const rdr::U16*)palette, palSize,
                    bufptr, (rdr::U16*)outbuf, stride, r);
      break;
    case 32:
      FilterPalette((const rdr::U32*)palette, palSize,
                    bufptr, (rdr::U32*)outbuf, stride, r);
      break;
    }
  }

  if (directDecode)
    pb->commitBufferRW(r);
  else {
    pb->imageRect(pf, r, outbuf);
    delete [] outbuf;
  }

  delete [] netbuf;
}

rdr::U32 TightDecoder::readCompact(rdr::InStream* is)
{
  rdr::U8 b;
  rdr::U32 result;

  b = is->readU8();
  result = (int)b & 0x7F;
  if (b & 0x80) {
    b = is->readU8();
    result |= ((int)b & 0x7F) << 7;
    if (b & 0x80) {
      b = is->readU8();
      result |= ((int)b & 0xFF) << 14;
    }
  }

  return result;
}
