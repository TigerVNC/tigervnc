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
#include <stdlib.h>

#include <FL/Fl_RGB_Image.H>
#include <FL/x.H>

#include <rdr/Exception.h>

#include "Surface.h"

void Surface::clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  XRenderColor color;

  color.red = (unsigned)r * 65535 / 255 * a / 255;
  color.green = (unsigned)g * 65535 / 255 * a / 255;
  color.blue = (unsigned)b * 65535 / 255 * a / 255;
  color.alpha = (unsigned)a * 65535 / 255;

  XRenderFillRectangle(fl_display, PictOpSrc, picture, &color,
                       0, 0, width(), height());
}

void Surface::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  Picture winPict;

  winPict = XRenderCreatePicture(fl_display, fl_window, visFormat, 0, NULL);
  XRenderComposite(fl_display, PictOpSrc, picture, None, winPict,
                   src_x, src_y, 0, 0, x, y, w, h);
  XRenderFreePicture(fl_display, winPict);
}

void Surface::draw(Surface* dst, int src_x, int src_y, int x, int y, int w, int h)
{
  XRenderComposite(fl_display, PictOpSrc, picture, None, dst->picture,
                   src_x, src_y, 0, 0, x, y, w, h);
}

static Picture alpha_mask(int a)
{
  Pixmap pixmap;
  XRenderPictFormat* format;
  XRenderPictureAttributes rep;
  Picture pict;
  XRenderColor color;

  if (a == 255)
    return None;

  pixmap = XCreatePixmap(fl_display, XDefaultRootWindow(fl_display),
                         1, 1, 8);

  format = XRenderFindStandardFormat(fl_display, PictStandardA8);
  rep.repeat = RepeatNormal;
  pict = XRenderCreatePicture(fl_display, pixmap, format, CPRepeat, &rep);
  XFreePixmap(fl_display, pixmap);

  color.alpha = (unsigned)a * 65535 / 255;

  XRenderFillRectangle(fl_display, PictOpSrc, pict, &color,
                       0, 0, 1, 1);

  return pict;
}

void Surface::blend(int src_x, int src_y, int x, int y, int w, int h, int a)
{
  Picture winPict, alpha;

  winPict = XRenderCreatePicture(fl_display, fl_window, visFormat, 0, NULL);
  alpha = alpha_mask(a);
  XRenderComposite(fl_display, PictOpOver, picture, alpha, winPict,
                   src_x, src_y, 0, 0, x, y, w, h);
  XRenderFreePicture(fl_display, winPict);

  if (alpha != None)
    XRenderFreePicture(fl_display, alpha);
}

void Surface::blend(Surface* dst, int src_x, int src_y, int x, int y, int w, int h, int a)
{
  Picture alpha;

  alpha = alpha_mask(a);
  XRenderComposite(fl_display, PictOpOver, picture, alpha, dst->picture,
                   src_x, src_y, 0, 0, x, y, w, h);
  if (alpha != None)
    XRenderFreePicture(fl_display, alpha);
}


void Surface::alloc()
{
  XRenderPictFormat templ;
  XRenderPictFormat* format;

  // Might not be open at this point
  fl_open_display();

  pixmap = XCreatePixmap(fl_display, XDefaultRootWindow(fl_display),
                         width(), height(), 32);

  // Our code assumes a BGRA byte order, regardless of what the endian
  // of the machine is or the native byte order of XImage, so make sure
  // we find such a format
  templ.type = PictTypeDirect;
  templ.depth = 32;
  if (XImageByteOrder(fl_display) == MSBFirst) {
    templ.direct.alpha = 0;
    templ.direct.red   = 8;
    templ.direct.green = 16;
    templ.direct.blue  = 24;
  } else {
    templ.direct.alpha = 24;
    templ.direct.red   = 16;
    templ.direct.green = 8;
    templ.direct.blue  = 0;
  }
  templ.direct.alphaMask = 0xff;
  templ.direct.redMask = 0xff;
  templ.direct.greenMask = 0xff;
  templ.direct.blueMask = 0xff;

  format = XRenderFindFormat(fl_display, PictFormatType | PictFormatDepth |
                             PictFormatRed | PictFormatRedMask |
                             PictFormatGreen | PictFormatGreenMask |
                             PictFormatBlue | PictFormatBlueMask |
                             PictFormatAlpha | PictFormatAlphaMask,
                             &templ, 0);

  if (!format)
    throw rdr::Exception("XRenderFindFormat");

  picture = XRenderCreatePicture(fl_display, pixmap, format, 0, NULL);

  visFormat = XRenderFindVisualFormat(fl_display, fl_visual->visual);
}

void Surface::dealloc()
{
  XRenderFreePicture(fl_display, picture);
  XFreePixmap(fl_display, pixmap);
}

void Surface::update(const Fl_RGB_Image* image)
{
  XImage* img;
  GC gc;

  int x, y;
  const unsigned char* in;
  unsigned char* out;

  assert(image->w() == width());
  assert(image->h() == height());

  img = XCreateImage(fl_display, CopyFromParent, 32,
                     ZPixmap, 0, NULL, width(), height(),
                     32, 0);
  if (!img)
    throw rdr::Exception("XCreateImage");

  img->data = (char*)malloc(img->bytes_per_line * img->height);
  if (!img->data)
    throw rdr::Exception("malloc");

  // Convert data and pre-multiply alpha
  in = (const unsigned char*)image->data()[0];
  out = (unsigned char*)img->data;
  for (y = 0;y < img->height;y++) {
    for (x = 0;x < img->width;x++) {
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

  gc = XCreateGC(fl_display, pixmap, 0, NULL);
  XPutImage(fl_display, pixmap, gc, img,
            0, 0, 0, 0, img->width, img->height);
  XFreeGC(fl_display, gc);

  XDestroyImage(img);
}

