/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <ApplicationServices/ApplicationServices.h>

#include <FL/Fl_Window.H>
#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include "OSXPixelBuffer.h"

using namespace rfb;

static rfb::LogWriter vlog("PlatformPixelBuffer");

PlatformPixelBuffer::PlatformPixelBuffer(int width, int height) :
  ManagedPixelBuffer(rfb::PixelFormat(32, 24, false, true,
                                      255, 255, 255, 16, 8, 0),
                     width, height),
  bitmap(NULL)
{
  CGColorSpaceRef lut;

  lut = CGColorSpaceCreateDeviceRGB();
  assert(lut);

  bitmap = CGBitmapContextCreate(data, width, height, 8, width*4, lut,
                                 kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
  assert(bitmap);

  CGColorSpaceRelease(lut);
}


PlatformPixelBuffer::~PlatformPixelBuffer()
{
  CFRelease((CGContextRef)bitmap);
}


void PlatformPixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  CGRect rect;
  CGContextRef gc;
  CGAffineTransform at;
  CGImageRef image;

  gc = (CGContextRef)fl_gc;

  CGContextSaveGState(gc);

  // We have to use clipping to partially display an image

  rect.origin.x = x - 0.5;
  rect.origin.y = y - 0.5;
  rect.size.width = w;
  rect.size.height = h;

  CGContextClipToRect(gc, rect);
 
  // Oh the hackiness that is OS X image handling...
  // The CGContextDrawImage() routine magically flips the images and offsets
  // them by 0.5,0.5 in order to compensate for OS X unfamiliar coordinate
  // system. But this breaks horribly when you set up the CTM to get the
  // more familiar top-down system (which FLTK does), meaning we have to
  // reset the CTM back to the identity matrix and calculate new origin
  // coordinates.

  at = CGContextGetCTM(gc);
  CGContextScaleCTM(gc, 1, -1);
  CGContextTranslateCTM(gc, -at.tx, -at.ty);

  rect.origin.x = x - src_x;
  rect.origin.y = Fl_Window::current()->h() - (y - src_y);
  rect.size.width = width();
  rect.size.height = -height(); // Negative height does _not_ flip the image

  image = CGBitmapContextCreateImage((CGContextRef)bitmap);
  CGContextDrawImage(gc, rect, image);
  CGImageRelease(image);

  CGContextRestoreGState(gc);
}
