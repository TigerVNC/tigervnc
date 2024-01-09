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

// -=- Socket.h - abstract base-class for any kind of network stream/socket

#ifndef __NETWORK_SOCKET_H__
#define __NETWORK_SOCKET_H__

#include <list>

#include <limits.h>
#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>

namespace network {

  void initSockets();

  bool isSocketListening(int sock);

  class Socket {
  public:
    Socket(int fd);
    virtual ~Socket();

    rdr::FdInStream &inStream() {return *instream;}
    rdr::FdOutStream &outStream() {return *outstream;}
    int getFd() {return outstream->getFd();}

    void shutdown();
    bool isShutdown() const;

    void cork(bool enable) { outstream->cork(enable); }

    // information about the remote end of the socket
    virtual const char* getPeerAddress() = 0; // a string e.g. "192.168.0.1"
    virtual const char* getPeerEndpoint() = 0; // <address>::<port>

    // Was there a "?" in the ConnectionFilter used to accept this Socket?
    void setRequiresQuery();
    bool requiresQuery() const;

  protected:
    Socket();

    void setFd(int fd);

  private:
    rdr::FdInStream* instream;
    rdr::FdOutStream* outstream;
    bool isShutdown_;
    bool queryConnection;
  };

  class ConnectionFilter {
  public:
    virtual bool verifyConnection(Socket* s) = 0;
    virtual ~ConnectionFilter() {}
  };

  class SocketListener {
  public:
    SocketListener(int fd);
    virtual ~SocketListener();

    // shutdown() stops the socket from accepting further connections
    void shutdown();

    // accept() returns a new Socket object if there is a connection
    // attempt in progress AND if the connection passes the filter
    // if one is installed.  Otherwise, returns 0.
    Socket* accept();

    virtual int getMyPort() = 0;

    // setFilter() applies the specified filter to all new connections
    void setFilter(ConnectionFilter* f) {filter = f;}
    int getFd() {return fd;}

  protected:
    SocketListener();

    void listen(int fd);

    // createSocket() should create a new socket of the correct class
    // for the given file descriptor
    virtual Socket* createSocket(int fd) = 0;

  protected:
    int fd;
    ConnectionFilter* filter;
  };

  struct SocketException : public rdr::SystemException {
    SocketException(const char* text, int err_) : rdr::SystemException(text, err_) {}
  };

}

#endif // __NETWORK_SOCKET_H__
