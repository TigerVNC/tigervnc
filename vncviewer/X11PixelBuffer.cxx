/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include "X11PixelBuffer.h"

using namespace rfb;

static rfb::LogWriter vlog("PlatformPixelBuffer");

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
    throw rfb::Exception("Error: display lacks pixmap format for default depth");

  switch (format[i].bits_per_pixel) {
  case 8:
  case 16:
  case 32:
    bpp = format[i].bits_per_pixel;
    break;
  default:
    throw rfb::Exception("Error: couldn't find suitable pixmap format");
  }

  XFree(format);

  bigEndian = (ImageByteOrder(fl_display) == MSBFirst);
  trueColour = (fl_visual->c_class == TrueColor);

  if (!trueColour)
    throw rfb::Exception("Error: only true colour displays supported");

  vlog.info("Using default colormap and visual, %sdepth %d.",
            (fl_visual->c_class == TrueColor) ? "TrueColor, " :
            ((fl_visual->c_class == PseudoColor) ? "PseudoColor, " : ""),
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

PlatformPixelBuffer::PlatformPixelBuffer(int width, int height) :
  FullFramePixelBuffer(display_pf(), width, height, NULL, NULL),
  shminfo(NULL), xim(NULL)
{
  // Might not be open at this point
  fl_open_display();

  if (!setupShm()) {
    xim = XCreateImage(fl_display, fl_visual->visual, fl_visual->depth,
                       ZPixmap, 0, 0, width, height, BitmapPad(fl_display), 0);
    assert(xim);

    xim->data = (char*)malloc(xim->bytes_per_line * xim->height);
    assert(xim->data);
  }

  data = (rdr::U8*)xim->data;
}


PlatformPixelBuffer::~PlatformPixelBuffer()
{
  if (shminfo) {
    vlog.debug("Freeing shared memory XImage");
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


void PlatformPixelBuffer::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  if (shminfo)
    XShmPutImage(fl_display, fl_window, fl_gc, xim, src_x, src_y, x, y, w, h, False);
  else
    XPutImage(fl_display, fl_window, fl_gc, xim, src_x, src_y, x, y, w, h);
}


int PlatformPixelBuffer::getStride() const
{
  return xim->bytes_per_line / (getPF().bpp/8);
}

static bool caughtError;

static int XShmAttachErrorHandler(Display *dpy, XErrorEvent *error)
{
  caughtError = true;
  return 0;
}

int PlatformPixelBuffer::setupShm()
{
  int major, minor;
  Bool pixmaps;
  XErrorHandler old_handler;
  Status status;

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

  XShmAttach(fl_display, shminfo);
  XSync(fl_display, False);

  XSetErrorHandler(old_handler);

  if (caughtError)
    goto free_shmaddr;

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
