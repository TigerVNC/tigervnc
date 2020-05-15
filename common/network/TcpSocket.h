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

// -=- TcpSocket.h - base-class for TCP stream sockets.
//     This header also defines the TcpListener class, used
//     to listen for incoming socket connections over TCP
//
//     NB: Any file descriptors created by the TcpSocket or
//     TcpListener classes are close-on-exec if the OS supports
//     it.  TcpSockets initialised with a caller-supplied fd
//     are NOT set to close-on-exec.

#ifndef __NETWORK_TCP_SOCKET_H__
#define __NETWORK_TCP_SOCKET_H__

#include <network/Socket.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h> /* for socklen_t */
#include <netinet/in.h> /* for struct sockaddr_in */
#endif

#include <list>

/* Tunnelling support. */
#define TUNNEL_PORT_OFFSET 5500

namespace network {

  /* Tunnelling support. */
  int findFreeTcpPort (void);

  int getSockPort(int sock);

  class TcpSocket : public Socket {
  public:
    TcpSocket(int sock);
    TcpSocket(const char *name, int port);

    virtual char* getPeerAddress();
    virtual char* getPeerEndpoint();

  protected:
    bool enableNagles(bool enable);
  };

  class TcpListener : public SocketListener {
  public:
    TcpListener(const struct sockaddr *listenaddr, socklen_t listenaddrlen);
    TcpListener(int sock);

    virtual int getMyPort();

    static void getMyAddresses(std::list<char*>* result);

  protected:
    virtual Socket* createSocket(int fd);
  };

  void createLocalTcpListeners(std::list<SocketListener*> *listeners,
                               int port);
  void createTcpListeners(std::list<SocketListener*> *listeners,
                          const char *addr,
                          int port);
  void createTcpListeners(std::list<SocketListener*> *listeners,
                          const struct addrinfo *ai);

  typedef struct vnc_sockaddr {
    union {
      sockaddr     sa;
      sockaddr_in  sin;
      sockaddr_in6 sin6;
    } u;
  } vnc_sockaddr_t;

  class TcpFilter : public ConnectionFilter {
  public:
    TcpFilter(const char* filter);
    virtual ~TcpFilter();

    virtual bool verifyConnection(Socket* s);

    typedef enum {Accept, Reject, Query} Action;
    struct Pattern {
      Action action;
      vnc_sockaddr_t address;
      unsigned int prefixlen;

      vnc_sockaddr_t mask; // computed from address and prefix
    };
    static Pattern parsePattern(const char* s);
    static char* patternToStr(const Pattern& p);
  protected:
    std::list<Pattern> filter;
  };

}

#endif // __NETWORK_TCP_SOCKET_H__
