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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <errno.h>

#include <X11/Xpoll.h>

#include "dix.h"
#include "scrnintstr.h"

#include "vncExtInit.h"
#include "vncBlockHandler.h"
#include "xorg-version.h"

#if XORG >= 119

static void vncBlockHandler(void* data, void* timeout)
{
  vncCallBlockHandlers(timeout);
}

void vncRegisterBlockHandlers(void)
{
  if (!RegisterBlockAndWakeupHandlers(vncBlockHandler,
                                      (ServerWakeupHandlerProcPtr)NoopDDA, 0))
    FatalError("RegisterBlockAndWakeupHandlers() failed\n");
}

#else

static void vncBlockHandler(void * data, OSTimePtr t, void * readmask);
static void vncWakeupHandler(void * data, int nfds, void * readmask);
void vncWriteBlockHandler(fd_set *fds);
void vncWriteWakeupHandler(int nfds, fd_set *fds);

void vncRegisterBlockHandlers(void)
{
  if (!RegisterBlockAndWakeupHandlers(vncBlockHandler, vncWakeupHandler, 0))
    FatalError("RegisterBlockAndWakeupHandlers() failed\n");
}

static void vncWriteBlockHandlerFallback(OSTimePtr timeout);
static void vncWriteWakeupHandlerFallback(void);

//
// vncBlockHandler - called just before the X server goes into select().  Call
// on to the block handler for each desktop.  Then check whether any of the
// selections have changed, and if so, notify any interested X clients.
//

static void vncBlockHandler(void * data, OSTimePtr timeout, void * readmask)
{
  fd_set* fds = (fd_set*)readmask;

  vncWriteBlockHandlerFallback(timeout);

  vncCallReadBlockHandlers(fds, timeout);
}

static void vncWakeupHandler(void * data, int nfds, void * readmask)
{
  fd_set* fds = (fd_set*)readmask;

  vncCallReadWakeupHandlers(fds, nfds);

  vncWriteWakeupHandlerFallback();
}

//
// vncWriteBlockHandler - extra hack to be able to get the main select loop
// to monitor writeable fds and not just readable. This requirers a modified
// Xorg and might therefore not be called. When it is called though, it will
// do so before vncBlockHandler (and vncWriteWakeupHandler called after
// vncWakeupHandler).
//

static Bool needFallback = TRUE;
static fd_set fallbackFds;
static struct timeval tw;

void vncWriteBlockHandler(fd_set *fds)
{
  struct timeval *dummy;

  needFallback = FALSE;

  dummy = NULL;
  vncCallWriteBlockHandlers(fds, &dummy);
}

void vncWriteWakeupHandler(int nfds, fd_set *fds)
{
  vncCallWriteWakeupHandlers(fds, nfds);
}

static void vncWriteBlockHandlerFallback(OSTimePtr timeout)
{
  if (!needFallback)
    return;

  FD_ZERO(&fallbackFds);
  vncWriteBlockHandler(&fallbackFds);

  // vncWriteBlockHandler() will clear this, so we need to restore it
  needFallback = TRUE;

  if (!XFD_ANYSET(&fallbackFds))
    return;

  if ((*timeout == NULL) ||
      ((*timeout)->tv_sec > 0) || ((*timeout)->tv_usec > 10000)) {
    tw.tv_sec = 0;
    tw.tv_usec = 10000;
    *timeout = &tw;
  }
}

static void vncWriteWakeupHandlerFallback(void)
{
  int ret;
  struct timeval timeout;

  if (!needFallback)
    return;

  if (!XFD_ANYSET(&fallbackFds))
    return;

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  ret = select(XFD_SETSIZE, NULL, &fallbackFds, NULL, &timeout);
  if (ret < 0) {
    ErrorF("vncWriteWakeupHandlerFallback(): select: %s\n",
           strerror(errno));
    return;
  }

  if (ret == 0)
    return;

  vncWriteWakeupHandler(ret, &fallbackFds);
}

#endif
