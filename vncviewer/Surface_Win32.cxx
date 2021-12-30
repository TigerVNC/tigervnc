/* Copyright 2016 Pierre Ossman for Cendio AB
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

#include <FL/Fl_RGB_Image.H>
#include <FL/x.H>

#include <rdr/Exception.h>

#include "Surface.h"

void Surface::clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  RGBQUAD* out;
  int x, y;

  r = (unsigned)r * a / 255;
  g = (unsigned)g * a / 255;
  b = (unsigned)b * a / 255;

  out = data;
  for (y = 0;y < width();y++) {
    for (x = 0;x < height();x++) {
      out->rgbRed = r;
      out->rgbGreen = g;
      out->rgbBlue = b;
      out->rgbReserved = a;
      out++;
    }
  }
}

void Surface::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  HDC dc;

  dc = CreateCompatibleDC(fl_gc);
  if (!dc)
    throw rdr::SystemException("CreateCompatibleDC", GetLastError());

  if (!SelectObject(dc, bitmap))
    throw rdr::SystemException("SelectObject", GetLastError());

  if (!BitBlt(fl_gc, x, y, w, h, dc, src_x, src_y, SRCCOPY)) {
    // If the desktop we're rendering to is inactive (like when the screen
    // is locked or the UAC is active), then GDI calls will randomly fail.
    // This is completely undocumented so we have no idea how best to deal
    // with it. For now, we've only seen this error and for this function
    // so only ignore this combination.
    if (GetLastError() != ERROR_INVALID_HANDLE)
      throw rdr::SystemException("BitBlt", GetLastError());
  }

  DeleteDC(dc);
}

void Surface::draw(Surface* dst, int src_x, int src_y, int x, int y, int w, int h)
{
  HDC origdc, dstdc;

  dstdc = CreateCompatibleDC(NULL);
  if (!dstdc)
    throw rdr::SystemException("CreateCompatibleDC", GetLastError());

  if (!SelectObject(dstdc, dst->bitmap))
    throw rdr::SystemException("SelectObject", GetLastError());

  origdc = fl_gc;
  fl_gc = dstdc;
  draw(src_x, src_y, x, y, w, h);
  fl_gc = origdc;

  DeleteDC(dstdc);
}

void Surface::blend(int src_x, int src_y, int x, int y, int w, int h, int a)
{
  // Compositing doesn't work properly for window DC:s
  assert(false);
}

void Surface::blend(Surface* dst, int src_x, int src_y, int x, int y, int w, int h, int a)
{
  HDC dstdc, srcdc;
  BLENDFUNCTION blend;

  dstdc = CreateCompatibleDC(NULL);
  if (!dstdc)
    throw rdr::SystemException("CreateCompatibleDC", GetLastError());
  srcdc = CreateCompatibleDC(NULL);
  if (!srcdc)
    throw rdr::SystemException("CreateCompatibleDC", GetLastError());

  if (!SelectObject(dstdc, dst->bitmap))
    throw rdr::SystemException("SelectObject", GetLastError());
  if (!SelectObject(srcdc, bitmap))
    throw rdr::SystemException("SelectObject", GetLastError());

  blend.BlendOp = AC_SRC_OVER;
  blend.BlendFlags = 0;
  blend.SourceConstantAlpha = a;
  blend.AlphaFormat = AC_SRC_ALPHA;

  if (!AlphaBlend(dstdc, x, y, w, h, srcdc, src_x, src_y, w, h, blend)) {
    // If the desktop we're rendering to is inactive (like when the screen
    // is locked or the UAC is active), then GDI calls will randomly fail.
    // This is completely undocumented so we have no idea how best to deal
    // with it. For now, we've only seen this error and for this function
    // so only ignore this combination.
    if (GetLastError() != ERROR_INVALID_HANDLE)
      throw rdr::SystemException("BitBlt", GetLastError());
  }

  DeleteDC(srcdc);
  DeleteDC(dstdc);
}

void Surface::alloc()
{
  BITMAPINFOHEADER bih;

  data = new RGBQUAD[width() * height()];

  memset(&bih, 0, sizeof(bih));

  bih.biSize         = sizeof(BITMAPINFOHEADER);
  bih.biBitCount     = 32;
  bih.biPlanes       = 1;
  bih.biWidth        = width();
  bih.biHeight       = -height(); // Negative to get top-down
  bih.biCompression  = BI_RGB;

  bitmap = CreateDIBSection(NULL, (BITMAPINFO*)&bih,
                            DIB_RGB_COLORS, (void**)&data, NULL, 0);
  if (!bitmap)
    throw rdr::SystemException("CreateDIBSection", GetLastError());
}

void Surface::dealloc()
{
  DeleteObject(bitmap);
}

void Surface::update(const Fl_RGB_Image* image)
{
  const unsigned char* in;
  RGBQUAD* out;
  int x, y;

  assert(image->w() == width());
  assert(image->h() == height());

  // Convert data and pre-multiply alpha
  in = (const unsigned char*)image->data()[0];
  out = data;
  for (y = 0;y < image->w();y++) {
    for (x = 0;x < image->h();x++) {
      switch (image->d()) {
      case 1:
        out->rgbBlue = in[0];
        out->rgbGreen = in[0];
        out->rgbRed = in[0];
        out->rgbReserved = 0xff;
        break;
      case 2:
        out->rgbBlue = (unsigned)in[0] * in[1] / 255;
        out->rgbGreen = (unsigned)in[0] * in[1] / 255;
        out->rgbRed = (unsigned)in[0] * in[1] / 255;
        out->rgbReserved = in[1];
        break;
      case 3:
        out->rgbBlue = in[2];
        out->rgbGreen = in[1];
        out->rgbRed = in[0];
        out->rgbReserved = 0xff;
        break;
      case 4:
        out->rgbBlue = (unsigned)in[2] * in[3] / 255;
        out->rgbGreen = (unsigned)in[1] * in[3] / 255;
        out->rgbRed = (unsigned)in[0] * in[3] / 255;
        out->rgbReserved = in[3];
        break;
      }
      in += image->d();
      out++;
    }
    if (image->ld() != 0)
      in += image->ld() - image->w() * image->d();
  }
}

