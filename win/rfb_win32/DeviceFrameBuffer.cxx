/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2017 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
  : DIBSectionBuffer(deviceContext), device(deviceContext),
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
  initBuffer(DeviceContext::getPF(device), w, h);
}

DeviceFrameBuffer::~DeviceFrameBuffer() {
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
    server->setCursor(0, 0, Point(), NULL);
    return;
  }

  try {

    int width, height;
    rdr::U8Array buffer;

    // - Get the size and other details about the cursor.

    IconInfo iconInfo((HICON)hCursor);

    BITMAP maskInfo;
    if (!GetObject(iconInfo.hbmMask, sizeof(BITMAP), &maskInfo))
      throw rdr::SystemException("GetObject() failed", GetLastError());
    if (maskInfo.bmPlanes != 1)
      throw rdr::Exception("unsupported multi-plane cursor");
    if (maskInfo.bmBitsPixel != 1)
      throw rdr::Exception("unsupported cursor mask format");

    width = maskInfo.bmWidth;
    height = maskInfo.bmHeight;
    if (!iconInfo.hbmColor)
      height /= 2;

    buffer.buf = new rdr::U8[width * height * 4];

    Point hotspot = Point(iconInfo.xHotspot, iconInfo.yHotspot);

    if (iconInfo.hbmColor) {
      // Colour cursor

      BITMAPV5HEADER bi;
      BitmapDC dc(device, iconInfo.hbmColor);

      memset(&bi, 0, sizeof(BITMAPV5HEADER));

      bi.bV5Size        = sizeof(BITMAPV5HEADER);
      bi.bV5Width       = width;
      bi.bV5Height      = -height; // Negative for top-down
      bi.bV5Planes      = 1;
      bi.bV5BitCount    = 32;
      bi.bV5Compression = BI_BITFIELDS;
      bi.bV5RedMask     = 0x000000FF;
      bi.bV5GreenMask   = 0x0000FF00;
      bi.bV5BlueMask    = 0x00FF0000;
      bi.bV5AlphaMask   = 0xFF000000;

      if (!GetDIBits(dc, iconInfo.hbmColor, 0, height,
                     buffer.buf, (LPBITMAPINFO)&bi, DIB_RGB_COLORS))
        throw rdr::SystemException("GetDIBits", GetLastError());

      // We may not get the RGBA order we want, so shuffle things around
      int ridx, gidx, bidx, aidx;

      ridx = __builtin_ffs(bi.bV5RedMask) / 8;
      gidx = __builtin_ffs(bi.bV5GreenMask) / 8;
      bidx = __builtin_ffs(bi.bV5BlueMask) / 8;
      // Usually not set properly
      aidx = 6 - ridx - gidx - bidx;

      if ((bi.bV5RedMask != ((unsigned)0xff << ridx*8)) ||
          (bi.bV5GreenMask != ((unsigned)0xff << gidx*8)) ||
          (bi.bV5BlueMask != ((unsigned)0xff << bidx*8)))
        throw rdr::Exception("unsupported cursor colour format");

      rdr::U8* rwbuffer = buffer.buf;
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          rdr::U8 r, g, b, a;

          r = rwbuffer[ridx];
          g = rwbuffer[gidx];
          b = rwbuffer[bidx];
          a = rwbuffer[aidx];

          rwbuffer[0] = r;
          rwbuffer[1] = g;
          rwbuffer[2] = b;
          rwbuffer[3] = a;

          rwbuffer += 4;
        }
      }
    } else {
      // B/W cursor

      rdr::U8Array mask(maskInfo.bmWidthBytes * maskInfo.bmHeight);
      rdr::U8* andMask = mask.buf;
      rdr::U8* xorMask = mask.buf + height * maskInfo.bmWidthBytes;

      if (!GetBitmapBits(iconInfo.hbmMask,
                         maskInfo.bmWidthBytes * maskInfo.bmHeight, mask.buf))
        throw rdr::SystemException("GetBitmapBits", GetLastError());

      bool doOutline = false;
      rdr::U8* rwbuffer = buffer.buf;
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          int byte = y * maskInfo.bmWidthBytes + x / 8;
          int bit = 7 - x % 8;

          if (!(andMask[byte] & (1 << bit))) {
            // Valid pixel, so make it opaque
            rwbuffer[3] = 0xff;

            // Black or white?
            if (xorMask[byte] & (1 << bit))
              rwbuffer[0] = rwbuffer[1] = rwbuffer[2] = 0xff;
            else
              rwbuffer[0] = rwbuffer[1] = rwbuffer[2] = 0;
          } else if (xorMask[byte] & (1 << bit)) {
            // Replace any XORed pixels with black, because RFB doesn't support
            // XORing of cursors.  XORing is used for the I-beam cursor, which is most
            // often used over a white background, but also sometimes over a black
            // background.  We set the XOR'd pixels to black, then draw a white outline
            // around the whole cursor.

            rwbuffer[0] = rwbuffer[1] = rwbuffer[2] = 0;
            rwbuffer[3] = 0xff;

            doOutline = true;
          } else {
            // Transparent pixel
            rwbuffer[0] = rwbuffer[1] = rwbuffer[2] = rwbuffer[3] = 0;
          }

          rwbuffer += 4;
        }
      }

      if (doOutline) {
        vlog.debug("drawing cursor outline!");

        // The buffer needs to be slightly larger to make sure there
        // is room for the outline pixels
        rdr::U8Array outline((width + 2)*(height + 2)*4);
        memset(outline.buf, 0, (width + 2)*(height + 2)*4);

        // Pass 1, outline everything
        rdr::U8* in = buffer.buf;
        rdr::U8* out = outline.buf + width*4 + 4;
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            // Visible pixel?
            if (in[3] > 0) {
              // Outline above...
              memset(out - (width+2)*4 - 4, 0xff, 4 * 3);
              // ...besides...
              memset(out - 4, 0xff, 4 * 3);
              // ...and above
              memset(out + (width+2)*4 - 4, 0xff, 4 * 3);
            }
            in += 4;
            out += 4;
          }
          // outline is slightly larger
          out += 2*4;
        }

        // Pass 2, overwrite with actual cursor
        in = buffer.buf;
        out = outline.buf + width*4 + 4;
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            if (in[3] > 0)
              memcpy(out, in, 4);
            in += 4;
            out += 4;
          }
          out += 2*4;
        }

        width += 2;
        height += 2;
        hotspot.x += 1;
        hotspot.y += 1;

        delete [] buffer.buf;
        buffer.buf = outline.takeBuf();
      }
    }

    server->setCursor(width, height, hotspot, buffer.buf);

  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
  }
}
