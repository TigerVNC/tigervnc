/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
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
// FILL_RECT          - fill a rectangle with a single colour
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
#define TIGHT_DECODE CONCAT2E(tightDecode,BPP)

#define TIGHT_MIN_TO_COMPRESS 12

// Main function implementing Tight decoder

void TIGHT_DECODE (const Rect& r, rdr::InStream* is,
                   rdr::ZlibInStream zis[], PIXEL_T* buf
#ifdef EXTRA_ARGS
                      , EXTRA_ARGS
#endif
                      )
{
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
    PIXEL_T pix = is->READ_PIXEL();
    FILL_RECT(r, pix);
    return;
  }

  // "JPEG" compression type.
  if (comp_ctl == rfbTightJpeg) {
    throw Exception("TightDecoder: FIXME: JPEG compression is not supported yet");
	return;
  }

  // Quit on unsupported compression type.
  if (comp_ctl > rfbTightMaxSubencoding) {
    throw Exception("TightDecoder: bad subencoding value received");
    return;
  }

  // "Basic" compression type.
  int palSize = 0;
  PIXEL_T palette[256];
  bool useGradient = false;

  if ((comp_ctl & rfbTightExplicitFilter) != 0) {
    rdr::U8 filterId = is->readU8();

    switch (filterId) {
    case rfbTightFilterPalette: 
      palSize = is->readU8() + 1;
      {
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
  }

  // Determine if the data should be decompressed or just copied.
  int rowSize = (r.width() * bppp + 7) / 8;
  int dataSize = r.height() * rowSize;
  rdr::InStream *input;
  if (dataSize < TIGHT_MIN_TO_COMPRESS) {
    input = is;
  } else {
    int length = is->readCompactLength();
    int streamId = comp_ctl & 0x03;
    zis[streamId].setUnderlying(is, length);
    input = &zis[streamId];
  }

  if (palSize == 0) {
    // Truecolor data
    input->readBytes(buf, dataSize);
    if (useGradient) {
      // FIXME: Implement the "gradient" filter.
    }
  } else {
    int x, y, b, w;
    PIXEL_T *ptr = buf;
    rdr::U8 bits;
    if (palSize <= 2) {
      // 2-color palette
      w = (r.width() + 7) / 8;
      for (y = 0; y < r.height(); y++) {
        for (x = 0; x < r.width() / 8; x++) {
          bits = input->readU8();
          for (b = 7; b >= 0; b--)
            *ptr++ = palette[bits >> b & 1];
        }
        if (r.width() % 8 != 0) {
          bits = input->readU8();
          for (b = 7; b >= 8 - r.width() % 8; b--) {
            *ptr++ = palette[bits >> b & 1];
          }
        }
      }
    } else {
      // 256-color palette
      for (y = 0; y < r.height(); y++)
        for (x = 0; x < r.width(); x++)
          *ptr++ = palette[input->readU8()];
    }
  }

  IMAGE_RECT(r, buf);
}

#undef TIGHT_DECODE
#undef READ_PIXEL
#undef PIXEL_T
}
