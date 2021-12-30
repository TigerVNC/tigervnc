/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2005 Constantin Kaplinsky.  All Rights Reserved.
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
//
// Image.cxx
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <rfb/LogWriter.h>
#include <x0vncserver/Image.h>

//
// ImageCleanup is used to delete Image instances automatically on
// program shutdown. This is important for shared memory images.
//

#include <list>

class ImageCleanup {
public:
  std::list<Image *> images;

  ~ImageCleanup()
  {
    while (!images.empty()) {
      delete images.front();
    }
  }
};

ImageCleanup imageCleanup;

//
// Image class implementation.
//

static rfb::LogWriter vlog("Image");

Image::Image(Display *d)
  : xim(NULL), dpy(d)
{
  imageCleanup.images.push_back(this);
}

Image::Image(Display *d, int width, int height)
  : xim(NULL), dpy(d)
{
  imageCleanup.images.push_back(this);
  Init(width, height);
}

void Image::Init(int width, int height)
{
  Visual* vis = DefaultVisual(dpy, DefaultScreen(dpy));

  if (vis->c_class != TrueColor) {
    vlog.error("pseudocolour not supported");
    exit(1);
  }

  xim = XCreateImage(dpy, vis, DefaultDepth(dpy, DefaultScreen(dpy)),
                     ZPixmap, 0, 0, width, height, BitmapPad(dpy), 0);

  xim->data = (char *)malloc(xim->bytes_per_line * xim->height);
  if (xim->data == NULL) {
    vlog.error("malloc() failed");
    exit(1);
  }
}

Image::~Image()
{
  imageCleanup.images.remove(this);

  // XDestroyImage will free xim->data if necessary
  if (xim != NULL)
    XDestroyImage(xim);
}

void Image::get(Window wnd, int x, int y)
{
  get(wnd, x, y, xim->width, xim->height);
}

void Image::get(Window wnd, int x, int y, int w, int h,
                int dst_x, int dst_y)
{
  XGetSubImage(dpy, wnd, x, y, w, h, AllPlanes, ZPixmap, xim, dst_x, dst_y);
}

//
// Copying pixels from one image to another.
//
// FIXME: Use Point and Rect structures?
// FIXME: Too many similar methods?
//

inline
void Image::copyPixels(XImage *src,
                       int dst_x, int dst_y,
                       int src_x, int src_y,
                       int w, int h)
{
  const char *srcOffset =
    src->data + (src_y * src->bytes_per_line +
                 src_x * (src->bits_per_pixel / 8));
  char *dstOffset =
    xim->data + (dst_y * xim->bytes_per_line +
                 dst_x * (xim->bits_per_pixel / 8));

  int rowLength = w * (xim->bits_per_pixel / 8);

  for (int i = 0; i < h ; i++) {
    memcpy(dstOffset, srcOffset, rowLength);
    srcOffset += src->bytes_per_line;
    dstOffset += xim->bytes_per_line;
  }
}

void Image::updateRect(XImage *src, int dst_x, int dst_y)
{
  // Limit width and height at destination image size.
  int w = src->width;
  if (dst_x + w > xim->width)
    w = xim->width - dst_x;
  int h = src->height;
  if (dst_y + h > xim->height)
    h = xim->height - dst_y;

  copyPixels(src, dst_x, dst_y, 0, 0, w, h);
}

void Image::updateRect(Image *src, int dst_x, int dst_y)
{
  updateRect(src->xim, dst_x, dst_y);
}

void Image::updateRect(XImage *src, int dst_x, int dst_y, int w, int h)
{
  // Correct width and height if necessary.
  if (w > src->width)
    w = src->width;
  if (dst_x + w > xim->width)
    w = xim->width - dst_x;
  if (h > src->height)
    h = src->height;
  if (dst_y + h > xim->height)
    h = xim->height - dst_y;

  copyPixels(src, dst_x, dst_y, 0, 0, w, h);
}

void Image::updateRect(Image *src, int dst_x, int dst_y, int w, int h)
{
  updateRect(src->xim, dst_x, dst_y, w, h);
}

void Image::updateRect(XImage *src, int dst_x, int dst_y,
                       int src_x, int src_y, int w, int h)
{
  // Correct width and height if necessary.
  if (src_x + w > src->width)
    w = src->width - src_x;
  if (dst_x + w > xim->width)
    w = xim->width - dst_x;
  if (src_y + h > src->height)
    h = src->height - src_y;
  if (dst_y + h > xim->height)
    h = xim->height - dst_y;

  copyPixels(src, dst_x, dst_y, src_x, src_y, w, h);
}

void Image::updateRect(Image *src, int dst_x, int dst_y,
                       int src_x, int src_y, int w, int h)
{
  updateRect(src->xim, dst_x, dst_y, src_x, src_y, w, h);
}

//
// ShmImage class implementation.
//

static bool caughtShmError = false;

static int ShmCreationXErrorHandler(Display *dpy, XErrorEvent *error)
{
  caughtShmError = true;
  return 0;
}

ShmImage::ShmImage(Display *d)
  : Image(d), shminfo(NULL)
{
}

ShmImage::ShmImage(Display *d, int width, int height)
  : Image(d), shminfo(NULL)
{
  Init(width, height);
}

// FIXME: Remove duplication of cleanup operations.
void ShmImage::Init(int width, int height, const XVisualInfo *vinfo)
{
  int major, minor;
  Bool pixmaps;

  if (!XShmQueryVersion(dpy, &major, &minor, &pixmaps)) {
    vlog.error("XShmQueryVersion() failed");
    return;
  }

  Visual *visual;
  int depth;

  if (vinfo == NULL) {
    visual = DefaultVisual(dpy, DefaultScreen(dpy));
    depth = DefaultDepth(dpy, DefaultScreen(dpy));
  } else {
    visual = vinfo->visual;
    depth = vinfo->depth;
  }

  if (visual->c_class != TrueColor) {
    vlog.error("pseudocolour not supported");
    exit(1);
  }

  shminfo = new XShmSegmentInfo;

  xim = XShmCreateImage(dpy, visual, depth, ZPixmap, 0, shminfo,
			width, height);
  if (xim == NULL) {
    vlog.error("XShmCreateImage() failed");
    delete shminfo;
    shminfo = NULL;
    return;
  }

  shminfo->shmid = shmget(IPC_PRIVATE,
                          xim->bytes_per_line * xim->height,
                          IPC_CREAT|0777);
  if (shminfo->shmid == -1) {
    perror("shmget");
    vlog.error("shmget() failed (%d bytes requested)",
               int(xim->bytes_per_line * xim->height));
    XDestroyImage(xim);
    xim = NULL;
    delete shminfo;
    shminfo = NULL;
    return;
  }

  shminfo->shmaddr = xim->data = (char *)shmat(shminfo->shmid, 0, 0);
  if (shminfo->shmaddr == (char *)-1) {
    perror("shmat");
    vlog.error("shmat() failed (%d bytes requested)",
               int(xim->bytes_per_line * xim->height));
    shmctl(shminfo->shmid, IPC_RMID, 0);
    XDestroyImage(xim);
    xim = NULL;
    delete shminfo;
    shminfo = NULL;
    return;
  }

  shminfo->readOnly = False;

  XErrorHandler oldHdlr = XSetErrorHandler(ShmCreationXErrorHandler);
  XShmAttach(dpy, shminfo);
  XSync(dpy, False);
  XSetErrorHandler(oldHdlr);
  if (caughtShmError) {
    vlog.error("XShmAttach() failed");
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    XDestroyImage(xim);
    xim = NULL;
    delete shminfo;
    shminfo = NULL;
    return;
  }
}

ShmImage::~ShmImage()
{
  // FIXME: Destroy image as described in MIT-SHM documentation.
  if (shminfo != NULL) {
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    delete shminfo;
  }
}

void ShmImage::get(Window wnd, int x, int y)
{
  XShmGetImage(dpy, wnd, xim, x, y, AllPlanes);
}

void ShmImage::get(Window wnd, int x, int y, int w, int h,
                   int dst_x, int dst_y)
{
  // XShmGetImage is faster, but can only retrieve the entire
  // window. Use it for large reads.
  if (x == dst_x && y == dst_y && (long)w * h > (long)xim->width * xim->height / 4) {
    XShmGetImage(dpy, wnd, xim, 0, 0, AllPlanes);
  } else {
    XGetSubImage(dpy, wnd, x, y, w, h, AllPlanes, ZPixmap, xim, dst_x, dst_y);
  }
}

//
// ImageFactory class implementation
//
// FIXME: Make ImageFactory always create images of the same class?
//

ImageFactory::ImageFactory(bool allowShm)
  : mayUseShm(allowShm)
{
}

ImageFactory::~ImageFactory()
{
}

Image *ImageFactory::newImage(Display *d, int width, int height)
{
  Image *image = NULL;

  // Now, try to use shared memory image.

  if (mayUseShm) {
    image = new ShmImage(d, width, height);
    if (image->xim != NULL) {
      return image;
    }

    delete image;
    vlog.error("Failed to create SHM image, falling back to Xlib image");
  }

  // Fall back to Xlib image.

  image = new Image(d, width, height);
  return image;
}
