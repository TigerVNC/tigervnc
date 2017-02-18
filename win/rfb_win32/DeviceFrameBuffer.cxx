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

// -=- DeviceFrameBuffer.cxx
//
// The DeviceFrameBuffer class encapsulates the pixel data of the system
// display.

#include <vector>
#include <rfb_win32/DeviceFrameBuffer.h>
#include <rfb_win32/DeviceContext.h>
#include <rfb_win32/IconInfo.h>
#include <rfb/VNCServer.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("DeviceFrameBuffer");

BoolParameter DeviceFrameBuffer::useCaptureBlt("UseCaptureBlt",
  "Use a slower capture method that ensures that alpha blended windows appear correctly",
  true);


// -=- DeviceFrameBuffer class

DeviceFrameBuffer::DeviceFrameBuffer(HDC deviceContext, const Rect& wRect)
  : DIBSectionBuffer(deviceContext), device(deviceContext), cursorBm(deviceContext),
    ignoreGrabErrors(false)
{

  // -=- Firstly, let's check that the device has suitable capabilities

  int capabilities = GetDeviceCaps(device, RASTERCAPS);
  if (!(capabilities & RC_BITBLT)) {
    throw Exception("device does not support BitBlt");
  }
  if (!(capabilities & RC_DI_BITMAP)) {
    throw Exception("device does not support GetDIBits");
  }
  /*
  if (GetDeviceCaps(device, PLANES) != 1) {
    throw Exception("device does not support planar displays");
  }
  */

  // -=- Get the display dimensions and pixel format

  // Get the display dimensions
  deviceCoords = DeviceContext::getClipBox(device);
  if (!wRect.is_empty())
    deviceCoords = wRect.translate(deviceCoords.tl);
  int w = deviceCoords.width();
  int h = deviceCoords.height();

  // We can't handle uneven widths :(
  if (w % 2) w--;

  // Configure the underlying DIB to match the device
  DIBSectionBuffer::setPF(DeviceContext::getPF(device));
  DIBSectionBuffer::setSize(w, h);

  // Configure the cursor buffer
  cursorBm.setPF(format);
}

DeviceFrameBuffer::~DeviceFrameBuffer() {
}


void
DeviceFrameBuffer::setPF(const PixelFormat &pf) {
  throw Exception("setPF not supported");
}

void
DeviceFrameBuffer::setSize(int w, int h) {
  throw Exception("setSize not supported");
}


#ifndef CAPTUREBLT
#define CAPTUREBLT 0x40000000
#endif

void
DeviceFrameBuffer::grabRect(const Rect &rect) {
  BitmapDC tmpDC(device, bitmap);

  // Map the rectangle coords from VNC Desktop-relative to device relative - usually (0,0)
  Point src = desktopToDevice(rect.tl);

  if (!::BitBlt(tmpDC, rect.tl.x, rect.tl.y,
                rect.width(), rect.height(), device, src.x, src.y,
                useCaptureBlt ? (CAPTUREBLT | SRCCOPY) : SRCCOPY)) {
    if (ignoreGrabErrors)
      vlog.error("BitBlt failed:%ld", GetLastError());
    else
      throw rdr::SystemException("BitBlt failed", GetLastError());
  }
}

void
DeviceFrameBuffer::grabRegion(const Region &rgn) {
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator i;
  rgn.get_rects(&rects);
  for(i=rects.begin(); i!=rects.end(); i++) {
    grabRect(*i);
  }
  ::GdiFlush();
}


void DeviceFrameBuffer::setCursor(HCURSOR hCursor, VNCServer* server)
{
  // - If hCursor is null then there is no cursor - clear the old one

  if (hCursor == 0) {
    server->setCursor(0, 0, Point(), 0, 0);
    return;
  }

  try {

    const rdr::U8* buffer;
    rdr::U8* rwbuffer;
    int stride;

    // - Get the size and other details about the cursor.

    IconInfo iconInfo((HICON)hCursor);

    BITMAP maskInfo;
    if (!GetObject(iconInfo.hbmMask, sizeof(BITMAP), &maskInfo))
      throw rdr::SystemException("GetObject() failed", GetLastError());
    if (maskInfo.bmPlanes != 1)
      throw rdr::Exception("unsupported multi-plane cursor");
    if (maskInfo.bmBitsPixel != 1)
      throw rdr::Exception("unsupported cursor mask format");

    // - Create the cursor pixel buffer and mask storage
    //   NB: The cursor pixel buffer is NOT used here.  Instead, we
    //   pass the cursorBm.data pointer directly, to save overhead.

    cursor.setSize(maskInfo.bmWidth, maskInfo.bmHeight);
    cursor.setPF(format);
    cursor.hotspot = Point(iconInfo.xHotspot, iconInfo.yHotspot);

    // - Get the AND and XOR masks.  There is only an XOR mask if this is not a
    // colour cursor.

    if (!iconInfo.hbmColor)
      cursor.setSize(cursor.width(), cursor.height() / 2);
    rdr::U8Array mask(maskInfo.bmWidthBytes * maskInfo.bmHeight);
    rdr::U8* xorMask = mask.buf + cursor.height() * maskInfo.bmWidthBytes;

    if (!GetBitmapBits(iconInfo.hbmMask,
                       maskInfo.bmWidthBytes * maskInfo.bmHeight, mask.buf))
      throw rdr::SystemException("GetBitmapBits failed", GetLastError());

    // Configure the cursor bitmap
    cursorBm.setSize(cursor.width(), cursor.height());

    // Draw the cursor into the bitmap
    BitmapDC dc(device, cursorBm.bitmap);
    if (!DrawIconEx(dc, 0, 0, hCursor, 0, 0, 0, NULL, DI_NORMAL | DI_COMPAT))
      throw rdr::SystemException("unable to render cursor", GetLastError());

    // Replace any XORed pixels with xorColour, because RFB doesn't support
    // XORing of cursors.  XORing is used for the I-beam cursor, which is most
    // often used over a white background, but also sometimes over a black
    // background.  We set the XOR'd pixels to black, then draw a white outline
    // around the whole cursor.

    // *** should we replace any pixels not set in mask to zero, to ensure
    // that irrelevant data doesn't screw compression?

    bool doOutline = false;
    if (!iconInfo.hbmColor) {
      rwbuffer = cursorBm.getBufferRW(cursorBm.getRect(), &stride);
      Pixel xorColour = format.pixelFromRGB((rdr::U16)0, (rdr::U16)0, (rdr::U16)0);
      for (int y = 0; y < cursor.height(); y++) {
        for (int x = 0; x < cursor.width(); x++) {
          int byte = y * maskInfo.bmWidthBytes + x / 8;
          int bit = 7 - x % 8;
          if ((mask.buf[byte] & (1 << bit)) && (xorMask[byte] & (1 << bit)))
          {
            mask.buf[byte] &= ~(1 << bit);

            switch (format.bpp) {
            case 8:
              rwbuffer[y * cursor.width() + x] = xorColour;  break;
            case 16:
              rwbuffer[y * cursor.width() + x] = xorColour; break;
            case 32:
              rwbuffer[y * cursor.width() + x] = xorColour; break;
            }

            doOutline = true;
          }
        }
      }
      cursorBm.commitBufferRW(cursorBm.getRect());
    }

    // Finally invert the AND mask so it's suitable for RFB and pack it into
    // the minimum number of bytes per row.

    int maskBytesPerRow = (cursor.width() + 7) / 8;

    for (int j = 0; j < cursor.height(); j++) {
      for (int i = 0; i < maskBytesPerRow; i++)
        cursor.mask.buf[j * maskBytesPerRow + i]
          = ~mask.buf[j * maskInfo.bmWidthBytes + i];
    }

    if (doOutline) {
      vlog.debug("drawing cursor outline!");

      buffer = cursorBm.getBuffer(cursorBm.getRect(), &stride);
      cursor.imageRect(cursorBm.getRect(), buffer, stride);

      cursor.drawOutline(format.pixelFromRGB((rdr::U16)0xffff, (rdr::U16)0xffff, (rdr::U16)0xffff));

      buffer = cursor.getBuffer(cursor.getRect(), &stride);
      cursorBm.imageRect(cursor.getRect(), buffer, stride);
    }

    buffer = cursorBm.getBuffer(cursorBm.getRect(), &stride);
    server->setCursor(cursor.width(), cursor.height(), cursor.hotspot,
                      buffer, cursor.mask.buf);

  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
  }
}
