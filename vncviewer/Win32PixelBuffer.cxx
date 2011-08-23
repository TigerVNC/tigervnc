/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#include <stdlib.h>

#include <windows.h>

#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include "Win32PixelBuffer.h"

using namespace rfb;

static rfb::LogWriter vlog("PlatformPixelBuffer");

PlatformPixelBuffer::PlatformPixelBuffer(int width, int height) :
  FullFramePixelBuffer(rfb::PixelFormat(32, 24, false, true,
                                        255, 255, 255, 16, 8, 0),
                       width, height, NULL, NULL),
  bitmap(NULL)
{
  BITMAPINFOHEADER bih;

  memset(&bih, 0, sizeof(bih));

  bih.biSize         = sizeof(BITMAPINFOHEADER);
  bih.biBitCount     = getPF().bpp;
  bih.biSizeImage    = (getPF().bpp / 8) * width * height;
  bih.biPlanes       = 1;
  bih.biWidth        = width;
  bih.biHeight       = -height; // Negative to get top-down
  bih.biCompression  = BI_RGB;

  bitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bih,
                            DIB_RGB_COLORS, (void**)&data, NULL, 0);
  if (!bitmap) {
    int err = GetLastError();
    throw rdr::SystemException("unable to create DIB section", err);
  }
}


PlatformPixelBuffer::~PlatformPixelBuffer()
{
  DeleteObject(bitmap);
}


void PlatformPixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  HDC dc;

  dc = CreateCompatibleDC(fl_gc);
  if (!dc)
    throw rdr::SystemException("CreateCompatibleDC failed", GetLastError());

  if (!SelectObject(dc, bitmap))
    throw rdr::SystemException("SelectObject failed", GetLastError());

  if (!BitBlt(fl_gc, x, y, w, h, dc, src_x, src_y, SRCCOPY)) {
    // If the desktop we're rendering to is inactive (like when the screen
    // is locked or the UAC is active), then GDI calls will randomly fail.
    // This is completely undocumented so we have no idea how best to deal
    // with it. For now, we've only seen this error and for this function
    // so only ignore this combination.
    if (GetLastError() != ERROR_INVALID_HANDLE)
      throw rdr::SystemException("BitBlt failed", GetLastError());
  }

  DeleteDC(dc);
}
