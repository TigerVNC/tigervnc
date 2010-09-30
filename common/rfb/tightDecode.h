/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2004-2005 Cendio AB. All rights reserved.
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
#define TIGHT_DECODE CONCAT2E(tightDecode,BPP)

#define TIGHT_MIN_TO_COMPRESS 12
static bool DecompressJpegRect(const Rect& r, rdr::InStream* is,
			       PIXEL_T* buf, CMsgHandler* handler);
static void FilterGradient(const Rect& r, rdr::InStream* is, int dataSize,
			   PIXEL_T* buf, CMsgHandler* handler);
#if BPP == 32
static void FilterGradient24(const Rect& r, rdr::InStream* is, int dataSize,
			     PIXEL_T* buf, CMsgHandler* handler);
#endif

// Main function implementing Tight decoder

void TIGHT_DECODE (const Rect& r, rdr::InStream* is,
                   rdr::ZlibInStream zis[], PIXEL_T* buf
#ifdef EXTRA_ARGS
                      , EXTRA_ARGS
#endif
                      )
{
  rdr::U8 *bytebuf = (rdr::U8*) buf;
  bool cutZeros = false;
  const rfb::PixelFormat& myFormat = handler->cp.pf();
#if BPP == 32
  if (myFormat.is888()) {
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
      is->readBytes(bytebuf, 3);
      myFormat.bufferFromRGB((rdr::U8*)&pix, bytebuf, 1, NULL);
    } else {
      pix = is->READ_PIXEL();
    }
    FILL_RECT(r, pix);
    return;
  }

  // "JPEG" compression type.
  if (comp_ctl == rfbTightJpeg) {
    DecompressJpegRect(r, is, buf, handler);
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
          myFormat.bufferFromRGB((rdr::U8*)&palette[i], elem, 1, NULL);
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

  if (palSize == 0) {
    // Truecolor data
    if (useGradient) {
#if BPP == 32
      if (cutZeros) {
	FilterGradient24(r, input, dataSize, buf, handler);
      } else 
#endif
	{
	  FilterGradient(r, input, dataSize, buf, handler);
	}
    } else {
      if (cutZeros) {
        rdr::U8 elem[3];
        for (int i = 0;i < r.area();i++) {
          input->readBytes(elem, 3);
          myFormat.bufferFromRGB((rdr::U8*)&buf[i], elem, 1, NULL);
        }
      } else {
        input->readBytes(buf, dataSize); 
      }
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
          for (b = 7; b >= 0; b--) {
            *ptr++ = palette[bits >> b & 1];
	  }
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
      for (y = 0; y < r.height(); y++) {
        for (x = 0; x < r.width(); x++) {
          *ptr++ = palette[input->readU8()];
	}
      }
    }
  }

  IMAGE_RECT(r, buf);

  if (streamId != -1) {
    zis[streamId].reset();
  }
}

static bool
DecompressJpegRect(const Rect& r, rdr::InStream* is,
		   PIXEL_T* buf, CMsgHandler* handler)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int w = r.width();
  int h = r.height();
  int pixelsize;
  rdr::U8 *dstBuf = NULL;
  bool dstBufIsTemp = false;
  const rfb::PixelFormat& pf = handler->cp.pf();

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

  // Set up JPEG decompression
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  JpegSetSrcManager(&cinfo, (char*)netbuf, compressedLen);
  jpeg_read_header(&cinfo, TRUE);

  cinfo.out_color_space = JCS_RGB;
  pixelsize = 3;

#ifdef JCS_EXTENSIONS
  // Try to have libjpeg output directly to our native format
  if (pf.is888()) {
    int redShift, greenShift, blueShift;

    if(pf.bigEndian) {
      redShift = 24 - pf.redShift;
      greenShift = 24 - pf.greenShift;
      blueShift = 24 - pf.blueShift;
    } else {
      redShift = pf.redShift;
      greenShift = pf.greenShift;
      blueShift = pf.blueShift;
    }

    // libjpeg can only handle some "standard" formats
    if(redShift == 0 && greenShift == 8 && blueShift == 16)
      cinfo.out_color_space = JCS_EXT_RGBX;
    if(redShift == 16 && greenShift == 8 && blueShift == 0)
      cinfo.out_color_space = JCS_EXT_BGRX;
    if(redShift == 24 && greenShift == 16 && blueShift == 8)
      cinfo.out_color_space = JCS_EXT_XBGR;
    if(redShift == 8 && greenShift == 16 && blueShift == 24)
      cinfo.out_color_space = JCS_EXT_XRGB;

    if (cinfo.out_color_space != JCS_RGB) {
      dstBuf = (rdr::U8 *)buf;
      pixelsize = 4;
    }
  }
#endif

  if (cinfo.out_color_space == JCS_RGB) {
    dstBuf = new rdr::U8[w * h * pixelsize];
    dstBufIsTemp = true;
  }

  JSAMPROW *rowPointer = new JSAMPROW[h];
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&dstBuf[dy * w * pixelsize]);

  jpeg_start_decompress(&cinfo);
  if (cinfo.output_width != (unsigned)r.width() || cinfo.output_height != (unsigned)r.height() ||
      cinfo.output_components != pixelsize) {
      jpeg_destroy_decompress(&cinfo);
      throw Exception("Tight Encoding: Wrong JPEG data received.\n");
  }

  // Decompress
  const rfb::PixelFormat& myFormat = handler->cp.pf();
  while (cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, &rowPointer[cinfo.output_scanline],
			cinfo.output_height - cinfo.output_scanline);
    if (jpegError) {
      break;
    }
  }

  delete [] rowPointer;

  if (cinfo.out_color_space == JCS_RGB)
    myFormat.bufferFromRGB((rdr::U8*)buf, dstBuf, w * h);

  IMAGE_RECT(r, buf);

  if (!jpegError) {
    jpeg_finish_decompress(&cinfo);
  }

  jpeg_destroy_decompress(&cinfo);

  if (dstBufIsTemp) delete [] dstBuf;
  delete [] netbuf;

  return !jpegError;
}

#if BPP == 32

static void
FilterGradient24(const Rect& r, rdr::InStream* is, int dataSize,
		 PIXEL_T* buf, CMsgHandler* handler)
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
  const rfb::PixelFormat& myFormat = handler->cp.pf();
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    /* First pixel in a row */
    for (c = 0; c < 3; c++) {
      pix[c] = netbuf[y*rectWidth*3+c] + prevRow[c];
      thisRow[c] = pix[c];
    }
    myFormat.bufferFromRGB((rdr::U8*)&buf[y*rectWidth], pix, 1, NULL);

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
      myFormat.bufferFromRGB((rdr::U8*)&buf[y*rectWidth+x], pix, 1, NULL);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }

  delete [] netbuf;
}

#endif

static void
FilterGradient(const Rect& r, rdr::InStream* is, int dataSize,
	       PIXEL_T* buf, CMsgHandler* handler)
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
  const rfb::PixelFormat& myFormat = handler->cp.pf();
  int rectHeight = r.height();
  int rectWidth = r.width();

  for (y = 0; y < rectHeight; y++) {
    /* First pixel in a row */
    myFormat.rgbFromBuffer(pix, (rdr::U8*)&netbuf[y*rectWidth], 1, NULL);
    for (c = 0; c < 3; c++)
      pix[c] += prevRow[c];

    memcpy(thisRow, pix, sizeof(pix));

    myFormat.bufferFromRGB((rdr::U8*)&buf[y*rectWidth], pix, 1, NULL);

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

      myFormat.rgbFromBuffer(pix, (rdr::U8*)&netbuf[y*rectWidth+x], 1, NULL);
      for (c = 0; c < 3; c++)
        pix[c] += est[c];

      memcpy(&thisRow[x*3], pix, sizeof(pix));

      myFormat.bufferFromRGB((rdr::U8*)&buf[y*rectWidth+x], pix, 1, NULL);
    }

    memcpy(prevRow, thisRow, sizeof(prevRow));
  }

  delete [] netbuf;
}

#undef TIGHT_MIN_TO_COMPRESS
#undef TIGHT_DECODE
#undef READ_PIXEL
#undef PIXEL_T
}
