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
// Image.h
//

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

//
// Image class is an Xlib-based implementation of screen image storage.
//

class Image {

public:

  Image(Display *d);
  Image(Display *d, int width, int height);
  virtual ~Image();

  bool isTrueColor() const { return trueColor; }

  virtual const char *className() const {
    return "Image";
  }
  virtual const char *classDesc() const {
    return "basic Xlib image";
  }

  virtual void get(Window wnd, int x = 0, int y = 0);
  virtual void get(Window wnd, int x, int y, int w, int h,
                   int dst_x = 0, int dst_y = 0);

  // Copying pixels from one image to another.
  virtual void updateRect(XImage *src, int dst_x = 0, int dst_y = 0);
  virtual void updateRect(Image *src, int dst_x = 0, int dst_y = 0);
  virtual void updateRect(XImage *src, int dst_x, int dst_y, int w, int h);
  virtual void updateRect(Image *src, int dst_x, int dst_y, int w, int h);
  virtual void updateRect(XImage *src, int dst_x, int dst_y,
                          int src_x, int src_y, int w, int h);
  virtual void updateRect(Image *src, int dst_x, int dst_y,
                          int src_x, int src_y, int w, int h);

  // Pointer to corresponding XImage, made public for efficiency.
  // NOTE: if this field is NULL, then no methods other than Init()
  //       may be called.
  XImage *xim;

  // Get a pointer to the data corresponding to the given coordinates.
  inline char *locatePixel(int x, int y) const {
    return (xim->data +
            y * xim->bytes_per_line +
            x * (xim->bits_per_pixel / 8));
  }

protected:

  void Init(int width, int height);

  // Like updateRect(), but does not check arguments.
  void copyPixels(XImage *src,
                  int dst_x, int dst_y,
                  int src_x, int src_y,
                  int w, int h);

  Display *dpy;
  bool trueColor;

};

//
// ShmImage uses MIT-SHM extension of an X server to get image data.
//

#ifdef HAVE_MITSHM

#include <X11/extensions/XShm.h>

class ShmImage : public Image {

public:

  ShmImage(Display *d);
  ShmImage(Display *d, int width, int height);
  virtual ~ShmImage();

  virtual const char *className() const {
    return "ShmImage";
  }
  virtual const char *classDesc() const {
    return "shared memory image";
  }

  virtual void get(Window wnd, int x = 0, int y = 0);
  virtual void get(Window wnd, int x, int y, int w, int h,
                   int dst_x = 0, int dst_y = 0);

protected:

  void Init(int width, int height, const XVisualInfo *vinfo = NULL);

  XShmSegmentInfo *shminfo;

};

//
// IrixOverlayShmImage uses ReadDisplay extension of an X server to
// get truecolor image data, regardless of the default X visual type. 
// This method is available on Irix only.
//

#ifdef HAVE_READDISPLAY

#include <X11/extensions/readdisplay.h>

class IrixOverlayShmImage : public ShmImage {

public:

  IrixOverlayShmImage(Display *d);
  IrixOverlayShmImage(Display *d, int width, int height);
  virtual ~IrixOverlayShmImage();

  virtual const char *className() const {
    return "IrixOverlayShmImage";
  }
  virtual const char *classDesc() const {
    return "IRIX-specific SHM-aware overlay image";
  }

  virtual void get(Window wnd, int x = 0, int y = 0);
  virtual void get(Window wnd, int x, int y, int w, int h,
                   int dst_x = 0, int dst_y = 0);

protected:

  void Init(int width, int height);

  // This method searches available X visuals for one that matches
  // actual pixel format returned by XReadDisplay(). Returns true on
  // success, false if there is no matching visual. On success, visual
  // information is placed into the structure pointed by vinfo_ret.
  bool getOverlayVisualInfo(XVisualInfo *vinfo_ret);

  ShmReadDisplayBuf *readDisplayBuf;

};

#endif // HAVE_READDISPLAY
#endif // HAVE_MITSHM

//
// SolarisOverlayImage uses SUN_OVL extension of an X server to get
// truecolor image data, regardless of the default X visual type. This
// method is available on Solaris only.
//

#ifdef HAVE_SUN_OVL

#include <X11/extensions/transovl.h>

class SolarisOverlayImage : public Image {

public:

  SolarisOverlayImage(Display *d);
  SolarisOverlayImage(Display *d, int width, int height);
  virtual ~SolarisOverlayImage();

  virtual const char *className() const {
    return "SolarisOverlayImage";
  }
  virtual const char *classDesc() const {
    return "Solaris-specific non-SHM overlay image";
  }

  virtual void get(Window wnd, int x = 0, int y = 0);
  virtual void get(Window wnd, int x, int y, int w, int h,
                   int dst_x = 0, int dst_y = 0);

protected:

  void Init(int width, int height);

};

#endif // HAVE_SUN_OVL

//
// ImageFactory class is used to produce instances of Image-derived
// objects that are most appropriate for current X server and user
// settings.
//

class ImageFactory {

public:

  ImageFactory(bool allowShm, bool allowOverlay);
  virtual ~ImageFactory();

  bool isShmAllowed()     { return mayUseShm; }
  bool isOverlayAllowed() { return mayUseOverlay; }

  virtual Image *newImage(Display *d, int width, int height);

protected:

  bool mayUseShm;
  bool mayUseOverlay;

};

#endif // __IMAGE_H__
