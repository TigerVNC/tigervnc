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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

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
  : xim(NULL), dpy(d), trueColor(true)
{
  imageCleanup.images.push_back(this);
}

Image::Image(Display *d, int width, int height)
  : xim(NULL), dpy(d), trueColor(true)
{
  imageCleanup.images.push_back(this);
  Init(width, height);
}

void Image::Init(int width, int height)
{
  Visual* vis = DefaultVisual(dpy, DefaultScreen(dpy));
  trueColor = (vis->c_class == TrueColor);

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

#ifdef HAVE_MITSHM

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

  trueColor = (visual->c_class == TrueColor);

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
  // FIXME: Use SHM for this as well?
  XGetSubImage(dpy, wnd, x, y, w, h, AllPlanes, ZPixmap, xim, dst_x, dst_y);
}

#ifdef HAVE_READDISPLAY

//
// IrixOverlayShmImage class implementation.
//

IrixOverlayShmImage::IrixOverlayShmImage(Display *d)
  : ShmImage(d), readDisplayBuf(NULL)
{
}

IrixOverlayShmImage::IrixOverlayShmImage(Display *d, int width, int height)
  : ShmImage(d), readDisplayBuf(NULL)
{
  Init(width, height);
}

void IrixOverlayShmImage::Init(int width, int height)
{
  // First determine the pixel format used by XReadDisplay.
  XVisualInfo vinfo;
  if (!getOverlayVisualInfo(&vinfo))
    return;

  // Create an SHM image of the same format.
  ShmImage::Init(width, height, &vinfo);
  if (xim == NULL)
    return;

  // FIXME: Check if the extension is available at run time.
  readDisplayBuf = XShmCreateReadDisplayBuf(dpy, NULL, shminfo, width, height);
}

bool IrixOverlayShmImage::getOverlayVisualInfo(XVisualInfo *vinfo_ret)
{
  // First, get an image in the format returned by XReadDisplay.
  unsigned long hints = 0, hints_ret;
  XImage *testImage = XReadDisplay(dpy, DefaultRootWindow(dpy),
				   0, 0, 8, 8, hints, &hints_ret);
  if (testImage == NULL)
    return false;

  // Fill in a template for matching visuals.
  XVisualInfo tmpl;
  tmpl.c_class = TrueColor;
  tmpl.depth = 24;
  tmpl.red_mask = testImage->red_mask;
  tmpl.green_mask = testImage->green_mask;
  tmpl.blue_mask = testImage->blue_mask;

  // List fields in template that make sense.
  long mask = (VisualClassMask |
	       VisualRedMaskMask |
	       VisualGreenMaskMask |
	       VisualBlueMaskMask);

  // We don't need that image any more.
  XDestroyImage(testImage);

  // Now, get a list of matching visuals available.
  int nVisuals;
  XVisualInfo *vinfo = XGetVisualInfo(dpy, mask, &tmpl, &nVisuals);
  if (vinfo == NULL || nVisuals <= 0) {
    if (vinfo != NULL) {
      XFree(vinfo);
    }
    return false;
  }

  // Use first visual from the list.
  *vinfo_ret = vinfo[0];

  XFree(vinfo);

  return true;
}

IrixOverlayShmImage::~IrixOverlayShmImage()
{
  if (readDisplayBuf != NULL)
    XShmDestroyReadDisplayBuf(readDisplayBuf);
}

void IrixOverlayShmImage::get(Window wnd, int x, int y)
{
  get(wnd, x, y, xim->width, xim->height);
}

void IrixOverlayShmImage::get(Window wnd, int x, int y, int w, int h,
                              int dst_x, int dst_y)
{
  XRectangle rect;
  unsigned long hints = XRD_TRANSPARENT | XRD_READ_POINTER;

  rect.x = x;
  rect.y = y;
  rect.width = w;
  rect.height = h;

  XShmReadDisplayRects(dpy, wnd,
                       &rect, 1, readDisplayBuf,
                       dst_x - x, dst_y - y,
                       hints, &hints);
}

#endif // HAVE_READDISPLAY
#endif // HAVE_MITSHM

#ifdef HAVE_SUN_OVL

//
// SolarisOverlayImage class implementation
//

SolarisOverlayImage::SolarisOverlayImage(Display *d)
  : Image(d)
{
}

SolarisOverlayImage::SolarisOverlayImage(Display *d, int width, int height)
  : Image(d)
{
  Init(width, height);
}

void SolarisOverlayImage::Init(int width, int height)
{
  // FIXME: Check if the extension is available at run time.
  // FIXME: Maybe just read a small (e.g. 8x8) screen area then
  //        reallocate xim->data[] and correct width and height?
  xim = XReadScreen(dpy, DefaultRootWindow(dpy), 0, 0, width, height, True);
  if (xim == NULL) {
    vlog.error("XReadScreen() failed");
    return;
  }
}

SolarisOverlayImage::~SolarisOverlayImage()
{
}

void SolarisOverlayImage::get(Window wnd, int x, int y)
{
  get(wnd, x, y, xim->width, xim->height);
}

void SolarisOverlayImage::get(Window wnd, int x, int y, int w, int h,
                              int dst_x, int dst_y)
{
  XImage *tmp_xim = XReadScreen(dpy, wnd, x, y, w, h, True);
  if (tmp_xim == NULL)
    return;

  updateRect(tmp_xim, dst_x, dst_y);

  XDestroyImage(tmp_xim);
}

#endif // HAVE_SUN_OVL

//
// ImageFactory class implementation
//
// FIXME: Make ImageFactory always create images of the same class?
//

// Prepare useful shortcuts for compile-time options.
#if defined(HAVE_READDISPLAY) && defined(HAVE_MITSHM)
#define HAVE_SHM_READDISPLAY
#endif
#if defined(HAVE_SHM_READDISPLAY) || defined(HAVE_SUN_OVL)
#define HAVE_OVERLAY_EXT
#endif

ImageFactory::ImageFactory(bool allowShm, bool allowOverlay)
  : mayUseShm(allowShm), mayUseOverlay(allowOverlay)
{
}

ImageFactory::~ImageFactory()
{
}

Image *ImageFactory::newImage(Display *d, int width, int height)
{
  Image *image = NULL;

  // First, try to create an image with overlay support.

#ifdef HAVE_OVERLAY_EXT
  if (mayUseOverlay) {
#if defined(HAVE_SHM_READDISPLAY)
    if (mayUseShm) {
      image = new IrixOverlayShmImage(d, width, height);
      if (image->xim != NULL) {
        return image;
      }
    }
#elif defined(HAVE_SUN_OVL)
    image = new SolarisOverlayImage(d, width, height);
    if (image->xim != NULL) {
      return image;
    }
#endif
    if (image != NULL) {
      delete image;
      vlog.error("Failed to create overlay image, trying other options");
    }
  }
#endif // HAVE_OVERLAY_EXT

  // Now, try to use shared memory image.

#ifdef HAVE_MITSHM
  if (mayUseShm) {
    image = new ShmImage(d, width, height);
    if (image->xim != NULL) {
      return image;
    }

    delete image;
    vlog.error("Failed to create SHM image, falling back to Xlib image");
  }
#endif // HAVE_MITSHM

  // Fall back to Xlib image.

  image = new Image(d, width, height);
  return image;
}
