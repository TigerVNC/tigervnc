/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2004-2005 Cendio AB. All rights reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/JpegDecompressor.h>
#include <rdr/Exception.h>
#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>

#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}
#include <jerror.h>
#include <setjmp.h>

using namespace rfb;

//
// Special formats that libjpeg can have optimised code paths for
//

static const PixelFormat pfRGBX(32, 24, false, true, 255, 255, 255, 0, 8, 16);
static const PixelFormat pfBGRX(32, 24, false, true, 255, 255, 255, 16, 8, 0);
static const PixelFormat pfXRGB(32, 24, false, true, 255, 255, 255, 8, 16, 24);
static const PixelFormat pfXBGR(32, 24, false, true, 255, 255, 255, 24, 16, 8);

//
// Error manager implementation for the JPEG library
//

struct JPEG_ERROR_MGR {
  struct jpeg_error_mgr pub;
  jmp_buf jmpBuffer;
  char lastError[JMSG_LENGTH_MAX];
};

static void
JpegErrorExit(j_common_ptr dinfo)
{
  JPEG_ERROR_MGR *err = (JPEG_ERROR_MGR *)dinfo->err;

  (*dinfo->err->output_message)(dinfo);
  longjmp(err->jmpBuffer, 1);
}

static void
JpegOutputMessage(j_common_ptr dinfo)
{
  JPEG_ERROR_MGR *err = (JPEG_ERROR_MGR *)dinfo->err;

  (*dinfo->err->format_message)(dinfo, err->lastError);
}


//
// Source manager implementation for the JPEG library.
//

struct JPEG_SRC_MGR {
  struct jpeg_source_mgr pub;
  JpegDecompressor *instance;
};

static void
JpegNoOp(j_decompress_ptr /*dinfo*/)
{
}

static boolean
JpegFillInputBuffer(j_decompress_ptr dinfo)
{
  ERREXIT(dinfo, JERR_BUFFER_SIZE);
  return TRUE;
}

static void
JpegSkipInputData(j_decompress_ptr dinfo, long num_bytes)
{
  JPEG_SRC_MGR *src = (JPEG_SRC_MGR *)dinfo->src;

  if (num_bytes < 0 || (size_t)num_bytes > src->pub.bytes_in_buffer) {
    ERREXIT(dinfo, JERR_BUFFER_SIZE);
  } else {
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

JpegDecompressor::JpegDecompressor(void)
{
  dinfo = new jpeg_decompress_struct;

  err = new struct JPEG_ERROR_MGR;
  dinfo->err = jpeg_std_error(&err->pub);
  snprintf(err->lastError, JMSG_LENGTH_MAX, "No error");
  err->pub.error_exit = JpegErrorExit;
  err->pub.output_message = JpegOutputMessage;

  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    throw rdr::Exception("%s", err->lastError);
  }

  jpeg_create_decompress(dinfo);

  src = new struct JPEG_SRC_MGR;
  src->pub.init_source = JpegNoOp;
  src->pub.fill_input_buffer = JpegFillInputBuffer;
  src->pub.skip_input_data = JpegSkipInputData;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = JpegNoOp;
  src->instance = this;
  dinfo->src = (struct jpeg_source_mgr *)src;
}

JpegDecompressor::~JpegDecompressor(void)
{
  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    return;
  }

  jpeg_destroy_decompress(dinfo);

  delete err;
  delete src;

  delete dinfo;
}

void JpegDecompressor::decompress(const uint8_t *jpegBuf,
                                  int jpegBufLen, uint8_t *buf,
                                  volatile int stride,
                                  const Rect& r, const PixelFormat& pf)
{
  int w = r.width();
  int h = r.height();
  int pixelsize;
  int dstBufStride;
  uint8_t * volatile dstBuf = nullptr;
  volatile bool dstBufIsTemp = false;
  JSAMPROW * volatile rowPointer = nullptr;

  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    jpeg_abort_decompress(dinfo);
    if (dstBufIsTemp && dstBuf) delete[] dstBuf;
    if (rowPointer) delete[] rowPointer;
    throw rdr::Exception("%s", err->lastError);
  }

  src->pub.next_input_byte = jpegBuf;
  src->pub.bytes_in_buffer = jpegBufLen;

  jpeg_read_header(dinfo, TRUE);
  dinfo->out_color_space = JCS_RGB;
  pixelsize = 3;
  if (stride == 0)
    stride = w;
  dstBufStride = stride;

#ifdef JCS_EXTENSIONS
  // Try to have libjpeg output directly to our native format
  // libjpeg can only handle some "standard" formats
  if (pfRGBX == pf)
    dinfo->out_color_space = JCS_EXT_RGBX;
  else if (pfBGRX == pf)
    dinfo->out_color_space = JCS_EXT_BGRX;
  else if (pfXRGB == pf)
    dinfo->out_color_space = JCS_EXT_XRGB;
  else if (pfXBGR == pf)
    dinfo->out_color_space = JCS_EXT_XBGR;

  if (dinfo->out_color_space != JCS_RGB) {
    dstBuf = (uint8_t *)buf;
    pixelsize = 4;
  }
#endif

  if (dinfo->out_color_space == JCS_RGB) {
    dstBuf = new uint8_t[w * h * pixelsize];
    dstBufIsTemp = true;
    dstBufStride = w;
  }

  rowPointer = new JSAMPROW[h];
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&dstBuf[dy * dstBufStride * pixelsize]);

  jpeg_start_decompress(dinfo);

  if (dinfo->output_width != (unsigned)r.width()
    || dinfo->output_height != (unsigned)r.height()
    || dinfo->output_components != pixelsize) {
    jpeg_abort_decompress(dinfo);
    if (dstBufIsTemp && dstBuf) delete[] dstBuf;
    if (rowPointer) delete[] rowPointer;
    throw rdr::Exception("Tight Decoding: Wrong JPEG data received.\n");
  }

  while (dinfo->output_scanline < dinfo->output_height) {
    jpeg_read_scanlines(dinfo, &rowPointer[dinfo->output_scanline],
			dinfo->output_height - dinfo->output_scanline);
  }

  if (dinfo->out_color_space == JCS_RGB)
    pf.bufferFromRGB((uint8_t*)buf, dstBuf, w, stride, h);

  jpeg_finish_decompress(dinfo);

  if (dstBufIsTemp) delete [] dstBuf;
  delete[] rowPointer;
}
