/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <rfb_win32/DeviceContext.h>
#include <rfb_win32/CompatibleBitmap.h>
#include <rfb_win32/BitmapInfo.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;


static LogWriter vlog("DeviceContext");

PixelFormat DeviceContext::getPF() const {
  return getPF(dc);
}

PixelFormat DeviceContext::getPF(HDC dc) {
  bool trueColour, bigEndian;
  int bpp, depth;
  int redMax, greenMax, blueMax;
  int redShift, greenShift, blueShift;

  CompatibleBitmap bitmap(dc, 1, 1);

  // -=- Get the bitmap format information
  BitmapInfo bi;
  memset(&bi, 0, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biBitCount = 0;
  if (!::GetDIBits(dc, bitmap, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
    throw rdr::SystemException("unable to determine device pixel format", GetLastError());
  }
  if (!::GetDIBits(dc, bitmap, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
    throw rdr::SystemException("unable to determine pixel shifts/palette", GetLastError());
  }

  // Set the initial format information
  trueColour = bi.bmiHeader.biBitCount > 8;
  bigEndian = 0;
  bpp = bi.bmiHeader.biBitCount;

  if (trueColour) {
    DWORD rMask=0, gMask=0, bMask=0;

    // Which true colour format is the DIB section using?
    switch (bi.bmiHeader.biCompression) {
    case BI_RGB:
      // Default RGB layout
      switch (bi.bmiHeader.biBitCount) {
      case 16:
        // RGB 555 - High Colour
        vlog.info("16-bit High Colour");
        rMask = 0x7c00;
        bMask = 0x001f;
        gMask = 0x03e0;
        break;
      case 24:
      case 32:
        // RGB 888 - True Colour
        vlog.info("24/32-bit High Colour");
        rMask = 0xff0000;
        gMask = 0x00ff00;
        bMask = 0x0000ff;
        break;
      default:
        vlog.error("bits per pixel %u not supported", bi.bmiHeader.biBitCount);
        throw rdr::Exception("unknown bits per pixel specified");
      };
      break;
    case BI_BITFIELDS:
      // Custom RGB layout
      rMask = bi.mask.red;
      gMask = bi.mask.green;
      bMask = bi.mask.blue;
      vlog.info("%d-bit BitFields: (%lx, %lx, %lx)",
                 bi.bmiHeader.biBitCount, rMask, gMask, bMask);
      break;
    };

    // Convert the data we just retrieved
    initMaxAndShift(rMask, &redMax, &redShift);
    initMaxAndShift(gMask, &greenMax, &greenShift);
    initMaxAndShift(bMask, &blueMax, &blueShift);

    // Calculate the depth from the colour shifts
    depth = 0;
    Pixel bits = rMask | gMask | bMask;
    while (bits) {
      depth++;
      bits = bits >> 1;
    }

    // Check that the depth & bpp are valid
    if (depth > bpp) {
      vlog.error("depth exceeds bits per pixel!");
      bpp = depth;
    }

    // Correct the bits-per-pixel to something we're happy with
    if (bpp <= 16)
      bpp = 16;
    else if (bpp <= 32)
      bpp = 32;
  } else {
    // Palettised format - depth reflects number of colours,
    // but bits-per-pixel is ALWAYS 8
    depth = bpp;
    if (bpp < 8)
      bpp = 8;
    vlog.info("%d-colour palettised", 1<<depth);
    // Aren't really used, but set them to keep the compiler happy
    redMax = redShift = 0;
    greenMax = greenShift = 0;
    blueMax = blueShift = 0;
  }


  return PixelFormat(bpp, depth, bigEndian, trueColour,
		                 redMax, greenMax, blueMax,
		                 redShift, greenShift, blueShift);
}

Rect DeviceContext::getClipBox() const {
  return getClipBox(dc);
}

Rect DeviceContext::getClipBox(HDC dc) {
  // Get the display dimensions
  RECT cr;
  if (!GetClipBox(dc, &cr))
    throw rdr::SystemException("GetClipBox", GetLastError());
  return Rect(cr.left, cr.top, cr.right, cr.bottom);
}


DeviceDC::DeviceDC(const TCHAR* deviceName) {
  dc = ::CreateDC(_T("DISPLAY"), deviceName, NULL, NULL);
  if (!dc)
    throw rdr::SystemException("failed to create DeviceDC", GetLastError());
}

DeviceDC::~DeviceDC() {
  if (dc)
    DeleteDC(dc);
}


WindowDC::WindowDC(HWND wnd) : hwnd(wnd) {
  dc = GetDC(wnd);
  if (!dc)
    throw rdr::SystemException("GetDC failed", GetLastError());
}

WindowDC::~WindowDC() {
  if (dc)
    ReleaseDC(hwnd, dc);
}


CompatibleDC::CompatibleDC(HDC existing) {
  dc = CreateCompatibleDC(existing);
  if (!dc)
    throw rdr::SystemException("CreateCompatibleDC failed", GetLastError());
}

CompatibleDC::~CompatibleDC() {
  if (dc)
    DeleteDC(dc);
}


BitmapDC::BitmapDC(HDC hdc, HBITMAP hbitmap) : CompatibleDC(hdc){
  oldBitmap = (HBITMAP)SelectObject(dc, hbitmap);
  if (!oldBitmap)
    throw rdr::SystemException("SelectObject to CompatibleDC failed",
    GetLastError());
}

BitmapDC::~BitmapDC() {
  SelectObject(dc, oldBitmap);
}
