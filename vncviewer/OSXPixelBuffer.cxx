/* Copyright 2011-2014 Pierre Ossman for Cendio AB
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

#include <ApplicationServices/ApplicationServices.h>

#include <FL/Fl_Window.H>
#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include "i18n.h"
#include "OSXPixelBuffer.h"

using namespace rfb;

static rfb::LogWriter vlog("OSXPixelBuffer");

OSXPixelBuffer::OSXPixelBuffer(int width, int height) :
  PlatformPixelBuffer(rfb::PixelFormat(32, 24, false, true,
                                       255, 255, 255, 16, 8, 0),
                      width, height, NULL, width),
  image(NULL)
{
  CGColorSpaceRef lut;
  CGDataProviderRef provider;

  data = new rdr::U8[width * height * format.bpp/8];
  if (data == NULL)
    throw rfb::Exception(_("Not enough memory for framebuffer"));

  lut = CGDisplayCopyColorSpace(kCGDirectMainDisplay);
  if (!lut) {
    vlog.error(_("Could not get screen color space. Using slower fallback."));
    lut = CGColorSpaceCreateDeviceRGB();
    if (!lut)
      throw rfb::Exception("CGColorSpaceCreateDeviceRGB");
  }

  provider = CGDataProviderCreateWithData(NULL, data,
                                          width * height * 4, NULL);
  if (!provider)
    throw rfb::Exception("CGDataProviderCreateWithData");

  image = CGImageCreate(width, height, 8, 32, width * 4, lut,
                        kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
                        provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(lut);
  CGDataProviderRelease(provider);
  if (!image)
    throw rfb::Exception("CGImageCreate");
}


OSXPixelBuffer::~OSXPixelBuffer()
{
  CGImageRelease(image);
  delete [] data;
}


void OSXPixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  CGContextRef gc;
  CGRect rect;

  gc = fl_gc;

  CGContextSaveGState(gc);

  // macOS Coordinates are from bottom left, not top left
  src_y = height() - (src_y + h);
  y = Fl_Window::current()->h() - (y + h);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(gc, CGAffineTransformInvert(CGContextGetCTM(gc)));

  // We have to use clipping to partially display an image
  rect.origin.x = x;
  rect.origin.y = y;
  rect.size.width = w;
  rect.size.height = h;

  CGContextClipToRect(gc, rect);

  rect.origin.x = x - src_x;
  rect.origin.y = y - src_y;
  rect.size.width = width();
  rect.size.height = height();

  CGContextDrawImage(gc, rect, image);

  CGContextRestoreGState(gc);
}
