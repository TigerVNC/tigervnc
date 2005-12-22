/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>

namespace network {

  class Socket {
  public:
    Socket(int fd)
      : instream(new rdr::FdInStream(fd)),
      outstream(new rdr::FdOutStream(fd)),
      own_streams(true) {}
    virtual ~Socket() {
      if (own_streams) {
        delete instream;
        delete outstream;
      }
    }
    rdr::FdInStream &inStream() {return *instream;}
    rdr::FdOutStream &outStream() {return *outstream;}
    int getFd() {return outstream->getFd();}
    virtual void shutdown() = 0;

    // information about this end of the socket
    virtual char* getMyAddress() = 0; // a string e.g. "192.168.0.1"
    virtual int getMyPort() = 0;
    virtual char* getMyEndpoint() = 0; // <address>::<port>

    // information about the remote end of the socket
    virtual char* getPeerAddress() = 0; // a string e.g. "192.168.0.1"
    virtual int getPeerPort() = 0;
    virtual char* getPeerEndpoint() = 0; // <address>::<port>

    // Is the remote end on the same machine?
    virtual bool sameMachine() = 0;

  protected:
    Socket() : instream(0), outstream(0), own_streams(false) {}
    Socket(rdr::FdInStream* i, rdr::FdOutStream* o, bool own)
      : instream(i), outstream(o), own_streams(own) {}
    rdr::FdInStream* instream;
    rdr::FdOutStream* outstream;
    bool own_streams;
  };

  class ConnectionFilter {
  public:
    virtual bool verifyConnection(Socket* s) = 0;
    virtual bool queryUserAcceptConnection(Socket*) {return false;}
  };

  class SocketListener {
  public:
    SocketListener() : fd(0), filter(0) {}
    virtual ~SocketListener() {}

    // shutdown() stops the socket from accepting further connections
    virtual void shutdown() = 0;

    // accept() returns a new Socket object if there is a connection
    // attempt in progress AND if the connection passes the filter
    // if one is installed.  Otherwise, returns 0.
    virtual Socket* accept() = 0;

    void setFilter(ConnectionFilter* f) {filter = f;}
    int getFd() {return fd;}
  protected:
    int fd;
    ConnectionFilter* filter;
  };

  struct SocketException : public rdr::SystemException {
    SocketException(const char* text, int err_) : rdr::SystemException(text, err_) {}
  };

  class SocketServer {
  public:
    virtual ~SocketServer() {}

    // addClient() tells the server to manage the socket.
    //   If the server can't manage the socket, it must shutdown() it.
    virtual void addClient(network::Socket* sock) = 0;

    // processSocketEvent() tells the server there is a socket read event.
    //   If there is an error, or the socket has been closed/shutdown then
    //   the server MUST delete the socket AND return false.
    virtual bool processSocketEvent(network::Socket* sock) = 0;

    // checkTimeouts() allows the server to check socket timeouts, etc.  The
    // return value is the number of milliseconds to wait before
    // checkTimeouts() should be called again.  If this number is zero then
    // there is no timeout and checkTimeouts() should be called the next time
    // an event occurs.
    virtual int checkTimeouts() = 0;

    // soonestTimeout() is a function to help work out the soonest of several
    // timeouts.
    static void soonestTimeout(int* timeout, int newTimeout) {
      if (newTimeout && (!*timeout || newTimeout < *timeout))
        *timeout = newTimeout;
    }
    virtual bool getDisable() {return true;};
  };

}

#endif // __NETWORK_SOCKET_H__
