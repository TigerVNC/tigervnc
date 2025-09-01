/* Copyright 2025 Adam Halim for Cendio AB
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

#ifndef __G_SOCKET_SOURCE__H
#define __G_SOCKET_SOURCE__H

#include <list>
#include <map>

#include <glib.h>

namespace rfb { class VNCServer; }
namespace network { class SocketListener; }
struct SocketState;
struct ListenerReadyEvent;

class GSocketSource {
public:
  GSocketSource(rfb::VNCServer* server,
                 std::list<network::SocketListener*> *listeners);
  ~GSocketSource();

  void attach(GMainContext* context);
  void listen();

private:
  int prepare(int* timeout);
  int dispatch();
  int handleListenerReady(ListenerReadyEvent* event);

private:
  GSource* source;
  GIOCondition previousCondition;
  rfb::VNCServer* server;
  std::map<int, SocketState> fdMap;
  std::list<network::SocketListener*>* listeners;
  std::list<GIOChannel*> channels;
  static GSourceFuncs sourceFuncs;
  std::list<ListenerReadyEvent*> listenerEvents;
};

#endif // __G_SOCKET_SOURCE__H
