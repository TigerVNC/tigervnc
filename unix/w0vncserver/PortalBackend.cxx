#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "glib-unix.h"

#include <rfb/VNCServerST.h>

#include "portals/RemoteDesktop.h"
#include "PortalBackend.h"
#include "Pipewire.h"

int CleanupSignalHandler(void* data)
{
  GMainLoop* loop;

  loop = (GMainLoop*)data;
  assert(loop);

  g_main_loop_quit(loop);
  return 1;
}


PortalBackend::PortalBackend(int rfbport)
  : server_(nullptr), timerSource_(nullptr), monitor_(nullptr),
    pipewire_(nullptr), rfbport_(rfbport), session_handle_(nullptr)
{
  loop_ = g_main_loop_new(nullptr, false);

  desktop_ = new WDesktop(this);
  remoteDesktop_ = new RemoteDesktop(this);
  screencast_ = new Screencast(remoteDesktop_->connection(), this);
}

PortalBackend::~PortalBackend()
{
  delete timerSource_;
  delete monitor_;
  delete server_;

  delete desktop_;
  delete screencast_;
  delete remoteDesktop_;
  delete pipewire_;

  free(session_handle_);
}

void PortalBackend::init()
{
  // Setup Portal signals/callbacks
  remoteDesktop_->init();

  g_unix_signal_add(SIGINT, CleanupSignalHandler, loop_);
  g_unix_signal_add(SIGTERM, CleanupSignalHandler, loop_);
  g_unix_signal_add(SIGHUP, CleanupSignalHandler, loop_);

  g_main_loop_run(loop_);
}

void PortalBackend::pointerEvent(const core::Point& pos,
                                       uint16_t buttonMask)
{
  remoteDesktop_->pointerEvent(pos.x, pos.y, buttonMask);
}

void PortalBackend::keyEvent(uint32_t keysym, uint32_t keycode,
                                   bool down) {
  remoteDesktop_->keyEvent(keysym, keycode, down);
}

unsigned int
PortalBackend::setScreenLayout(int fb_width, int fb_height,
                                     const rfb::ScreenSet& layout) {
  return desktop_->setScreenLayout(fb_width, fb_height, layout);
}

void PortalBackend::select_sources()
{
  assert(session_handle_);
  screencast_->select_sources();
}

void PortalBackend::select_devices()
{
  remoteDesktop_->select_devices();
}

void PortalBackend::open_pipewire_remote()
{
  screencast_->open_pipewire_remote();
}

void PortalBackend::start()
{
  remoteDesktop_->start();
}

void PortalBackend::listen()
{
  server_ = new rfb::VNCServerST("Desktop", desktop_);
  monitor_ = new WSocketMonitor(nullptr, rfbport_);
  timerSource_ = new RFBTimerSource();

  monitor_->listen(server_);
  timerSource_->attach(g_main_loop_get_context(loop_));
}

void PortalBackend::start_pipewire(int fd, uint32_t node_id)
{
  desktop_->init(server_);
  pipewire_ = new Pipewire(fd, node_id, desktop_);
  pipewire_->start();

  listen();
}

void PortalBackend::set_session_handle(char* handle)
{
  assert(session_handle_ == nullptr);

  session_handle_ = strdup(handle);
}

std::vector<rd_stream*>  PortalBackend::streams()
{
  return remoteDesktop_->streams();
}
