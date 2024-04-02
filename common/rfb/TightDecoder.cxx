/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2009-2022 Pierre Ossman for Cendio AB
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

#include <vector>

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

TightDecoder::TightDecoder() : Decoder(DecoderPartiallyOrdered)
{
}

TightDecoder::~TightDecoder()
{
}

bool TightDecoder::readRect(const Rect& r, rdr::InStream* is,
                            const ServerParams& server, rdr::OutStream* os)
{
  uint8_t comp_ctl;

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
    uint32_t len;

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
    uint8_t filterId;

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
    uint32_t len;

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

bool TightDecoder::doRectsConflict(const Rect& /*rectA*/,
                                   const uint8_t* bufferA,
                                   size_t buflenA,
                                   const Rect& /*rectB*/,
                                   const uint8_t* bufferB,
                                   size_t buflenB,
                                   const ServerParams& /*server*/)
{
  uint8_t comp_ctl_a, comp_ctl_b;

  assert(buflenA >= 1);
  assert(buflenB >= 1);

  comp_ctl_a = *(const uint8_t*)bufferA;
  comp_ctl_b = *(const uint8_t*)bufferB;

  // Resets or use of zlib pose the same problem, so merge them
  if ((comp_ctl_a & 0x80) == 0x00)
    comp_ctl_a |= 1 << ((comp_ctl_a >> 4) & 0x03);
  if ((comp_ctl_b & 0x80) == 0x00)
    comp_ctl_b |= 1 << ((comp_ctl_b >> 4) & 0x03);

  if (((comp_ctl_a & 0x0f) & (comp_ctl_b & 0x0f)) != 0)
    return true;

  return false;
}

void TightDecoder::decodeRect(const Rect& r, const uint8_t* buffer,
                              size_t buflen, const ServerParams& server,
                              ModifiablePixelBuffer* pb)
{
  const uint8_t* bufptr;
  const PixelFormat& pf = server.pf();

  uint8_t comp_ctl;

  bufptr = (const uint8_t*)buffer;

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
      uint8_t pix[4];

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
    uint32_t len;

    int stride;
    uint8_t *buf;

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
  uint8_t palette[256 * 4];
  bool useGradient = false;

  if ((comp_ctl & tightExplicitFilter) != 0) {
    uint8_t filterId;

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
        std::vector<uint8_t> tightPalette(len);

        assert(buflen >= len);

        memcpy(tightPalette.data(), bufptr, len);
        bufptr += len;
        buflen -= len;

        pf.bufferFromRGB(palette, tightPalette.data(), palSize);
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
  uint8_t* netbuf;

  netbuf = nullptr;

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
    uint32_t len;
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
    netbuf = new uint8_t[dataSize];

    if (!zis[streamId].hasData(dataSize))
      throw Exception("Tight decode error");
    zis[streamId].readBytes(netbuf, dataSize);

    zis[streamId].flushUnderlying();
    zis[streamId].setUnderlying(nullptr, 0);
    delete ms;

    bufptr = netbuf;
    buflen = dataSize;
  }

  // Time to decode the actual data
  bool directDecode;

  uint8_t* outbuf;
  int stride;

  if (pb->getPF() == pf) {
    // Decode directly into the framebuffer (fast path)
    directDecode = true;
  } else {
    // Decode into an intermediate buffer and use pixel translation
    directDecode = false;
  }

  if (directDecode)
    outbuf = pb->getBufferRW(r, &stride);
  else {
    outbuf = new uint8_t[r.area() * (pf.bpp/8)];
    stride = r.width();
  }

  if (palSize == 0) {
    // Truecolor data
    if (useGradient) {
      if (pf.is888())
        FilterGradient24(bufptr, pf, (uint32_t*)outbuf, stride, r);
      else {
        switch (pf.bpp) {
        case 8:
          assert(false);
          break;
        case 16:
          FilterGradient(bufptr, pf, (uint16_t*)outbuf, stride, r);
          break;
        case 32:
          FilterGradient(bufptr, pf, (uint32_t*)outbuf, stride, r);
          break;
        }
      }
    } else {
      // Copy
      uint8_t* ptr = outbuf;
      const uint8_t* srcPtr = bufptr;
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
      FilterPalette((const uint8_t*)palette, palSize,
                    bufptr, (uint8_t*)outbuf, stride, r);
      break;
    case 16:
      FilterPalette((const uint16_t*)palette, palSize,
                    bufptr, (uint16_t*)outbuf, stride, r);
      break;
    case 32:
      FilterPalette((const uint32_t*)palette, palSize,
                    bufptr, (uint32_t*)outbuf, stride, r);
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

uint32_t TightDecoder::readCompact(rdr::InStream* is)
{
  uint8_t b;
  uint32_t result;

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

void
TightDecoder::FilterGradient24(const uint8_t *inbuf,
                               const PixelFormat& pf, uint32_t* outbuf,
                               int stride, const Rect& r)
{
  int x, y, c;
  uint8_t prevRow[TIGHT_MAX_WIDTH*3];
  uint8_t thisRow[TIGHT_MAX_WIDTH*3];
  uint8_t pix[3]; 
  int est[3]; 

  memset(prevRow, 0, sizeof(prevRow));

  // Set up shortcut variables
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    for (x = 0; x < rectWidth; x++) {
      /* First pixel in a row */
      if (x == 0) {
        for (c = 0; c < 3; c++) {
          pix[c] = inbuf[y*rectWidth*3+c] + prevRow[c];
          thisRow[c] = pix[c];
        }
        pf.bufferFromRGB((uint8_t*)&outbuf[y*stride], pix, 1);
        continue;
      }

      for (c = 0; c < 3; c++) {
        est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
        if (est[c] > 0xff) {
          est[c] = 0xff;
        } else if (est[c] < 0) {
          est[c] = 0;
        }
        pix[c] = inbuf[(y*rectWidth+x)*3+c] + est[c];
        thisRow[x*3+c] = pix[c];
      }
      pf.bufferFromRGB((uint8_t*)&outbuf[y*stride+x], pix, 1);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }
}

template<class T>
void TightDecoder::FilterGradient(const uint8_t* inbuf,
                                  const PixelFormat& pf, T* outbuf,
                                  int stride, const Rect& r)
{
  int x, y, c;
  static uint8_t prevRow[TIGHT_MAX_WIDTH*3];
  static uint8_t thisRow[TIGHT_MAX_WIDTH*3];
  uint8_t pix[3]; 
  int est[3]; 

  memset(prevRow, 0, sizeof(prevRow));

  // Set up shortcut variables
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    for (x = 0; x < rectWidth; x++) {
      /* First pixel in a row */
      if (x == 0) {
        pf.rgbFromBuffer(pix, &inbuf[y*rectWidth], 1);
        for (c = 0; c < 3; c++)
          pix[c] += prevRow[c];

        memcpy(thisRow, pix, sizeof(pix));

        pf.bufferFromRGB((uint8_t*)&outbuf[y*stride], pix, 1);

        continue;
      }

      for (c = 0; c < 3; c++) {
        est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
        if (est[c] > 255) {
          est[c] = 255;
        } else if (est[c] < 0) {
          est[c] = 0;
        }
      }

      pf.rgbFromBuffer(pix, &inbuf[y*rectWidth+x], 1);
      for (c = 0; c < 3; c++)
        pix[c] += est[c];

      memcpy(&thisRow[x*3], pix, sizeof(pix));

      pf.bufferFromRGB((uint8_t*)&outbuf[y*stride+x], pix, 1);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }
}

template<class T>
void TightDecoder::FilterPalette(const T* palette, int palSize,
                                 const uint8_t* inbuf, T* outbuf,
                                 int stride, const Rect& r)
{
  // Indexed color
  int x, h = r.height(), w = r.width(), b, pad = stride - w;
  T* ptr = outbuf;
  uint8_t bits;
  const uint8_t* srcPtr = inbuf;
  if (palSize <= 2) {
    // 2-color palette
    while (h > 0) {
      for (x = 0; x < w / 8; x++) {
        bits = *srcPtr++;
        for (b = 7; b >= 0; b--) {
          *ptr++ = palette[bits >> b & 1];
        }
      }
      if (w % 8 != 0) {
        bits = *srcPtr++;
        for (b = 7; b >= 8 - w % 8; b--) {
          *ptr++ = palette[bits >> b & 1];
        }
      }
      ptr += pad;
      h--;
    }
  } else {
    // 256-color palette
    while (h > 0) {
      T *endOfRow = ptr + w;
      while (ptr < endOfRow) {
        *ptr++ = palette[*srcPtr++];
      }
      ptr += pad;
      h--;
    }
  }
}
