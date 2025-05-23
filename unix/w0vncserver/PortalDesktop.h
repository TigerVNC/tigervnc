
#ifndef __PORTAL_DESKTOP_H__
#define __PORTAL_DESKTOP_H__

#include <core/Timer.h>
#include <rfb/SDesktop.h>
#include <rfb/VNCServer.h>

#include "GSignalWrapper.h"
#include "PipewirePixelBuffer.h"
#include "RFBTimerSource.h"
#include "WSocketMonitor.h"

class RemoteDesktop;

class PortalDesktop : public rfb::SDesktop
{
public:
  PortalDesktop(GMainLoop* loop, int rfbport);
  virtual ~PortalDesktop();

  // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  void queryConnection(network::Socket* sock,
                       const char* userName) override;
  void terminate() override;
  unsigned int setScreenLayout(int fb_width, int fb_height,
                               const rfb::ScreenSet& layout) override;
  void keyEvent(uint32_t keysym, uint32_t keycode, bool down) override;
  void pointerEvent(const core::Point& pos,
                    uint16_t buttonMask) override;
  void handleClipboardRequest() override;
  void handleClipboardAnnounce(bool available) override;
  void handleClipboardData(const char* data) override;

  // Start running the Desktop
  void run();

  // Check if portals implementations are available
  static bool portalsAvailable();

private:
  // Start listening for incoming connections
  void listen();
  // Initialize PipeWire
  void startPipewire(int fd, uint32_t nodeId);

protected:
  rfb::VNCServer* server_;

  RFBTimerSource* timerSource_;
  WSocketMonitor* monitor_;
  RemoteDesktop* remoteDesktop_;
  PipeWirePixelBuffer* pipewire_;

private:
  int rfbport_;
  bool running;
  GMainLoop* loop_;
  GSignalWrapper* signalWrapper_;
};

#endif // __PORTAL_DESKTOP_H__
