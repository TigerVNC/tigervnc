#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>

#include <network/TcpSocket.h>
#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rdr/FdOutStream.h>

#include <map>

#include "GSocketMonitor.h"

static core::LogWriter vlog("GSocketMonitor");

struct ListenerReadyEvent {
  GSocketSource* source;
  network::SocketListener* listener;
};

struct SocketState {
  void* tag;
  bool prevHadBufferedData;
};

struct GSocketSource {
  GSource base;
  GIOCondition previousCondition;
  rfb::VNCServer* server;
  std::map<int, SocketState> fdMap;
};

static int prepare(GSource* source, int* timeout);
static int dispatch(GSource* source, GSourceFunc callback,
                    void* userData);
static int handleListenerReady(GIOChannel* source,
                               GIOCondition condition, void* data);

static GSourceFuncs sourceFuncs {
  .prepare = prepare,
  .check = nullptr,
  .dispatch = dispatch,
  .finalize = nullptr,
  .closure_callback = nullptr,
  .closure_marshal = nullptr
};

static int prepare(GSource* source, int* timeout)
{
  GSocketSource* data;
  std::list<network::Socket*> sockets;

  data = (GSocketSource*)source;
  data->server->getSockets(&sockets);

  *timeout = -1;

  for (network::Socket* sock : sockets) {
    int fd;
    SocketState state;

    fd = sock->getFd();
    state = data->fdMap[fd];
    assert(state.tag);

    if (sock->isShutdown()) {
      vlog.debug("Client gone, sock %d", fd);
      g_source_remove_unix_fd(source, state.tag);
      data->server->removeSocket(sock);
      delete sock;
      assert(data->fdMap.erase(fd));
      return FALSE;
    }

    if (state.prevHadBufferedData != sock->outStream().hasBufferedData()) {
      // FIXME: Calling g_source_modify_unix_fd() will cause the main
      // loop to wake up immediately if it is currently blocked.
      // Calling it while we are in prepare() will skip polling FDs
      // and just dispatch() instead, essentially causing a busy wait.
      // To circumvent this, we only modify which events we want to
      // listen for if they differ from the last event loop iteration.
      GIOCondition newEvents = static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR);
      if (sock->outStream().hasBufferedData())
        newEvents = static_cast<GIOCondition>(newEvents | G_IO_OUT);
      g_source_modify_unix_fd(source, state.tag, newEvents);
    }

    state.prevHadBufferedData = sock->outStream().hasBufferedData();
  }

  return FALSE;
}

static int dispatch(GSource * source, GSourceFunc /* callback */,
                    void* /* userData */) {
  GSocketSource* data;
  std::list<network::Socket*> sockets;

  data = (GSocketSource*)source;
  data->server->getSockets(&sockets);

  for (network::Socket* sock : sockets) {
    GIOCondition events;
    int fd;
    SocketState state;

    fd = sock->getFd();
    state = data->fdMap[fd];
    assert(state.tag);

    events = g_source_query_unix_fd(source, state.tag);

    if (events & G_IO_HUP || events & G_IO_ERR) {
      vlog.debug("Client gone, sock %d", fd);
      g_source_remove_unix_fd(source, state.tag);
      data->server->removeSocket(sock);
      delete sock;
      assert(data->fdMap.erase(fd));
      return G_SOURCE_CONTINUE;
    }

    if (events & G_IO_IN)
      data->server->processSocketReadEvent(sock);
    if (events & G_IO_OUT)
      data->server->processSocketWriteEvent(sock);
  }

  return G_SOURCE_CONTINUE;
}

int handleListenerReady(GIOChannel* /* source */,
                        GIOCondition condition , void* data) {
  ListenerReadyEvent* event;
  network::Socket* sock;
  SocketState state;
  int fd;
  void* tag;

  event = static_cast<ListenerReadyEvent*>(data);
  assert(event);

  if (condition & G_IO_ERR || condition & G_IO_HUP) {
    vlog.status("Client connection error");
    return G_SOURCE_CONTINUE;
  }

  sock = event->listener->accept();
  if (!sock) {
    vlog.status("Client connection rejected");
    return G_SOURCE_CONTINUE;
  }

  event->source->server->addSocket(sock);
  fd = sock->getFd();
  tag = g_source_add_unix_fd((GSource*)event->source, fd,
                              static_cast<GIOCondition>((G_IO_IN | G_IO_HUP | G_IO_ERR)));
  state.tag = tag;
  state.prevHadBufferedData = false;

  event->source->fdMap[fd] = state;

  return G_SOURCE_CONTINUE;
}

GSocketMonitor::GSocketMonitor(std::list<network::SocketListener*>
                               *listeners)
  : listeners_(listeners)
  {
    source = (GSocketSource*)g_source_new(&sourceFuncs, sizeof(GSocketSource));
    source->server = nullptr;
    source->previousCondition = G_IO_IN;

    // glib won't initialize our map
    new (&source->fdMap) std::map<int, void*>();
}

GSocketMonitor::~GSocketMonitor() {
  for (ListenerReadyEvent* event : readyEvents)
    delete event;
  for (GIOChannel* channel : channels)
    g_io_channel_unref(channel);

  source->fdMap.~map();

  // GLib will take care of the monitored FDs on our source
  g_source_unref((GSource*)source);
}

void GSocketMonitor::attach(GMainContext * context) {
  assert(source);

  g_source_attach((GSource*)source, context);
}

void GSocketMonitor::listen(rfb::VNCServer * server) {
  for (network::SocketListener* listener : *listeners_) {
    GIOChannel* channel;
    int fd;
    ListenerReadyEvent* event;

    event = new ListenerReadyEvent();
    fd = listener->getFd();
    source->server = server;

    event->listener = listener;
    event->source = source;
    readyEvents.push_back(event);

    channel = g_io_channel_unix_new(fd);
    channels.push_back(channel);
    g_io_add_watch(channel,
                    static_cast<GIOCondition>((G_IO_IN | G_IO_ERR | G_IO_HUP)),
                    handleListenerReady, event);
  }
}
