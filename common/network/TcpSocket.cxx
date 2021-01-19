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
#else
#define errorNumber errno
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#endif

#include <stdlib.h>
#include <unistd.h>

#include <network/TcpSocket.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>

#ifdef WIN32
#include <os/winerrno.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long)-1)
#endif
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK ((unsigned long)0x7F000001)
#endif

#ifndef IN6_ARE_ADDR_EQUAL
#define IN6_ARE_ADDR_EQUAL(a,b) \
  (memcmp ((const void*)(a), (const void*)(b), sizeof (struct in6_addr)) == 0)
#endif

// Missing on older Windows and OS X
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif

using namespace network;
using namespace rdr;

static rfb::LogWriter vlog("TcpSocket");

static rfb::BoolParameter UseIPv4("UseIPv4", "Use IPv4 for incoming and outgoing connections.", true);
static rfb::BoolParameter UseIPv6("UseIPv6", "Use IPv6 for incoming and outgoing connections.", true);

/* Tunnelling support. */
int network::findFreeTcpPort (void)
{
  int sock;
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;

  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    throw SocketException ("unable to create socket", errorNumber);

  addr.sin_port = 0;
  if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    throw SocketException ("unable to find free port", errorNumber);

  socklen_t n = sizeof(addr);
  if (getsockname (sock, (struct sockaddr *)&addr, &n) < 0)
    throw SocketException ("unable to get port number", errorNumber);

  closesocket (sock);
  return ntohs(addr.sin_port);
}

int network::getSockPort(int sock)
{
  vnc_sockaddr_t sa;
  socklen_t sa_size = sizeof(sa);
  if (getsockname(sock, &sa.u.sa, &sa_size) < 0)
    return 0;

  switch (sa.u.sa.sa_family) {
  case AF_INET6:
    return ntohs(sa.u.sin6.sin6_port);
  default:
    return ntohs(sa.u.sin.sin_port);
  }
}

// -=- TcpSocket

TcpSocket::TcpSocket(int sock) : Socket(sock)
{
  // Disable Nagle's algorithm, to reduce latency
  enableNagles(false);
}

TcpSocket::TcpSocket(const char *host, int port)
{
  int sock, err, result;
  struct addrinfo *ai, *current, hints;

  // - Create a socket

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  if ((result = getaddrinfo(host, NULL, &hints, &ai)) != 0) {
    throw GAIException("unable to resolve host by name", result);
  }

  sock = -1;
  err = 0;
  for (current = ai; current != NULL; current = current->ai_next) {
    int family;
    vnc_sockaddr_t sa;
    socklen_t salen;
    char ntop[NI_MAXHOST];

    family = current->ai_family;

    switch (family) {
    case AF_INET:
      if (!UseIPv4)
        continue;
      break;
    case AF_INET6:
      if (!UseIPv6)
        continue;
      break;
    default:
      continue;
    }

    salen = current->ai_addrlen;
    memcpy(&sa, current->ai_addr, salen);

    if (family == AF_INET)
      sa.u.sin.sin_port = htons(port);
    else
      sa.u.sin6.sin6_port = htons(port);

    getnameinfo(&sa.u.sa, salen, ntop, sizeof(ntop), NULL, 0, NI_NUMERICHOST);
    vlog.debug("Connecting to %s [%s] port %d", host, ntop, port);

    sock = socket (family, SOCK_STREAM, 0);
    if (sock == -1) {
      err = errorNumber;
      freeaddrinfo(ai);
      throw SocketException("unable to create socket", err);
    }

  /* Attempt to connect to the remote host */
    while ((result = connect(sock, &sa.u.sa, salen)) == -1) {
      err = errorNumber;
#ifndef WIN32
      if (err == EINTR)
        continue;
#endif
      vlog.debug("Failed to connect to address %s port %d: %d",
                 ntop, port, err);
      closesocket(sock);
      sock = -1;
      break;
    }

    if (result == 0)
      break;
  }

  freeaddrinfo(ai);

  if (sock == -1) {
    if (err == 0)
      throw Exception("No useful address for host");
    else
      throw SocketException("unable to connect to socket", err);
  }

  // Take proper ownership of the socket
  setFd(sock);

  // Disable Nagle's algorithm, to reduce latency
  enableNagles(false);
}

char* TcpSocket::getPeerAddress() {
  vnc_sockaddr_t sa;
  socklen_t sa_size = sizeof(sa);

  if (getpeername(getFd(), &sa.u.sa, &sa_size) != 0) {
    vlog.error("unable to get peer name for socket");
    return rfb::strDup("");
  }

  if (sa.u.sa.sa_family == AF_INET6) {
    char buffer[INET6_ADDRSTRLEN + 2];
    int ret;

    buffer[0] = '[';

    ret = getnameinfo(&sa.u.sa, sizeof(sa.u.sin6),
                      buffer + 1, sizeof(buffer) - 2, NULL, 0,
                      NI_NUMERICHOST);
    if (ret != 0) {
      vlog.error("unable to convert peer name to a string");
      return rfb::strDup("");
    }

    strcat(buffer, "]");

    return rfb::strDup(buffer);
  }

  if (sa.u.sa.sa_family == AF_INET) {
    char *name;

    name = inet_ntoa(sa.u.sin.sin_addr);
    if (name == NULL) {
      vlog.error("unable to convert peer name to a string");
      return rfb::strDup("");
    }

    return rfb::strDup(name);
  }

  vlog.error("unknown address family for socket");
  return rfb::strDup("");
}

char* TcpSocket::getPeerEndpoint() {
  rfb::CharArray address; address.buf = getPeerAddress();
  vnc_sockaddr_t sa;
  socklen_t sa_size = sizeof(sa);
  int port;

  getpeername(getFd(), &sa.u.sa, &sa_size);

  if (sa.u.sa.sa_family == AF_INET6)
    port = ntohs(sa.u.sin6.sin6_port);
  else if (sa.u.sa.sa_family == AF_INET)
    port = ntohs(sa.u.sin.sin_port);
  else
    port = 0;

  int buflen = strlen(address.buf) + 32;
  char* buffer = new char[buflen];
  sprintf(buffer, "%s::%d", address.buf, port);
  return buffer;
}

bool TcpSocket::enableNagles(bool enable) {
  int one = enable ? 0 : 1;
  if (setsockopt(getFd(), IPPROTO_TCP, TCP_NODELAY,
                 (char *)&one, sizeof(one)) < 0) {
    int e = errorNumber;
    vlog.error("unable to setsockopt TCP_NODELAY: %d", e);
    return false;
  }
  return true;
}

TcpListener::TcpListener(int sock) : SocketListener(sock)
{
}

TcpListener::TcpListener(const struct sockaddr *listenaddr,
                         socklen_t listenaddrlen)
{
  int one = 1;
  vnc_sockaddr_t sa;
  int sock;

  if ((sock = socket (listenaddr->sa_family, SOCK_STREAM, 0)) < 0)
    throw SocketException("unable to create listening socket", errorNumber);

  memcpy (&sa, listenaddr, listenaddrlen);
#ifdef IPV6_V6ONLY
  if (listenaddr->sa_family == AF_INET6) {
    if (setsockopt (sock, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&one, sizeof(one))) {
      int e = errorNumber;
      closesocket(sock);
      throw SocketException("unable to set IPV6_V6ONLY", e);
    }
  }
#endif /* defined(IPV6_V6ONLY) */

#ifdef FD_CLOEXEC
  // - By default, close the socket on exec()
  fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif

  // SO_REUSEADDR is broken on Windows. It allows binding to a port
  // that already has a listening socket on it. SO_EXCLUSIVEADDRUSE
  // might do what we want, but requires investigation.
#ifndef WIN32
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&one, sizeof(one)) < 0) {
    int e = errorNumber;
    closesocket(sock);
    throw SocketException("unable to create listening socket", e);
  }
#endif

  if (bind(sock, &sa.u.sa, listenaddrlen) == -1) {
    int e = errorNumber;
    closesocket(sock);
    throw SocketException("failed to bind socket", e);
  }

  listen(sock);
}

Socket* TcpListener::createSocket(int fd) {
  return new TcpSocket(fd);
}

void TcpListener::getMyAddresses(std::list<char*>* result) {
  struct addrinfo *ai, *current, hints;

  initSockets();

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Windows doesn't like NULL for service, so specify something
  if ((getaddrinfo(NULL, "1", &hints, &ai)) != 0)
    return;

  for (current= ai; current != NULL; current = current->ai_next) {
    switch (current->ai_family) {
    case AF_INET:
      if (!UseIPv4)
        continue;
      break;
    case AF_INET6:
      if (!UseIPv6)
        continue;
      break;
    default:
      continue;
    }

    char *addr = new char[INET6_ADDRSTRLEN];

    getnameinfo(current->ai_addr, current->ai_addrlen, addr, INET6_ADDRSTRLEN,
                NULL, 0, NI_NUMERICHOST);

    result->push_back(addr);
  }

  freeaddrinfo(ai);
}

int TcpListener::getMyPort() {
  return getSockPort(getFd());
}


void network::createLocalTcpListeners(std::list<SocketListener*> *listeners,
                                      int port)
{
  struct addrinfo ai[2];
  vnc_sockaddr_t sa[2];

  memset(ai, 0, sizeof(ai));
  memset(sa, 0, sizeof(sa));

  sa[0].u.sin.sin_family = AF_INET;
  sa[0].u.sin.sin_port = htons (port);
  sa[0].u.sin.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

  ai[0].ai_family = sa[0].u.sin.sin_family;
  ai[0].ai_addr = &sa[0].u.sa;
  ai[0].ai_addrlen = sizeof(sa[0].u.sin);
  ai[0].ai_next = &ai[1];

  sa[1].u.sin6.sin6_family = AF_INET6;
  sa[1].u.sin6.sin6_port = htons (port);
  sa[1].u.sin6.sin6_addr = in6addr_loopback;

  ai[1].ai_family = sa[1].u.sin6.sin6_family;
  ai[1].ai_addr = &sa[1].u.sa;
  ai[1].ai_addrlen = sizeof(sa[1].u.sin6);
  ai[1].ai_next = NULL;

  createTcpListeners(listeners, ai);
}

void network::createTcpListeners(std::list<SocketListener*> *listeners,
                                 const char *addr,
                                 int port)
{
  struct addrinfo *ai, hints;
  char service[16];
  int result;

  initSockets();

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  snprintf (service, sizeof (service) - 1, "%d", port);
  service[sizeof (service) - 1] = '\0';
  if ((result = getaddrinfo(addr, service, &hints, &ai)) != 0)
    throw GAIException("unable to resolve listening address", result);

  try {
    createTcpListeners(listeners, ai);
  } catch(...) {
    freeaddrinfo(ai);
    throw;
  }

  freeaddrinfo(ai);
}

void network::createTcpListeners(std::list<SocketListener*> *listeners,
                                 const struct addrinfo *ai)
{
  const struct addrinfo *current;
  std::list<SocketListener*> new_listeners;

  initSockets();

  for (current = ai; current != NULL; current = current->ai_next) {
    switch (current->ai_family) {
    case AF_INET:
      if (!UseIPv4)
        continue;
      break;

    case AF_INET6:
      if (!UseIPv6)
        continue;
      break;

    default:
      continue;
    }

    try {
      new_listeners.push_back(new TcpListener(current->ai_addr,
                                              current->ai_addrlen));
    } catch (SocketException& e) {
      // Ignore this if it is due to lack of address family support on
      // the interface or on the system
      if (e.err != EADDRNOTAVAIL && e.err != EAFNOSUPPORT) {
        // Otherwise, report the error
        while (!new_listeners.empty()) {
          delete new_listeners.back();
          new_listeners.pop_back();
        }
        throw;
      }
    }
  }

  if (new_listeners.empty ())
    throw SocketException("createTcpListeners: no addresses available",
                          EADDRNOTAVAIL);

  listeners->splice (listeners->end(), new_listeners);
}


TcpFilter::TcpFilter(const char* spec) {
  rfb::CharArray tmp;
  tmp.buf = rfb::strDup(spec);
  while (tmp.buf) {
    rfb::CharArray first;
    rfb::strSplit(tmp.buf, ',', &first.buf, &tmp.buf);
    if (strlen(first.buf))
      filter.push_back(parsePattern(first.buf));
  }
}

TcpFilter::~TcpFilter() {
}


static bool
patternMatchIP(const TcpFilter::Pattern& pattern, vnc_sockaddr_t *sa) {
  switch (pattern.address.u.sa.sa_family) {
    unsigned long address;

  case AF_INET:
    if (sa->u.sa.sa_family != AF_INET)
      return false;

    address = sa->u.sin.sin_addr.s_addr;
    if (address == htonl (INADDR_NONE)) return false;
    return ((pattern.address.u.sin.sin_addr.s_addr &
             pattern.mask.u.sin.sin_addr.s_addr) ==
            (address & pattern.mask.u.sin.sin_addr.s_addr));

  case AF_INET6:
    if (sa->u.sa.sa_family != AF_INET6)
      return false;

    for (unsigned int n = 0; n < 16; n++) {
      unsigned int bits = (n + 1) * 8;
      unsigned int mask;
      if (pattern.prefixlen > bits)
        mask = 0xff;
      else {
        unsigned int lastbits = 0xff;
        lastbits <<= bits - pattern.prefixlen;
        mask = lastbits & 0xff;
      }

      if ((pattern.address.u.sin6.sin6_addr.s6_addr[n] & mask) !=
          (sa->u.sin6.sin6_addr.s6_addr[n] & mask))
        return false;

      if (mask < 0xff)
        break;
    }

    return true;

  case AF_UNSPEC:
    // Any address matches
    return true;

  default:
    break;
  }

  return false;
}

bool
TcpFilter::verifyConnection(Socket* s) {
  rfb::CharArray name;
  vnc_sockaddr_t sa;
  socklen_t sa_size = sizeof(sa);

  if (getpeername(s->getFd(), &sa.u.sa, &sa_size) != 0)
    return false;

  name.buf = s->getPeerAddress();
  std::list<TcpFilter::Pattern>::iterator i;
  for (i=filter.begin(); i!=filter.end(); i++) {
    if (patternMatchIP(*i, &sa)) {
      switch ((*i).action) {
      case Accept:
        vlog.debug("ACCEPT %s", name.buf);
        return true;
      case Query:
        vlog.debug("QUERY %s", name.buf);
        s->setRequiresQuery();
        return true;
      case Reject:
        vlog.debug("REJECT %s", name.buf);
        return false;
      }
    }
  }

  vlog.debug("[REJECT] %s", name.buf);
  return false;
}


TcpFilter::Pattern TcpFilter::parsePattern(const char* p) {
  TcpFilter::Pattern pattern;

  rfb::CharArray addr, pref;
  bool prefix_specified;
  int family;

  initSockets();

  prefix_specified = rfb::strSplit(&p[1], '/', &addr.buf, &pref.buf);
  if (addr.buf[0] == '\0') {
    // Match any address
    memset (&pattern.address, 0, sizeof (pattern.address));
    pattern.address.u.sa.sa_family = AF_UNSPEC;
    pattern.prefixlen = 0;
  } else {
    struct addrinfo hints;
    struct addrinfo *ai;
    char *p = addr.buf;
    int result;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;

    // Take out brackets, if present
    if (*p == '[') {
      size_t len;
      p++;
      len = strlen (p);
      if (len > 0 && p[len - 1] == ']')
        p[len - 1] = '\0';
    }

    if ((result = getaddrinfo (p, NULL, &hints, &ai)) != 0) {
      throw GAIException("unable to resolve host by name", result);
    }

    memcpy (&pattern.address.u.sa, ai->ai_addr, ai->ai_addrlen);
    freeaddrinfo (ai);

    family = pattern.address.u.sa.sa_family;

    if (prefix_specified) {
      if (family == AF_INET &&
          rfb::strContains(pref.buf, '.')) {
        throw Exception("mask no longer supported for filter, "
                        "use prefix instead");
      }

      pattern.prefixlen = (unsigned int) atoi(pref.buf);
    } else {
      switch (family) {
      case AF_INET:
        pattern.prefixlen = 32;
        break;
      case AF_INET6:
        pattern.prefixlen = 128;
        break;
      default:
        throw Exception("unknown address family");
      }
    }
  }

  family = pattern.address.u.sa.sa_family;

  if (pattern.prefixlen > (family == AF_INET ? 32: 128))
    throw Exception("invalid prefix length for filter address: %u",
                    pattern.prefixlen);

  // Compute mask from address and prefix length
  memset (&pattern.mask, 0, sizeof (pattern.mask));
  switch (family) {
    unsigned long mask;
  case AF_INET:
    mask = 0;
    for (unsigned int i=0; i<pattern.prefixlen; i++)
      mask |= 1<<(31-i);
    pattern.mask.u.sin.sin_addr.s_addr = htonl(mask);
    break;

  case AF_INET6:
    for (unsigned int n = 0; n < 16; n++) {
      unsigned int bits = (n + 1) * 8;
      if (pattern.prefixlen > bits)
        pattern.mask.u.sin6.sin6_addr.s6_addr[n] = 0xff;
      else {
        unsigned int lastbits = 0xff;
        lastbits <<= bits - pattern.prefixlen;
        pattern.mask.u.sin6.sin6_addr.s6_addr[n] = lastbits & 0xff;
        break;
      }
    }
    break;
  case AF_UNSPEC:
    // No mask to compute
    break;
  default:
    ; /* not reached */
  }

  switch(p[0]) {
  case '+': pattern.action = TcpFilter::Accept; break;
  case '-': pattern.action = TcpFilter::Reject; break;
  case '?': pattern.action = TcpFilter::Query; break;
  };

  return pattern;
}

char* TcpFilter::patternToStr(const TcpFilter::Pattern& p) {
  rfb::CharArray addr;
  char buffer[INET6_ADDRSTRLEN + 2];

  if (p.address.u.sa.sa_family == AF_INET) {
    getnameinfo(&p.address.u.sa, sizeof(p.address.u.sin),
                buffer, sizeof (buffer), NULL, 0, NI_NUMERICHOST);
    addr.buf = rfb::strDup(buffer);
  } else if (p.address.u.sa.sa_family == AF_INET6) {
    buffer[0] = '[';
    getnameinfo(&p.address.u.sa, sizeof(p.address.u.sin6),
                buffer + 1, sizeof (buffer) - 2, NULL, 0, NI_NUMERICHOST);
    strcat(buffer, "]");
    addr.buf = rfb::strDup(buffer);
  } else
    addr.buf = rfb::strDup("");

  char action;
  switch (p.action) {
  case Accept: action = '+'; break;
  case Reject: action = '-'; break;
  default:
  case Query: action = '?'; break;
  };
  size_t resultlen = (1                   // action
                      + strlen (addr.buf) // address
                      + 1                 // slash
                      + 3                 // prefix length, max 128
                      + 1);               // terminating nul
  char* result = new char[resultlen];
  if (addr.buf[0] == '\0')
    snprintf(result, resultlen, "%c", action);
  else
    snprintf(result, resultlen, "%c%s/%u", action, addr.buf, p.prefixlen);

  return result;
}
