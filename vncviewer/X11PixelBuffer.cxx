/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2014 Pierre Ossman for Cendio AB
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

#include <FL/Fl.H>
#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include "i18n.h"
#include "X11PixelBuffer.h"

using namespace rfb;

static rfb::LogWriter vlog("X11PixelBuffer");

std::list<X11PixelBuffer*> X11PixelBuffer::shmList;

static PixelFormat display_pf()
{
  int i;

  int bpp;
  int trueColour, bigEndian;
  int redShift, greenShift, blueShift;
  int redMax, greenMax, blueMax;

  int nformats;
  XPixmapFormatValues* format;

  // Might not be open at this point
  fl_open_display();

  format = XListPixmapFormats(fl_display, &nformats);

  for (i = 0; i < nformats; i++)
    if (format[i].depth == fl_visual->depth) break;

  if (i == nformats)
    // TRANSLATORS: "pixmap" is an X11 concept and may not be suitable
    // to translate.
    throw rfb::Exception(_("Display lacks pixmap format for default depth"));

  switch (format[i].bits_per_pixel) {
  case 8:
  case 16:
  case 32:
    bpp = format[i].bits_per_pixel;
    break;
  default:
    // TRANSLATORS: "pixmap" is an X11 concept and may not be suitable
    // to translate.
    throw rfb::Exception(_("Couldn't find suitable pixmap format"));
  }

  XFree(format);

  bigEndian = (ImageByteOrder(fl_display) == MSBFirst);
  trueColour = (fl_visual->c_class == TrueColor);

  if (!trueColour)
    throw rfb::Exception(_("Only true colour displays supported"));

  vlog.info(_("Using default colormap and visual, TrueColor, depth %d."),
            fl_visual->depth);

  redShift   = ffs(fl_visual->red_mask)   - 1;
  greenShift = ffs(fl_visual->green_mask) - 1;
  blueShift  = ffs(fl_visual->blue_mask)  - 1;
  redMax     = fl_visual->red_mask   >> redShift;
  greenMax   = fl_visual->green_mask >> greenShift;
  blueMax    = fl_visual->blue_mask  >> blueShift;

  return PixelFormat(bpp, fl_visual->depth, bigEndian, trueColour,
                     redMax, greenMax, blueMax,
                     redShift, greenShift, blueShift);
}

X11PixelBuffer::X11PixelBuffer(int width, int height) :
  PlatformPixelBuffer(display_pf(), width, height, NULL, 0),
  shminfo(NULL), xim(NULL), pendingPutImage(0)
{
  // Might not be open at this point
  fl_open_display();

  if (!setupShm()) {
    xim = XCreateImage(fl_display, fl_visual->visual, fl_visual->depth,
                       ZPixmap, 0, 0, width, height, BitmapPad(fl_display), 0);
    if (!xim)
      throw rfb::Exception(_("Could not create framebuffer image"));

    xim->data = (char*)malloc(xim->bytes_per_line * xim->height);
    if (!xim->data)
      throw rfb::Exception(_("Not enough memory for framebuffer"));
  }

  data = (rdr::U8*)xim->data;
  stride = xim->bytes_per_line / (getPF().bpp/8);
}


X11PixelBuffer::~X11PixelBuffer()
{
  if (shminfo) {
    vlog.debug("Freeing shared memory XImage");
    shmList.remove(this);
    Fl::remove_system_handler(handleSystemEvent);
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    delete shminfo;
    shminfo = NULL;
  }

  // XDestroyImage() will free(xim->data) if appropriate
  if (xim)
    XDestroyImage(xim);
  xim = NULL;
}


void X11PixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  if (shminfo) {
    XShmPutImage(fl_display, fl_window, fl_gc, xim, src_x, src_y, x, y, w, h, True);
    pendingPutImage++;
  } else {
    XPutImage(fl_display, fl_window, fl_gc, xim, src_x, src_y, x, y, w, h);
  }
}

bool X11PixelBuffer::isRendering(void)
{
  return pendingPutImage > 0;
}

static bool caughtError;

static int XShmAttachErrorHandler(Display *dpy, XErrorEvent *error)
{
  caughtError = true;
  return 0;
}

int X11PixelBuffer::setupShm()
{
  int major, minor;
  Bool pixmaps;
  XErrorHandler old_handler;
  const char *display_name = XDisplayName (NULL);

  /* Don't use MIT-SHM on remote displays */
  if (*display_name && *display_name != ':')
    return 0;

  if (!XShmQueryVersion(fl_display, &major, &minor, &pixmaps))
    return 0;

  shminfo = new XShmSegmentInfo;

  xim = XShmCreateImage(fl_display, fl_visual->visual, fl_visual->depth,
                        ZPixmap, 0, shminfo, width(), height());
  if (!xim)
    goto free_shminfo;

  shminfo->shmid = shmget(IPC_PRIVATE,
                          xim->bytes_per_line * xim->height,
                          IPC_CREAT|0777);
  if (shminfo->shmid == -1)
    goto free_xim;

  shminfo->shmaddr = xim->data = (char*)shmat(shminfo->shmid, 0, 0);
  if (shminfo->shmaddr == (char *)-1)
    goto free_shm;

  shminfo->readOnly = True;

  // This is the only way we can detect that shared memory won't work
  // (e.g. because we're accessing a remote X11 server)
  caughtError = false;
  old_handler = XSetErrorHandler(XShmAttachErrorHandler);

  if (!XShmAttach(fl_display, shminfo)) {
    XSetErrorHandler(old_handler);
    goto free_shmaddr;
  }

  XSync(fl_display, False);

  XSetErrorHandler(old_handler);

  if (caughtError)
    goto free_shmaddr;

  // FLTK is a bit stupid and unreliable if you register the same
  // callback with different data values.
  Fl::add_system_handler(handleSystemEvent, NULL);
  shmList.push_back(this);

  vlog.debug("Using shared memory XImage");

  return 1;

free_shmaddr:
  shmdt(shminfo->shmaddr);

free_shm:
  shmctl(shminfo->shmid, IPC_RMID, 0);

free_xim:
  XDestroyImage(xim);
  xim = NULL;

free_shminfo:
  delete shminfo;
  shminfo = NULL;

  return 0;
}

int X11PixelBuffer::handleSystemEvent(void* event, void* data)
{
  XEvent* xevent;
  XShmCompletionEvent* shmevent;

  std::list<X11PixelBuffer*>::iterator iter;

  xevent = (XEvent*)event;
  assert(xevent);

  if (xevent->type != XShmGetEventBase(fl_display))
    return 0;

  shmevent = (XShmCompletionEvent*)event;

  for (iter = shmList.begin();iter != shmList.end();++iter) {
    if (shmevent->shmseg != (*iter)->shminfo->shmseg)
      continue;

    (*iter)->pendingPutImage--;
    assert((*iter)->pendingPutImage >= 0);

    return 1;
  }

  return 0;
}
