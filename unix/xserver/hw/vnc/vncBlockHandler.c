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

#include "os.h"
#include "dix.h"
#include "scrnintstr.h"

#include "vncExtInit.h"
#include "vncBlockHandler.h"
#include "xorg-version.h"

static void vncBlockHandler(void* data, void* timeout);
static void vncSocketNotify(int fd, int xevents, void *data);

void vncRegisterBlockHandlers(void)
{
  if (!RegisterBlockAndWakeupHandlers(vncBlockHandler,
                                      (ServerWakeupHandlerProcPtr)NoopDDA,
                                      0))
    FatalError("RegisterBlockAndWakeupHandlers() failed\n");
}

void vncSetNotifyFd(int fd, int scrIdx, int read, int write)
{
  int mask = (read ? X_NOTIFY_READ : 0) | (write ? X_NOTIFY_WRITE : 0);
  SetNotifyFd(fd, vncSocketNotify, mask, (void*)(intptr_t)scrIdx);
}

void vncRemoveNotifyFd(int fd)
{
  RemoveNotifyFd(fd);
}

static void vncSocketNotify(int fd, int xevents, void *data)
{
  int scrIdx;

  scrIdx = (intptr_t)data;
  vncHandleSocketEvent(fd, scrIdx,
                       xevents & X_NOTIFY_READ,
                       xevents & X_NOTIFY_WRITE);
}

//
// vncBlockHandler - called just before the X server goes into poll().
//

static void vncBlockHandler(void* data, void* timeout)
{
  vncCallBlockHandlers(timeout);
}
