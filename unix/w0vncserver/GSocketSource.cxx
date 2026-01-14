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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <map>

#include <glib.h>

#include <network/TcpSocket.h>
#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rdr/FdOutStream.h>

#include "w0vncserver.h"
#include "GSocketSource.h"

static core::LogWriter vlog("GSocketMonitor");

static std::map<GSource*, GSocketSource*> sources;

struct SocketState {
  void* tag;
  bool prevHadBufferedData;
};

struct ListenerReadyEvent {
  GSocketSource* instance;
  network::SocketListener* listener;
  GIOCondition condition;
};

GSourceFuncs GSocketSource::sourceFuncs {
  .prepare = [](GSource* source, int* timeout) {
    assert(sources.count(source) != 0);
    return sources[source]->prepare(timeout);
  },
  .check = nullptr,
  .dispatch = [](GSource* source, GSourceFunc, void*) {
    assert(sources.count(source) != 0);
    return sources[source]->dispatch();
  },
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

GSocketSource::GSocketSource(rfb::VNCServer* server_,
                             std::list<network::SocketListener*> *listeners_)
  : source(nullptr), server(server_), listeners(listeners_)
{
  source = g_source_new(&sourceFuncs, sizeof(GSource));
  previousCondition = G_IO_IN;

  sources[source] = this;
}

GSocketSource::~GSocketSource()
{
  std::list<network::Socket*> sockets;

  for (GIOChannel* channel : channels)
    g_io_channel_unref(channel);

  for (ListenerReadyEvent* event : listenerEvents)
    delete event;

  server->getSockets(&sockets);
  for (network::Socket* sock : sockets) {
    server->removeSocket(sock);
    delete sock;
  }

  sources.erase(source);
  // GLib will take care of the monitored FDs on our source
  g_source_unref(source);
  g_source_destroy(source);
}

void GSocketSource::attach(GMainContext* context)
{
  assert(source);

  g_source_attach((GSource*)source, context);
}

void GSocketSource::listen()
{
  assert(listeners->size());

  for (network::SocketListener* listener : *listeners) {
    ListenerReadyEvent* event;
    GIOChannel* channel;
    int fd;

    fd = listener->getFd();

    channel = g_io_channel_unix_new(fd);
    channels.push_back(channel);

    event = new ListenerReadyEvent();
    event->instance = this;
    event->listener = listener;

    listenerEvents.push_back(event);

    g_io_add_watch(
      channel,
      static_cast<GIOCondition>((G_IO_IN | G_IO_ERR | G_IO_HUP)),
      [](GIOChannel*, GIOCondition condition, void* data) {
        ListenerReadyEvent* event_;

        event_ = static_cast<ListenerReadyEvent*>(data);
        event_->condition = condition;
        return event_->instance->handleListenerReady(event_);
      },
      event);
  }
}

int GSocketSource::prepare(int* timeout)
{
  std::list<network::Socket*> sockets;

  server->getSockets(&sockets);

  *timeout = -1;

  for (network::Socket* sock : sockets) {
    int fd;
    SocketState* state;

    fd = sock->getFd();
    state = &fdMap[fd];
    assert(state->tag);

    if (sock->isShutdownRead()) {
      vlog.debug("Client gone, sock %d", fd);
      g_source_remove_unix_fd(source, state->tag);
      server->removeSocket(sock);
      delete sock;
      assert(fdMap.erase(fd));
      return FALSE;
    }

    if (state->prevHadBufferedData != sock->outStream().hasBufferedData()) {
      // FIXME: Calling g_source_modify_unix_fd() will cause the main
      // loop to wake up immediately if it is currently blocked.
      // Calling it while we are in prepare() will skip polling FDs
      // and just dispatch() instead, essentially causing a busy wait.
      // To circumvent this, we only modify which events we want to
      // listen for if they differ from the last event loop iteration.
      GIOCondition newEvents = static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR);
      if (sock->outStream().hasBufferedData())
        newEvents = static_cast<GIOCondition>(newEvents | G_IO_OUT);
      g_source_modify_unix_fd(source, state->tag, newEvents);
      state->prevHadBufferedData = sock->outStream().hasBufferedData();
    }
  }

  return FALSE;
}

int GSocketSource::dispatch()
{
  std::list<network::Socket*> sockets;

  server->getSockets(&sockets);

  for (network::Socket* sock : sockets) {
    GIOCondition events;
    int fd;
    SocketState state;

    fd = sock->getFd();
    state = fdMap[fd];
    assert(state.tag);

    events = g_source_query_unix_fd(source, state.tag);

    if (events & G_IO_HUP || events & G_IO_ERR) {
      vlog.debug("Client gone, sock %d", fd);
      g_source_remove_unix_fd(source, state.tag);
      server->removeSocket(sock);
      delete sock;
      assert(fdMap.erase(fd));
      return G_SOURCE_CONTINUE;
    }

    if (events & G_IO_IN)
      server->processSocketReadEvent(sock);
    if (events & G_IO_OUT)
      server->processSocketWriteEvent(sock);
  }

  return G_SOURCE_CONTINUE;
}

int GSocketSource::handleListenerReady(ListenerReadyEvent* event)
{
  network::Socket* sock;
  network::SocketListener* listener;
  GIOCondition condition;
  SocketState state;
  int fd;
  void* tag;

  listener = event->listener;
  condition = event->condition;

  if (condition & G_IO_ERR || condition & G_IO_HUP) {
    vlog.status("Client connection error");
    return G_SOURCE_CONTINUE;
  }

  sock = listener->accept();
  if (!sock) {
    vlog.status("Client connection rejected");
    return G_SOURCE_CONTINUE;
  }

  if (!server->addSocket(sock)) {
    delete sock;
    return G_SOURCE_CONTINUE;
  }

  fd = sock->getFd();
  tag = g_source_add_unix_fd(source, fd,
                             static_cast<GIOCondition>((G_IO_IN | G_IO_HUP | G_IO_ERR)));
  state.tag = tag;
  state.prevHadBufferedData = false;

  fdMap[fd] = state;

  return G_SOURCE_CONTINUE;
}
