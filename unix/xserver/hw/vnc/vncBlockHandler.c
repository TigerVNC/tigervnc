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

#if XORG_AT_LEAST(1, 19, 0)
static void vncBlockHandler(void* data, void* timeout);
static void vncSocketNotify(int fd, int xevents, void *data);
#else
static void vncBlockHandler(void * data, OSTimePtr t, void * readmask);
static void vncWakeupHandler(void * data, int nfds, void * readmask);

struct vncFdEntry {
  int fd;
  int read, write;
  int scrIdx;
  struct vncFdEntry* next;
};

static struct vncFdEntry* fdsHead = NULL;
#endif

void vncRegisterBlockHandlers(void)
{
  if (!RegisterBlockAndWakeupHandlers(vncBlockHandler,
#if XORG_AT_LEAST(1, 19, 0)
                                      (ServerWakeupHandlerProcPtr)NoopDDA,
#else
                                      vncWakeupHandler,
#endif
                                      0))
    FatalError("RegisterBlockAndWakeupHandlers() failed\n");
}

void vncSetNotifyFd(int fd, int scrIdx, int read, int write)
{
#if XORG_AT_LEAST(1, 19, 0)
  int mask = (read ? X_NOTIFY_READ : 0) | (write ? X_NOTIFY_WRITE : 0);
  SetNotifyFd(fd, vncSocketNotify, mask, (void*)(intptr_t)scrIdx);
#else
  struct vncFdEntry* entry;

  entry = fdsHead;
  while (entry) {
    if (entry->fd == fd) {
      assert(entry->scrIdx == scrIdx);
      entry->read = read;
      entry->write = write;
      return;
    }
    entry = entry->next;
  }

  entry = malloc(sizeof(struct vncFdEntry));
  memset(entry, 0, sizeof(struct vncFdEntry));

  entry->fd = fd;
  entry->scrIdx = scrIdx;
  entry->read = read;
  entry->write = write;

  entry->next = fdsHead;
  fdsHead = entry;
#endif
}

void vncRemoveNotifyFd(int fd)
{
#if XORG_AT_LEAST(1, 19, 0)
  RemoveNotifyFd(fd);
#else
  struct vncFdEntry** prev;
  struct vncFdEntry* entry;

  prev = &fdsHead;
  entry = fdsHead;
  while (entry) {
    if (entry->fd == fd) {
      *prev = entry->next;
      return;
    }
    prev = &entry->next;
    entry = entry->next;
  }

  assert(FALSE);
#endif
}

#if XORG_AT_LEAST(1, 19, 0)
static void vncSocketNotify(int fd, int xevents, void *data)
{
  int scrIdx;

  scrIdx = (intptr_t)data;
  vncHandleSocketEvent(fd, scrIdx,
                       xevents & X_NOTIFY_READ,
                       xevents & X_NOTIFY_WRITE);
}
#endif

#if XORG_OLDER_THAN(1, 19, 0)
static void vncWriteBlockHandlerFallback(OSTimePtr timeout);
static void vncWriteWakeupHandlerFallback(void);
void vncWriteBlockHandler(fd_set *fds);
void vncWriteWakeupHandler(int nfds, fd_set *fds);
#endif

//
// vncBlockHandler - called just before the X server goes into poll().
//
// For older versions of X this also allows us to register file
// descriptors that we want read events on.
//

#if XORG_AT_LEAST(1, 19, 0)
static void vncBlockHandler(void* data, void* timeout)
#else
static void vncBlockHandler(void * data, OSTimePtr t, void * readmask)
#endif
{
#if XORG_OLDER_THAN(1, 19, 0)
  int _timeout;
  int* timeout = &_timeout;
  static struct timeval tv;

  fd_set* fds;
  static struct vncFdEntry* entry;

  if (*t == NULL)
    _timeout = -1;
  else
    _timeout = (*t)->tv_sec * 1000 + (*t)->tv_usec / 1000;
#endif

  vncCallBlockHandlers(timeout);

#if XORG_OLDER_THAN(1, 19, 0)
  if (_timeout != -1) {
    tv.tv_sec= _timeout / 1000;
    tv.tv_usec = (_timeout % 1000) * 1000;
    *t = &tv;
  }

  fds = (fd_set*)readmask;
  entry = fdsHead;
  while (entry) {
    if (entry->read)
      FD_SET(entry->fd, fds);
    entry = entry->next;
  }

  vncWriteBlockHandlerFallback(t);
#endif
}

#if XORG_OLDER_THAN(1, 19, 0)
static void vncWakeupHandler(void * data, int nfds, void * readmask)
{
  fd_set* fds = (fd_set*)readmask;

  static struct vncFdEntry* entry;

  if (nfds <= 0)
    return;

  entry = fdsHead;
  while (entry) {
    if (entry->read && FD_ISSET(entry->fd, fds))
      vncHandleSocketEvent(entry->fd, entry->scrIdx, TRUE, FALSE);
    entry = entry->next;
  }

  vncWriteWakeupHandlerFallback();
}
#endif

//
// vncWriteBlockHandler - extra hack to be able to get old versions of the X
// server to monitor writeable fds and not just readable. This requirers a
// modified Xorg and might therefore not be called.
//

#if XORG_OLDER_THAN(1, 19, 0)
static Bool needFallback = TRUE;
static fd_set fallbackFds;
static struct timeval tw;

void vncWriteBlockHandler(fd_set *fds)
{
  static struct vncFdEntry* entry;

  needFallback = FALSE;

  entry = fdsHead;
  while (entry) {
    if (entry->write)
      FD_SET(entry->fd, fds);
    entry = entry->next;
  }
}

void vncWriteWakeupHandler(int nfds, fd_set *fds)
{
  static struct vncFdEntry* entry;

  if (nfds <= 0)
    return;

  entry = fdsHead;
  while (entry) {
    if (entry->write && FD_ISSET(entry->fd, fds))
      vncHandleSocketEvent(entry->fd, entry->scrIdx, FALSE, TRUE);
    entry = entry->next;
  }
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
