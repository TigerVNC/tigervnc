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

//
// Tight decoding functions.
//
// This file is #included after having set the following macro:
// BPP                - 8, 16 or 32

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)

#if BPP == 32

void
TightDecoder::FilterGradient24(const rdr::U8 *inbuf,
                               const PixelFormat& pf, PIXEL_T* outbuf,
                               int stride, const Rect& r)
{
  int x, y, c;
  rdr::U8 prevRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 thisRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 pix[3]; 
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
        pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride], pix, 1);
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
      pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride+x], pix, 1);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }
}

#endif

#if BPP != 8

void TightDecoder::FilterGradient(const rdr::U8* inbuf,
                                  const PixelFormat& pf, PIXEL_T* outbuf,
                                  int stride, const Rect& r)
{
  int x, y, c;
  static rdr::U8 prevRow[TIGHT_MAX_WIDTH*3];
  static rdr::U8 thisRow[TIGHT_MAX_WIDTH*3];
  rdr::U8 pix[3]; 
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

        pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride], pix, 1);

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

      pf.bufferFromRGB((rdr::U8*)&outbuf[y*stride+x], pix, 1);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }
}

#endif

void TightDecoder::FilterPalette(const PIXEL_T* palette, int palSize,
                                 const rdr::U8* inbuf, PIXEL_T* outbuf,
                                 int stride, const Rect& r)
{
  // Indexed color
  int x, h = r.height(), w = r.width(), b, pad = stride - w;
  PIXEL_T* ptr = outbuf;
  rdr::U8 bits;
  const rdr::U8* srcPtr = inbuf;
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
      PIXEL_T *endOfRow = ptr + w;
      while (ptr < endOfRow) {
        *ptr++ = palette[*srcPtr++];
      }
      ptr += pad;
      h--;
    }
  }
}

#undef PIXEL_T
}
