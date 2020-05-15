/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman for Cendio AB
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <winsock2.h>
#undef errno
#define errno WSAGetLastError()
#include <os/winerrno.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif

/* Old systems have select() in sys/time.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>
#include <rfb/util.h>


using namespace rdr;

FdOutStream::FdOutStream(int fd_, bool blocking_, int timeoutms_)
  : fd(fd_), blocking(blocking_), timeoutms(timeoutms_)
{
  gettimeofday(&lastWrite, NULL);
}

FdOutStream::~FdOutStream()
{
  try {
    while (sentUpTo != ptr)
      flushBuffer(true);
  } catch (Exception&) {
  }
}

void FdOutStream::setTimeout(int timeoutms_) {
  timeoutms = timeoutms_;
}

void FdOutStream::setBlocking(bool blocking_) {
  blocking = blocking_;
}

unsigned FdOutStream::getIdleTime()
{
  return rfb::msSince(&lastWrite);
}

void FdOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);

#ifdef TCP_CORK
  int one = enable ? 1 : 0;
  setsockopt(fd, IPPROTO_TCP, TCP_CORK, (char *)&one, sizeof(one));
#endif
}

bool FdOutStream::flushBuffer(bool wait)
{
  size_t n = writeWithTimeout((const void*) sentUpTo,
                              ptr - sentUpTo,
                              (blocking || wait)? timeoutms : 0);

  // Timeout?
  if (n == 0) {
    // If non-blocking then we're done here
    if (!blocking && !wait)
      return false;

    throw TimedOut();
  }

  sentUpTo += n;

  return true;
}

//
// writeWithTimeout() writes up to the given length in bytes from the given
// buffer to the file descriptor.  If there is a timeout set and that timeout
// expires, it throws a TimedOut exception.  Otherwise it returns the number of
// bytes written.  It never attempts to send() unless select() indicates that
// the fd is writable - this means it can be used on an fd which has been set
// non-blocking.  It also has to cope with the annoying possibility of both
// select() and send() returning EINTR.
//

size_t FdOutStream::writeWithTimeout(const void* data, size_t length, int timeoutms)
{
  int n;

  do {
    fd_set fds;
    struct timeval tv;
    struct timeval* tvp = &tv;

    if (timeoutms != -1) {
      tv.tv_sec = timeoutms / 1000;
      tv.tv_usec = (timeoutms % 1000) * 1000;
    } else {
      tvp = NULL;
    }

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    n = select(fd+1, 0, &fds, 0, tvp);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    throw SystemException("select", errno);

  if (n == 0)
    return 0;

  do {
    // select only guarantees that you can write SO_SNDLOWAT without
    // blocking, which is normally 1. Use MSG_DONTWAIT to avoid
    // blocking, when possible.
#ifndef MSG_DONTWAIT
    n = ::send(fd, (const char*)data, length, 0);
#else
    n = ::send(fd, (const char*)data, length, MSG_DONTWAIT);
#endif
  } while (n < 0 && (errno == EINTR));

  if (n < 0)
    throw SystemException("write", errno);

  gettimeofday(&lastWrite, NULL);

  return n;
}
