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

#ifdef WIN32
//#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define errorNumber WSAGetLastError()
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#else
#define errorNumber errno
#define closesocket close
#include <sys/socket.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>

#include <network/Socket.h>

using namespace network;

static core::LogWriter vlog("Socket");

// -=- Socket initialisation
static bool socketsInitialised = false;
void network::initSockets() {
  if (socketsInitialised)
    return;
#ifdef WIN32
  WORD requiredVersion = MAKEWORD(2,0);
  WSADATA initResult;
  
  if (WSAStartup(requiredVersion, &initResult) != 0)
    throw core::socket_error("Unable to initialise Winsock2", errorNumber);
#else
  signal(SIGPIPE, SIG_IGN);
#endif
  socketsInitialised = true;
}

bool network::isSocketListening(int sock)
{
  int listening = 0;
  socklen_t listening_size = sizeof(listening);
  if (getsockopt(sock, SOL_SOCKET, SO_ACCEPTCONN,
                 (char *)&listening, &listening_size) < 0)
    return false;
  return listening != 0;
}

Socket::Socket(int fd)
  : instream(nullptr), outstream(nullptr),
    isShutdown_(false), queryConnection(false)
{
  initSockets();
  setFd(fd);
}

Socket::Socket()
  : instream(nullptr), outstream(nullptr),
    isShutdown_(false), queryConnection(false)
{
  initSockets();
}

Socket::~Socket()
{
  if (instream && outstream)
    closesocket(getFd());
  delete instream;
  delete outstream;
}

int Socket::getFd()
{
  return outstream->getFd();
}

// if shutdown() is overridden then the override MUST call on to here
void Socket::shutdown()
{
  try {
    if (outstream->hasBufferedData()) {
      outstream->cork(false);
      outstream->flush();
      if (outstream->hasBufferedData())
        vlog.error("Failed to flush remaining socket data on close");
    }
  } catch (std::exception& e) {
    vlog.error("Failed to flush remaining socket data on close: %s", e.what());
  }

  isShutdown_ = true;
  ::shutdown(getFd(), SHUT_RDWR);
}

bool Socket::isShutdown() const
{
  return isShutdown_;
}

void Socket::cork(bool enable)
{
  outstream->cork(enable);
}

// Was there a "?" in the ConnectionFilter used to accept this Socket?
void Socket::setRequiresQuery()
{
  queryConnection = true;
}

bool Socket::requiresQuery() const
{
  return queryConnection;
}

void Socket::setFd(int fd)
{
#ifndef WIN32
  // - By default, close the socket on exec()
  fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

  instream = new rdr::FdInStream(fd);
  outstream = new rdr::FdOutStream(fd);
  isShutdown_ = false;
}

SocketListener::SocketListener(int fd_)
  : fd(fd_), filter(nullptr)
{
  initSockets();
}

SocketListener::SocketListener()
  : fd(-1), filter(nullptr)
{
  initSockets();
}

SocketListener::~SocketListener()
{
  if (fd != -1)
    closesocket(fd);
}

void SocketListener::shutdown()
{
#ifdef WIN32
  closesocket(fd);
  fd = -1;
#else
  ::shutdown(fd, SHUT_RDWR);
#endif
}

Socket* SocketListener::accept() {
  int new_sock = -1;

  // Accept an incoming connection
  if ((new_sock = ::accept(fd, nullptr, nullptr)) < 0)
    throw core::socket_error("Unable to accept new connection", errorNumber);

  // Create the socket object & check connection is allowed
  Socket* s = createSocket(new_sock);
  if (filter && !filter->verifyConnection(s)) {
    delete s;
    return nullptr;
  }

  return s;
}

void SocketListener::listen(int sock)
{
  // - Set it to be a listening socket
  if (::listen(sock, 5) < 0) {
    int e = errorNumber;
    closesocket(sock);
    throw core::socket_error("Unable to set socket to listening mode", e);
  }

  fd = sock;
}
