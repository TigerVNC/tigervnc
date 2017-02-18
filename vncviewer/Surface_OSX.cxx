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

#include <assert.h>

#include <ApplicationServices/ApplicationServices.h>

#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#include <rdr/Exception.h>

#include "Surface.h"

static void render(CGContextRef gc, CGImageRef image,
                   CGBlendMode mode, CGFloat alpha,
                   int src_x, int src_y, int src_w, int src_h,
                   int x, int y, int w, int h)
{
  CGRect rect;

  CGContextSaveGState(gc);

  CGContextSetBlendMode(gc, mode);
  CGContextSetAlpha(gc, alpha);

  // We have to use clipping to partially display an image
  rect.origin.x = x;
  rect.origin.y = y;
  rect.size.width = w;
  rect.size.height = h;

  CGContextClipToRect(gc, rect);

  rect.origin.x = x - src_x;
  rect.origin.y = y - src_y;
  rect.size.width = src_w;
  rect.size.height = src_h;

  CGContextDrawImage(gc, rect, image);

  CGContextRestoreGState(gc);
}

static CGContextRef make_bitmap(int width, int height, unsigned char* data)
{
  CGColorSpaceRef lut;
  CGContextRef bitmap;

  lut = CGDisplayCopyColorSpace(kCGDirectMainDisplay);
  if (!lut) {
    lut = CGColorSpaceCreateDeviceRGB();
    if (!lut)
      throw rdr::Exception("CGColorSpaceCreateDeviceRGB");
  }

  bitmap = CGBitmapContextCreate(data, width, height, 8, width*4, lut,
                                 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  CGColorSpaceRelease(lut);
  if (!bitmap)
    throw rdr::Exception("CGBitmapContextCreate");

  return bitmap;
}

void Surface::clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  unsigned char* out;
  int x, y;

  r = (unsigned)r * a / 255;
  g = (unsigned)g * a / 255;
  b = (unsigned)b * a / 255;

  out = data;
  for (y = 0;y < width();y++) {
    for (x = 0;x < height();x++) {
      *out++ = b;
      *out++ = g;
      *out++ = r;
      *out++ = a;
    }
  }
}

void Surface::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  CGContextSaveGState(fl_gc);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(fl_gc, CGAffineTransformInvert(CGContextGetCTM(fl_gc)));

  // macOS Coordinates are from bottom left, not top left
  src_y = height() - (src_y + h);
  y = Fl_Window::current()->h() - (y + h);

  render(fl_gc, image, kCGBlendModeCopy, 1.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRestoreGState(fl_gc);
}

void Surface::draw(Surface* dst, int src_x, int src_y, int x, int y, int w, int h)
{
  CGContextRef bitmap;

  bitmap = make_bitmap(dst->width(), dst->height(), dst->data);

  // macOS Coordinates are from bottom left, not top left
  src_y = height() - (src_y + h);
  y = dst->height() - (y + h);

  render(bitmap, image, kCGBlendModeCopy, 1.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRelease(bitmap);
}

void Surface::blend(int src_x, int src_y, int x, int y, int w, int h, int a)
{
  CGContextSaveGState(fl_gc);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(fl_gc, CGAffineTransformInvert(CGContextGetCTM(fl_gc)));

  // macOS Coordinates are from bottom left, not top left
  src_y = height() - (src_y + h);
  y = Fl_Window::current()->h() - (y + h);

  render(fl_gc, image, kCGBlendModeNormal, (CGFloat)a/255.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRestoreGState(fl_gc);
}

void Surface::blend(Surface* dst, int src_x, int src_y, int x, int y, int w, int h, int a)
{
  CGContextRef bitmap;

  bitmap = make_bitmap(dst->width(), dst->height(), dst->data);

  // macOS Coordinates are from bottom left, not top left
  src_y = height() - (src_y + h);
  y = dst->height() - (y + h);

  render(bitmap, image, kCGBlendModeNormal, (CGFloat)a/255.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRelease(bitmap);
}

void Surface::alloc()
{
  CGColorSpaceRef lut;
  CGDataProviderRef provider;

  data = new unsigned char[width() * height() * 4];

  lut = CGDisplayCopyColorSpace(kCGDirectMainDisplay);
  if (!lut) {
    lut = CGColorSpaceCreateDeviceRGB();
    if (!lut)
      throw rdr::Exception("CGColorSpaceCreateDeviceRGB");
  }

  provider = CGDataProviderCreateWithData(NULL, data,
                                          width() * height() * 4, NULL);
  if (!provider)
    throw rdr::Exception("CGDataProviderCreateWithData");

  image = CGImageCreate(width(), height(), 8, 32, width() * 4, lut,
                        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little,
                        provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(lut);
  CGDataProviderRelease(provider);
  if (!image)
    throw rdr::Exception("CGImageCreate");
}

void Surface::dealloc()
{
  CGImageRelease(image);
  delete [] data;
}

void Surface::update(const Fl_RGB_Image* image)
{
  int x, y;
  const unsigned char* in;
  unsigned char* out;

  assert(image->w() == width());
  assert(image->h() == height());

  // Convert data and pre-multiply alpha
  in = (const unsigned char*)image->data()[0];
  out = data;
  for (y = 0;y < image->h();y++) {
    for (x = 0;x < image->w();x++) {
      switch (image->d()) {
      case 1:
        *out++ = in[0];
        *out++ = in[0];
        *out++ = in[0];
        *out++ = 0xff;
        break;
      case 2:
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = in[1];
        break;
      case 3:
        *out++ = in[2];
        *out++ = in[1];
        *out++ = in[0];
        *out++ = 0xff;
        break;
      case 4:
        *out++ = (unsigned)in[2] * in[3] / 255;
        *out++ = (unsigned)in[1] * in[3] / 255;
        *out++ = (unsigned)in[0] * in[3] / 255;
        *out++ = in[3];
        break;
      }
      in += image->d();
    }
    if (image->ld() != 0)
      in += image->ld() - image->w() * image->d();
  }
}
