/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <rfb/JpegCompressor.h>
#include <rdr/Exception.h>
#include <rfb/Rect.h>
#include <rfb/PixelFormat.h>
#include <os/print.h>

using namespace rfb;

//
// Error manager implmentation for the JPEG library
//

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

static void
JpegInitDestination(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->clear();
  dest->pub.next_output_byte = jc->getptr();
  dest->pub.free_in_buffer = jc->getend() - jc->getptr();
}

static boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->setptr(dest->pub.next_output_byte);
  jc->overrun(jc->getend() - jc->getstart(), 1);
  dest->pub.next_output_byte = jc->getptr();
  dest->pub.free_in_buffer = jc->getend() - jc->getptr();

  return TRUE;
}

static void
JpegTermDestination(j_compress_ptr cinfo)
{
  JPEG_DEST_MGR *dest = (JPEG_DEST_MGR *)cinfo->dest;
  JpegCompressor *jc = dest->instance;

  jc->setptr(dest->pub.next_output_byte);
}

JpegCompressor::JpegCompressor(int bufferLen) : MemOutStream(bufferLen)
{
  cinfo.err = jpeg_std_error(&err.pub);
  snprintf(err.lastError, JMSG_LENGTH_MAX, "No error");
  err.pub.error_exit = JpegErrorExit;
  err.pub.output_message = JpegOutputMessage;

  if(setjmp(err.jmpBuffer)) {
    // this will execute if libjpeg has an error
    throw rdr::Exception(err.lastError);
  }

  jpeg_create_compress(&cinfo);

  dest.pub.init_destination = JpegInitDestination;
  dest.pub.empty_output_buffer = JpegEmptyOutputBuffer;
  dest.pub.term_destination = JpegTermDestination;
  dest.instance = this;
  cinfo.dest = (struct jpeg_destination_mgr *)&dest;
}

JpegCompressor::~JpegCompressor(void)
{
  if(setjmp(err.jmpBuffer)) {
    // this will execute if libjpeg has an error
    return;
  }

  jpeg_destroy_compress(&cinfo);
}

void JpegCompressor::compress(rdr::U8 *buf, int pitch, const Rect& r,
  const PixelFormat& pf, int quality, JPEG_SUBSAMP subsamp)
{
  int w = r.width();
  int h = r.height();
  int pixelsize;
  rdr::U8 *srcBuf = NULL;
  bool srcBufIsTemp = false;
  JSAMPROW *rowPointer = NULL;

  if(setjmp(err.jmpBuffer)) {
    // this will execute if libjpeg has an error
    jpeg_abort_compress(&cinfo);
    if (srcBufIsTemp && srcBuf) delete[] srcBuf;
    if (rowPointer) delete[] rowPointer;
    throw rdr::Exception(err.lastError);
  }

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.in_color_space = JCS_RGB;
  pixelsize = 3;

#ifdef JCS_EXTENSIONS
  // Try to have libjpeg read directly from our native format
  if(pf.is888()) {
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

    if(redShift == 0 && greenShift == 8 && blueShift == 16)
      cinfo.in_color_space = JCS_EXT_RGBX;
    if(redShift == 16 && greenShift == 8 && blueShift == 0)
      cinfo.in_color_space = JCS_EXT_BGRX;
    if(redShift == 24 && greenShift == 16 && blueShift == 8)
      cinfo.in_color_space = JCS_EXT_XBGR;
    if(redShift == 8 && greenShift == 16 && blueShift == 24)
      cinfo.in_color_space = JCS_EXT_XRGB;

    if (cinfo.in_color_space != JCS_RGB) {
      srcBuf = (rdr::U8 *)buf;
      pixelsize = 4;
    }
  }
#endif

  if (pitch == 0) pitch = w * pixelsize;

  if (cinfo.in_color_space == JCS_RGB) {
    srcBuf = new rdr::U8[w * h * pixelsize];
    srcBufIsTemp = true;
    pf.rgbFromBuffer(srcBuf, (const rdr::U8 *)buf, w, pitch, h);
    pitch = w * pixelsize;
  }

  cinfo.input_components = pixelsize;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE);
  if(quality >= 96) cinfo.dct_method = JDCT_ISLOW;
  else cinfo.dct_method = JDCT_FASTEST;

  switch (subsamp) {
  case SUBSAMP_420:
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;
    break;
  case SUBSAMP_422:
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 1;
    break;
  default:
    cinfo.comp_info[0].h_samp_factor = 1;
    cinfo.comp_info[0].v_samp_factor = 1;
  }

  rowPointer = new JSAMPROW[h];
  for (int dy = 0; dy < h; dy++)
    rowPointer[dy] = (JSAMPROW)(&srcBuf[dy * pitch]);

  jpeg_start_compress(&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height)
    jpeg_write_scanlines(&cinfo, &rowPointer[cinfo.next_scanline],
      cinfo.image_height - cinfo.next_scanline);

  jpeg_finish_compress(&cinfo);

  if (srcBufIsTemp) delete[] srcBuf;
  delete[] rowPointer;
}

void JpegCompressor::writeBytes(const void* data, int length)
{
  throw rdr::Exception("writeBytes() is not valid with a JpegCompressor instance.  Use compress() instead.");
}
