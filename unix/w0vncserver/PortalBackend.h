#ifndef __PORTAL_BACKEND_H__
#define __PORTAL_BACKEND_H__

#include "Pipewire.h"
#include "WaylandBackend.h"

#include "RFBTimerSource.h"
#include "WSocketMonitor.h"
#include "WDesktop.h"
#include "portals/RemoteDesktop.h"
#include "portals/Screencast.h"

class PortalBackend : public WaylandBackend {
public:
  PortalBackend(int rfbport);
  ~PortalBackend();

  // -=- WaylandRemote interface -=-
  void init() override;
  void pointerEvent(const core::Point& pos,
                    uint16_t buttonMask) override;
  void keyEvent(uint32_t keysym, uint32_t keycode, bool down) override;
  unsigned int setScreenLayout(int fb_width, int fb_height,
                               const rfb::ScreenSet& layout) override;

  // -=- Portal methods -=-
  void select_sources();
  void select_devices();
  void open_pipewire_remote();
  void start();

  // Start listening for incoming connections
  void listen();
  void start_pipewire(int fd, uint32_t node_id);

  void set_session_handle(char* handle);
  char* session_handle() { return session_handle_; }
  std::vector<rd_stream*> streams();
  WDesktop* desktop() { return desktop_; }


private:
  GMainLoop* loop_;
  rfb::VNCServerST* server_;
  RFBTimerSource* timerSource_;
  WSocketMonitor* monitor_;
  WDesktop* desktop_;
  RemoteDesktop* remoteDesktop_;
  Screencast* screencast_;
  Pipewire* pipewire_;
  int rfbport_;
  char* session_handle_;

};
#endif // __PORTAL_BACKEND_H__
