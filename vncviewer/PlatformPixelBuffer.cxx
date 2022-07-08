/* Copyright 2011-2016 Pierre Ossman for Cendio AB
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

#if !defined(WIN32) && !defined(__APPLE__)
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <FL/Fl.H>
#include <FL/x.H>

#include <rfb/LogWriter.h>
#include <rdr/Exception.h>

#include "PlatformPixelBuffer.h"

static rfb::LogWriter vlog("PlatformPixelBuffer");

PlatformPixelBuffer::PlatformPixelBuffer(int width, int height) :
  FullFramePixelBuffer(rfb::PixelFormat(32, 24, false, true,
                                        255, 255, 255, 16, 8, 0),
                       0, 0, NULL, 0),
  Surface(width, height)
#if !defined(WIN32) && !defined(__APPLE__)
  , shminfo(NULL), xim(NULL)
#endif
{
#if !defined(WIN32) && !defined(__APPLE__)
  if (!setupShm(width, height)) {
    xim = XCreateImage(fl_display, CopyFromParent, 32,
                       ZPixmap, 0, 0, width, height, 32, 0);
    if (!xim)
      throw rdr::Exception("XCreateImage");

    xim->data = (char*)malloc(xim->bytes_per_line * xim->height);
    if (!xim->data)
      throw rdr::Exception("malloc");

    vlog.debug("Using standard XImage");
  }

  setBuffer(width, height, (rdr::U8*)xim->data,
            xim->bytes_per_line / (getPF().bpp/8));

  // On X11, the Pixmap backing this Surface is uninitialized.
  clear(0, 0, 0);
#else
  setBuffer(width, height, (rdr::U8*)Surface::data, width);
#endif
}

PlatformPixelBuffer::~PlatformPixelBuffer()
{
#if !defined(WIN32) && !defined(__APPLE__)
  if (shminfo) {
    vlog.debug("Freeing shared memory XImage");
    XShmDetach(fl_display, shminfo);
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    delete shminfo;
    shminfo = NULL;
  }

  // XDestroyImage() will free(xim->data) if appropriate
  if (xim)
    XDestroyImage(xim);
  xim = NULL;
#endif
}

void PlatformPixelBuffer::commitBufferRW(const rfb::Rect& r)
{
  FullFramePixelBuffer::commitBufferRW(r);
  mutex.lock();
  damage.assign_union(rfb::Region(r));
  mutex.unlock();
}

rfb::Rect PlatformPixelBuffer::getDamage(void)
{
  rfb::Rect r;

  mutex.lock();
  r = damage.get_bounding_rect();
  damage.clear();
  mutex.unlock();

#if !defined(WIN32) && !defined(__APPLE__)
  if (r.width() == 0 || r.height() == 0)
    return r;

  GC gc;

  gc = XCreateGC(fl_display, pixmap, 0, NULL);
  if (shminfo) {
    XShmPutImage(fl_display, pixmap, gc, xim,
                 r.tl.x, r.tl.y, r.tl.x, r.tl.y,
                 r.width(), r.height(), False);
    // Need to make sure the X server has finished reading the
    // shared memory before we return
    XSync(fl_display, False);
  } else {
    XPutImage(fl_display, pixmap, gc, xim,
              r.tl.x, r.tl.y, r.tl.x, r.tl.y, r.width(), r.height());
  }
  XFreeGC(fl_display, gc);
#endif

  return r;
}

#if !defined(WIN32) && !defined(__APPLE__)

static bool caughtError;

static int XShmAttachErrorHandler(Display *dpy, XErrorEvent *error)
{
  caughtError = true;
  return 0;
}

bool PlatformPixelBuffer::setupShm(int width, int height)
{
  int major, minor;
  Bool pixmaps;
  XErrorHandler old_handler;
  const char *display_name = XDisplayName (NULL);

  /* Don't use MIT-SHM on remote displays */
  if (*display_name && *display_name != ':')
    return false;

  if (!XShmQueryVersion(fl_display, &major, &minor, &pixmaps))
    return false;

  shminfo = new XShmSegmentInfo;

  xim = XShmCreateImage(fl_display, CopyFromParent, 32,
                        ZPixmap, 0, shminfo, width, height);
  if (!xim)
    goto free_shminfo;

  shminfo->shmid = shmget(IPC_PRIVATE,
                          xim->bytes_per_line * xim->height,
                          IPC_CREAT|0600);
  if (shminfo->shmid == -1)
    goto free_xim;

  shminfo->shmaddr = xim->data = (char*)shmat(shminfo->shmid, 0, 0);
  shmctl(shminfo->shmid, IPC_RMID, 0); // to avoid memory leakage
  if (shminfo->shmaddr == (char *)-1)
    goto free_xim;

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

  vlog.debug("Using shared memory XImage");

  return true;

free_shmaddr:
  shmdt(shminfo->shmaddr);

free_xim:
  XDestroyImage(xim);
  xim = NULL;

free_shminfo:
  delete shminfo;
  shminfo = NULL;

  return 0;
}

#endif
