#ifndef __G_SOCKET_MONITOR__H
#define __G_SOCKET_MONITOR__H

#include <list>

#include <glib.h>

#include <vector>

namespace rfb { class VNCServer; }
namespace network { class SocketListener; }
struct GSocketSource;
struct ListenerReadyEvent;

class GSocketMonitor {
public:
  GSocketMonitor(std::list<network::SocketListener*> *listeners);
  ~GSocketMonitor();

  void attach(GMainContext* context);
  void listen(rfb::VNCServer* server);
private:
  GSocketSource* source;
  std::list<network::SocketListener*>* listeners_;
  std::vector<ListenerReadyEvent*> readyEvents;
  std::list<GIOChannel*> channels;
};

#endif // __G_SOCKET_MONITOR__H
