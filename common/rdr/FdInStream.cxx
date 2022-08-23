/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
#include <sys/time.h>
#ifdef _WIN32
#include <winsock2.h>
#define close closesocket
#undef errno
#define errno WSAGetLastError()
#include <os/winerrno.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/* Old systems have select() in sys/time.h */
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <rdr/FdInStream.h>
#include <rdr/Exception.h>

using namespace rdr;

FdInStream::FdInStream(int fd_, bool closeWhenDone_)
  : fd(fd_), closeWhenDone(closeWhenDone_)
{
}

FdInStream::~FdInStream()
{
  if (closeWhenDone) close(fd);
}


bool FdInStream::fillBuffer()
{
  size_t n = readFd((U8*)end, availSpace());
  if (n == 0)
    return false;
  end += n;

  return true;
}

//
// readFd() reads up to the given length in bytes from the
// file descriptor into a buffer. Zero is
// returned if no bytes can be read. Otherwise it returns the number of bytes read.  It
// never attempts to recv() unless select() indicates that the fd is readable -
// this means it can be used on an fd which has been set non-blocking.  It also
// has to cope with the annoying possibility of both select() and recv()
// returning EINTR.
//

size_t FdInStream::readFd(void* buf, size_t len)
{
  int n;
  do {
    fd_set fds;
    struct timeval tv;

    tv.tv_sec = tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    n = select(fd+1, &fds, 0, 0, &tv);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    throw SystemException("select",errno);

  if (n == 0)
    return 0;

  do {
    n = ::recv(fd, (char*)buf, len, 0);
  } while (n < 0 && errno == EINTR);

  if (n < 0)
    throw SystemException("read",errno);
  if (n == 0)
    throw EndOfStream();

  return n;
}
