/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <rfb/JpegCompressor.h>
#include <rdr/Exception.h>
#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>
#include <rfb/ClientParams.h>

#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}
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
JpegErrorExit(j_common_ptr cinfo)
{
  JPEG_ERROR_MGR *err = (JPEG_ERROR_MGR *)cinfo->err;

  (*cinfo->err->output_message)(cinfo);
  longjmp(err->jmpBuffer, 1);
}

static void
JpegOutputMessage(j_common_ptr cinfo)
{
  JPEG_ERROR_MGR *err = (JPEG_ERROR_MGR *)cinfo->err;

  (*cinfo->err->format_message)(cinfo, err->lastError);
}

//
// Destination manager implementation for the JPEG library.
//

struct JPEG_DEST_MGR {
  struct jpeg_destination_mgr pub;
  JpegCompressor *instance;
  size_t chunkSize;
};

static void
JpegInitDestination(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->clear();
  dest->pub.next_output_byte = jc->getptr(jc->length());
  dest->pub.free_in_buffer = dest->chunkSize = jc->avail();
}

static boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->setptr(jc->avail());
  dest->pub.next_output_byte = jc->getptr(jc->length());
  dest->pub.free_in_buffer = dest->chunkSize = jc->avail();

  return TRUE;
}

static void
JpegTermDestination(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->setptr(dest->chunkSize - dest->pub.free_in_buffer);
}

JpegCompressor::JpegCompressor(int bufferLen) : MemOutStream(bufferLen)
{
  cinfo = new jpeg_compress_struct;

  err = new struct JPEG_ERROR_MGR;
  cinfo->err = jpeg_std_error(&err->pub);
  snprintf(err->lastError, JMSG_LENGTH_MAX, "No error");
  err->pub.error_exit = JpegErrorExit;
  err->pub.output_message = JpegOutputMessage;

  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    throw rdr::Exception("%s", err->lastError);
  }

  jpeg_create_compress(cinfo);

  dest = new struct JPEG_DEST_MGR;
  dest->pub.init_destination = JpegInitDestination;
  dest->pub.empty_output_buffer = JpegEmptyOutputBuffer;
  dest->pub.term_destination = JpegTermDestination;
  dest->instance = this;
  cinfo->dest = (struct jpeg_destination_mgr *)dest;
}

JpegCompressor::~JpegCompressor(void)
{
  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    return;
  }

  jpeg_destroy_compress(cinfo);

  delete err;
  delete dest;

  delete cinfo;
}

void JpegCompressor::compress(const rdr::U8 *buf, int stride, const Rect& r,
  const PixelFormat& pf, int quality, int subsamp)
{
  int w = r.width();
  int h = r.height();
  int pixelsize;
  rdr::U8 *srcBuf = NULL;
  bool srcBufIsTemp = false;
  JSAMPROW *rowPointer = NULL;

  if(setjmp(err->jmpBuffer)) {
    // this will execute if libjpeg has an error
    jpeg_abort_compress(cinfo);
    if (srcBufIsTemp && srcBuf) delete[] srcBuf;
    if (rowPointer) delete[] rowPointer;
    throw rdr::Exception("%s", err->lastError);
  }

  cinfo->image_width = w;
  cinfo->image_height = h;
  cinfo->in_color_space = JCS_RGB;
  pixelsize = 3;

#ifdef JCS_EXTENSIONS
  // Try to have libjpeg output directly to our native format
  // libjpeg can only handle some "standard" formats
  if (pfRGBX.equal(pf))
    cinfo->in_color_space = JCS_EXT_RGBX;
  else if (pfBGRX.equal(pf))
    cinfo->in_color_space = JCS_EXT_BGRX;
  else if (pfXRGB.equal(pf))
    cinfo->in_color_space = JCS_EXT_XRGB;
  else if (pfXBGR.equal(pf))
    cinfo->in_color_space = JCS_EXT_XBGR;

  if (cinfo->in_color_space != JCS_RGB) {
    srcBuf = (rdr::U8 *)buf;
    pixelsize = 4;
  }
#endif

  if (stride == 0)
    stride = w;

  if (cinfo->in_color_space == JCS_RGB) {
    srcBuf = new rdr::U8[w * h * pixelsize];
    srcBufIsTemp = true;
    pf.rgbFromBuffer(srcBuf, (const rdr::U8 *)buf, w, stride, h);
    stride = w;
  }

  cinfo->input_components = pixelsize;

  jpeg_set_defaults(cinfo);

  if (quality >= 1 && quality <= 100) {
    jpeg_set_quality(cinfo, quality, TRUE);
    if (quality >= 96)
      cinfo->dct_method = JDCT_ISLOW;
    else
      cinfo->dct_method = JDCT_FASTEST;
  }

  switch (subsamp) {
  case subsample16X:
  case subsample8X:
    // FIXME (fall through)
  case subsample4X:
    cinfo->comp_info[0].h_samp_factor = 2;
    cinfo->comp_info[0].v_samp_factor = 2;
    break;
  case subsample2X:
    cinfo->comp_info[0].h_samp_factor = 2;
    cinfo->comp_info[0].v_samp_factor = 1;
    break;
  case subsampleGray:
    jpeg_set_colorspace(cinfo, JCS_GRAYSCALE);
  default:
    cinfo->comp_info[0].h_samp_factor = 1;
    cinfo->comp_info[0].v_samp_factor = 1;
  }

  rowPointer = new JSAMPROW[h];
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&srcBuf[dy * stride * pixelsize]);

  jpeg_start_compress(cinfo, TRUE);
  while (cinfo->next_scanline < cinfo->image_height)
    jpeg_write_scanlines(cinfo, &rowPointer[cinfo->next_scanline],
      cinfo->image_height - cinfo->next_scanline);

  jpeg_finish_compress(cinfo);

  if (srcBufIsTemp) delete[] srcBuf;
  delete[] rowPointer;
}

void JpegCompressor::writeBytes(const void* data, int length)
{
  throw rdr::Exception("writeBytes() is not valid with a JpegCompressor instance.  Use compress() instead.");
}
