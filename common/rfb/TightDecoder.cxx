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
#include <rfb/CMsgReader.h>
#include <rfb/CMsgHandler.h>
#include <rfb/TightDecoder.h>
#include <stdio.h> /* jpeglib.h needs FILE */
extern "C" {
#include <jpeglib.h>
}

using namespace rfb;

#define RGB_TO_PIXEL(r,g,b)						\
  (((PIXEL_T)(r) & myFormat.redMax) << myFormat.redShift |		\
   ((PIXEL_T)(g) & myFormat.greenMax) << myFormat.greenShift |	        \
   ((PIXEL_T)(b) & myFormat.blueMax) << myFormat.blueShift)

#define RGB24_TO_PIXEL(r,g,b)                                       \
   ((((PIXEL_T)(r) & 0xFF) * myFormat.redMax + 127) / 255             \
    << myFormat.redShift |                                              \
    (((PIXEL_T)(g) & 0xFF) * myFormat.greenMax + 127) / 255           \
    << myFormat.greenShift |                                            \
    (((PIXEL_T)(b) & 0xFF) * myFormat.blueMax + 127) / 255            \
    << myFormat.blueShift)

#define RGB24_TO_PIXEL32(r,g,b)						\
  (((rdr::U32)(r) & 0xFF) << myFormat.redShift |				\
   ((rdr::U32)(g) & 0xFF) << myFormat.greenShift |			\
   ((rdr::U32)(b) & 0xFF) << myFormat.blueShift)

#define TIGHT_MAX_WIDTH 2048

static void JpegSetSrcManager(j_decompress_ptr cinfo, char *compressedData,
			      int compressedLen);
static bool jpegError;

#define EXTRA_ARGS CMsgHandler* handler
#define FILL_RECT(r, p) handler->fillRect(r, p)
#define IMAGE_RECT(r, p) handler->imageRect(r, p)
#define BPP 8
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 16
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 32
#include <rfb/tightDecode.h>
#undef BPP

Decoder* TightDecoder::create(CMsgReader* reader)
{
  return new TightDecoder(reader);
}

TightDecoder::TightDecoder(CMsgReader* reader_) : reader(reader_)
{
}

TightDecoder::~TightDecoder()
{
}

void TightDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  rdr::InStream* is = reader->getInStream();
  /* Uncompressed RGB24 JPEG data, before translated, can be up to 3
     times larger, if VNC bpp is 8. */
  rdr::U8* buf = reader->getImageBuf(r.area()*3);
  switch (reader->bpp()) {
  case 8:
    tightDecode8 (r, is, zis, (rdr::U8*) buf, handler); break;
  case 16:
    tightDecode16(r, is, zis, (rdr::U16*)buf, handler); break;
  case 32:
    tightDecode32(r, is, zis, (rdr::U32*)buf, handler); break;
  }
}


//
// A "Source manager" for the JPEG library.
//

static struct jpeg_source_mgr jpegSrcManager;
static JOCTET *jpegBufferPtr;
static size_t jpegBufferLen;

static void JpegInitSource(j_decompress_ptr cinfo);
static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);
static void JpegTermSource(j_decompress_ptr cinfo);

static void
JpegInitSource(j_decompress_ptr cinfo)
{
  jpegError = false;
}

static boolean
JpegFillInputBuffer(j_decompress_ptr cinfo)
{
  jpegError = true;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;
  jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;

  return TRUE;
}

static void
JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
  if (num_bytes < 0 || (size_t)num_bytes > jpegSrcManager.bytes_in_buffer) {
    jpegError = true;
    jpegSrcManager.bytes_in_buffer = jpegBufferLen;
    jpegSrcManager.next_input_byte = (JOCTET *)jpegBufferPtr;
  } else {
    jpegSrcManager.next_input_byte += (size_t) num_bytes;
    jpegSrcManager.bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
JpegTermSource(j_decompress_ptr cinfo)
{
  /* No work necessary here. */
}

static void
JpegSetSrcManager(j_decompress_ptr cinfo, char *compressedData, int compressedLen)
{
  jpegBufferPtr = (JOCTET *)compressedData;
  jpegBufferLen = (size_t)compressedLen;

  jpegSrcManager.init_source = JpegInitSource;
  jpegSrcManager.fill_input_buffer = JpegFillInputBuffer;
  jpegSrcManager.skip_input_data = JpegSkipInputData;
  jpegSrcManager.resync_to_restart = jpeg_resync_to_restart;
  jpegSrcManager.term_source = JpegTermSource;
  jpegSrcManager.next_input_byte = jpegBufferPtr;
  jpegSrcManager.bytes_in_buffer = jpegBufferLen;

  cinfo->src = &jpegSrcManager;
}
