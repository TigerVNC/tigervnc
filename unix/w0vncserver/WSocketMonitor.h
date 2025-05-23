#ifndef __W_SOCKET_MONITOR__H
#define __W_SOCKET_MONITOR__H

#include <list>

#include <glib.h>

#include <network/TcpSocket.h>
#include <rfb/VNCServerST.h>

struct sock_event_ctx {
  rfb::VNCServer* server;
  network::SocketListener* listener;
  network::Socket* sock;
};


class WSocketMonitor {
public:
  WSocketMonitor(char* address, int rfbport);
  ~WSocketMonitor();

  void listen(rfb::VNCServer* server);

private:
  static int sockProcess(GIOChannel* source, GIOCondition condition,
                         void* data);
  static int acceptConnection(GIOChannel* source,
                              GIOCondition condition, void* data);

private:
  std::list<network::SocketListener*> listeners;
  std::list<GIOChannel*> channels;
};

#endif // __W_SOCKET_MONITOR__H
