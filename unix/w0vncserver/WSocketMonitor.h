#ifndef __W_SOCKET_MONITOR__H
#define __W_SOCKET_MONITOR__H

#include "glib.h"
#include <network/TcpSocket.h>
#include <rfb/VNCServerST.h>
#include <list>

struct sock_event_ctx {
  rfb::VNCServerST* server;
  network::SocketListener* listener;
  network::Socket* sock;
};


class WSocketMonitor {
public:
  WSocketMonitor(char* address, int rfbport);
  ~WSocketMonitor();

  void listen(rfb::VNCServerST* server);

private:
  static int sock_process(GIOChannel* source, GIOCondition condition,
                          void* data);
  static int accept_connection(GIOChannel* source,
                               GIOCondition condition, void* data);

private:
  std::list<network::SocketListener*> listeners;
  std::list<GIOChannel*> channels;
};

#endif // __W_SOCKET_MONITOR__H