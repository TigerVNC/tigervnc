/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
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
// Tight decoding functions.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// FILL_RECT          - fill a rectangle with a single color
// IMAGE_RECT         - draw a rectangle of pixel data from a buffer

#include <rdr/InStream.h>
#include <rdr/ZlibInStream.h>
#include <rfb/Exception.h>
#include <assert.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define READ_PIXEL CONCAT2E(readOpaque,BPP)
#define TIGHT_DECODE TightDecoder::CONCAT2E(tightDecode,BPP)
#define DECOMPRESS_JPEG_RECT TightDecoder::CONCAT2E(DecompressJpegRect,BPP)
#define FILTER_GRADIENT TightDecoder::CONCAT2E(FilterGradient,BPP)
#define DIRECT_FILL_RECT TightDecoder::CONCAT2E(directFillRect,BPP)

#define TIGHT_MIN_TO_COMPRESS 12

// Main function implementing Tight decoder

void TIGHT_DECODE (const Rect& r)
{
  bool cutZeros = false;
#if BPP == 32
  if (serverpf.is888()) {
    cutZeros = true;
  } 
#endif

  rdr::U8 comp_ctl = is->readU8();

  // Flush zlib streams if we are told by the server to do so.
  for (int i = 0; i < 4; i++) {
    if (comp_ctl & 1) {
      zis[i].reset();
    }
    comp_ctl >>= 1;
  }

  // "Fill" compression type.
  if (comp_ctl == rfbTightFill) {
    PIXEL_T pix;
    if (cutZeros) {
      rdr::U8 bytebuf[3];
      is->readBytes(bytebuf, 3);
      serverpf.bufferFromRGB((rdr::U8*)&pix, bytebuf, 1, NULL);
    } else {
      pix = is->READ_PIXEL();
    }
    if (directDecode) DIRECT_FILL_RECT(r, pix);
    else FILL_RECT(r, pix);
    return;
  }

  // "JPEG" compression type.
  if (comp_ctl == rfbTightJpeg) {
    DECOMPRESS_JPEG_RECT(r);
    return;
  }

  // Quit on unsupported compression type.
  if (comp_ctl > rfbTightMaxSubencoding) {
    throw Exception("TightDecoder: bad subencoding value received");
    return;
  }

  // "Basic" compression type.
  int palSize = 0;
  static PIXEL_T palette[256];
  bool useGradient = false;

  if ((comp_ctl & rfbTightExplicitFilter) != 0) {
    rdr::U8 filterId = is->readU8();

    switch (filterId) {
    case rfbTightFilterPalette: 
      palSize = is->readU8() + 1;
      if (cutZeros) {
        rdr::U8 elem[3];
        for (int i = 0;i < palSize;i++) {
          is->readBytes(elem, 3);
          serverpf.bufferFromRGB((rdr::U8*)&palette[i], elem, 1, NULL);
        }
      } else {
        for (int i = 0; i < palSize; i++)
          palette[i] = is->READ_PIXEL();
      }
      break;
    case rfbTightFilterGradient: 
      useGradient = true;
      break;
    case rfbTightFilterCopy:
      break;
    default:
      throw Exception("TightDecoder: unknown filter code received");
      return;
    }
  }

  int bppp = BPP;
  if (palSize != 0) {
    bppp = (palSize <= 2) ? 1 : 8;
  } else if (cutZeros) {
    bppp = 24;
  }

  // Determine if the data should be decompressed or just copied.
  int rowSize = (r.width() * bppp + 7) / 8;
  int dataSize = r.height() * rowSize;
  int streamId = -1;
  rdr::InStream *input;
  if (dataSize < TIGHT_MIN_TO_COMPRESS) {
    input = is;
  } else {
    int length = is->readCompactLength();
    streamId = comp_ctl & 0x03;
    zis[streamId].setUnderlying(is, length);
    input = &zis[streamId];
  }

  PIXEL_T *buf;
  int stride = r.width();
  if (directDecode) buf = (PIXEL_T *)handler->getRawPixelsRW(r, &stride);
  else buf = (PIXEL_T *)reader->getImageBuf(r.area());

  if (palSize == 0) {
    // Truecolor data
    if (useGradient) {
#if BPP == 32
      if (cutZeros) {
        FilterGradient24(input, buf, stride, r, dataSize);
      } else 
#endif
      {
        FILTER_GRADIENT(input, buf, stride, r, dataSize);
      }
    } else {
      // Copy
      int h = r.height();
      PIXEL_T *ptr = buf;
      if (cutZeros) {
        int w = r.width(), pad = stride - w;
        rdr::U8 elem[3];
        while (h > 0) {
          PIXEL_T *endOfRow = ptr + w;
          while (ptr < endOfRow) {
            input->readBytes(elem, 3);
            serverpf.bufferFromRGB((rdr::U8*)ptr++, elem, 1, NULL);
          }
          ptr += pad;
          h--;
        }
      } else {
        while (h > 0) {
          input->readBytes(ptr, rowSize);
          ptr += stride;
          h--;
        }
      }
    }
  } else {
    // Indexed color
    int x, h = r.height(), w = r.width(), b, pad = stride - w;
    PIXEL_T *ptr = buf;
    rdr::U8 bits;
    if (palSize <= 2) {
      // 2-color palette
      while (h > 0) {
        for (x = 0; x < w / 8; x++) {
          bits = input->readU8();
          for (b = 7; b >= 0; b--) {
            *ptr++ = palette[bits >> b & 1];
          }
        }
        if (w % 8 != 0) {
          bits = input->readU8();
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
        PIXEL_T *endOfRow = ptr + w;
        while (ptr < endOfRow) {
          *ptr++ = palette[input->readU8()];
        }
        ptr += pad;
        h--;
      }
    }
  }

  if (directDecode) handler->releaseRawPixels(r);
  else IMAGE_RECT(r, buf);

  if (streamId != -1) {
    zis[streamId].reset();
  }
}

void
DECOMPRESS_JPEG_RECT(const Rect& r)
{
  // Read length
  int compressedLen = is->readCompactLength();
  if (compressedLen <= 0) {
      throw Exception("Incorrect data received from the server.\n");
  }

  // Allocate netbuf and read in data
  rdr::U8* netbuf = new rdr::U8[compressedLen];
  if (!netbuf) {
    throw Exception("rfb::tightDecode unable to allocate buffer");
  }
  is->readBytes(netbuf, compressedLen);

  // We always use direct decoding with JPEG images
  int stride;
  rdr::U8 *buf = handler->getRawPixelsRW(r, &stride);
  jd.decompress(netbuf, compressedLen, buf, stride * clientpf.bpp / 8, r,
                clientpf);
  handler->releaseRawPixels(r);

  delete [] netbuf;
}

#if BPP == 32

void
TightDecoder::FilterGradient24(rdr::InStream* is, PIXEL_T* buf, int stride,
                               const Rect& r, int dataSize)
{
  int x, y, c;
  static rdr::U8 prevRow[TIGHT_MAX_WIDTH*3];
  static rdr::U8 thisRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 pix[3]; 
  int est[3]; 

  memset(prevRow, 0, sizeof(prevRow));

  // Allocate netbuf and read in data
  rdr::U8 *netbuf = new rdr::U8[dataSize];
  if (!netbuf) {
    throw Exception("rfb::tightDecode unable to allocate buffer");
  }
  is->readBytes(netbuf, dataSize);

  // Set up shortcut variables
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    /* First pixel in a row */
    for (c = 0; c < 3; c++) {
      pix[c] = netbuf[y*rectWidth*3+c] + prevRow[c];
      thisRow[c] = pix[c];
    }
    serverpf.bufferFromRGB((rdr::U8*)&buf[y*stride], pix, 1, NULL);

    /* Remaining pixels of a row */
    for (x = 1; x < rectWidth; x++) {
      for (c = 0; c < 3; c++) {
        est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
        if (est[c] > 0xff) {
          est[c] = 0xff;
        } else if (est[c] < 0) {
          est[c] = 0;
        }
        pix[c] = netbuf[(y*rectWidth+x)*3+c] + est[c];
        thisRow[x*3+c] = pix[c];
      }
      serverpf.bufferFromRGB((rdr::U8*)&buf[y*stride+x], pix, 1, NULL);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }

  delete [] netbuf;
}

#endif

void
FILTER_GRADIENT(rdr::InStream* is, PIXEL_T* buf, int stride, const Rect& r,
                int dataSize)
{
  int x, y, c;
  static rdr::U8 prevRow[TIGHT_MAX_WIDTH*sizeof(PIXEL_T)];
  static rdr::U8 thisRow[TIGHT_MAX_WIDTH*sizeof(PIXEL_T)];
  rdr::U8 pix[3]; 
  int est[3]; 

  memset(prevRow, 0, sizeof(prevRow));

  // Allocate netbuf and read in data
  PIXEL_T *netbuf = (PIXEL_T*)new rdr::U8[dataSize];
  if (!netbuf) {
    throw Exception("rfb::tightDecode unable to allocate buffer");
  }
  is->readBytes(netbuf, dataSize);

  // Set up shortcut variables
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    /* First pixel in a row */
    serverpf.rgbFromBuffer(pix, (rdr::U8*)&netbuf[y*rectWidth], 1, NULL);
    for (c = 0; c < 3; c++)
      pix[c] += prevRow[c];

    memcpy(thisRow, pix, sizeof(pix));

    serverpf.bufferFromRGB((rdr::U8*)&buf[y*stride], pix, 1, NULL);

    /* Remaining pixels of a row */
    for (x = 1; x < rectWidth; x++) {
      for (c = 0; c < 3; c++) {
        est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
        if (est[c] > 255) {
          est[c] = 255;
        } else if (est[c] < 0) {
          est[c] = 0;
        }
      }

      serverpf.rgbFromBuffer(pix, (rdr::U8*)&netbuf[y*rectWidth+x], 1, NULL);
      for (c = 0; c < 3; c++)
        pix[c] += est[c];

      memcpy(&thisRow[x*3], pix, sizeof(pix));

      serverpf.bufferFromRGB((rdr::U8*)&buf[y*stride+x], pix, 1, NULL);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }

  delete [] netbuf;
}

void
DIRECT_FILL_RECT(const Rect& r, Pixel pix) {

  int stride;
  PIXEL_T *buf = (PIXEL_T *)handler->getRawPixelsRW(r, &stride);

  int w = r.width(), h = r.height();
  PIXEL_T *ptr = buf;
#if BPP != 8
  int pad = stride - w;
#endif

  while (h > 0) {
#if BPP == 8
    memset(ptr, pix, w);
    ptr += stride;
#else
    PIXEL_T *endOfRow = ptr + w;
    while (ptr < endOfRow) {
      *ptr++ = pix;
    }
    ptr += pad;
#endif
    h--;
  }

  handler->releaseRawPixels(r);
}

#undef TIGHT_MIN_TO_COMPRESS
#undef DIRECT_FILL_RECT
#undef FILTER_GRADIENT
#undef DECOMPRESS_JPEG_RECT
#undef TIGHT_DECODE
#undef READ_PIXEL
#undef PIXEL_T
}
