#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>

#include <rdr/FdOutStream.h>
#include <core/LogWriter.h>

#include "GSocketMonitor.h"


static core::LogWriter vlog("gsocketlistener");


GSocketMonitor::GSocketMonitor(char* address, int rfbport)
{
  network::createTcpListeners(&listeners, address, rfbport);
}

GSocketMonitor::~GSocketMonitor()
{
  for (network::SocketListener* listener : listeners)
    delete listener;

  for (GIOChannel* channel : channels)
    g_io_channel_unref(channel);
}

void GSocketMonitor::listen(rfb::VNCServer* server)
{
  for (network::SocketListener* listener : listeners) {
    int fd;
    GIOChannel* channel;
    sock_event_ctx* data;

    // FIXME: This data should be freed
    data = new sock_event_ctx;
    data->server = server;
    data->listener = listener;

    fd = listener->getFd();
    channel = g_io_channel_unix_new(fd);
    g_io_add_watch(channel,
                  (GIOCondition)(G_IO_IN | G_IO_ERR | G_IO_HUP),
                  acceptConnection, data);

    channels.push_back(channel);
  }
}

int GSocketMonitor::sockProcess(GIOChannel* source,
                                GIOCondition condition, void* data)
{
  (void) condition;
  sock_event_ctx* data_;
  network::Socket* sock;

  data_ = (sock_event_ctx*)data;
  assert(data_->sock);
  sock = data_->sock;

  if (sock->isShutdown()) {
    data_->server->removeSocket(sock);
    g_io_channel_unref(source);
    return false;
  }

  data_->server->processSocketReadEvent(sock);

  if (sock->outStream().hasBufferedData())
    data_->server->processSocketWriteEvent(sock);

  return true;
}

int GSocketMonitor::acceptConnection(GIOChannel* source,
                                     GIOCondition condition, void* data)
{
  (void) source;
  (void) condition;
  sock_event_ctx* data_;
  network::Socket* sock;
  GIOChannel* channel;
  int fd;

  data_ = (sock_event_ctx*)data;
  assert(data_);

  sock = data_->listener->accept();
  if (!sock) {
    vlog.status("Client connection rejected");
    delete data_;
    return false;
  }

  data_->server->addSocket(sock);
  data_->sock = sock;

  fd = sock->getFd();
  channel = g_io_channel_unix_new(fd);
  g_io_add_watch(channel,(GIOCondition)G_IO_IN, sockProcess, data_);

  vlog.debug("added sock %d", sock->getFd());
  return true;
}
