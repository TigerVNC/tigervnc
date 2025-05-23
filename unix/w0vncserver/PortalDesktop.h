
#ifndef __PORTAL_DESKTOP_H__
#define __PORTAL_DESKTOP_H__

#include <glib.h>

#include <core/Timer.h>
#include <rfb/SDesktop.h>

class PipeWirePixelBuffer;

namespace rfb { class VNCServer; }

class RemoteDesktop;
class GSocketMonitor;
class RFBTimerSource;
class PortalDesktop : public rfb::SDesktop
{
public:
  PortalDesktop(GMainLoop* loop);
  virtual ~PortalDesktop();

  // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  void stop() override;
  void queryConnection(network::Socket* sock,
                       const char* userName) override;
  void terminate() override;
  unsigned int setScreenLayout(int fb_width, int fb_height,
                               const rfb::ScreenSet& layout) override;

  // Check if portals implementations are available
  static bool available();

protected:
  rfb::VNCServer* server_;

  RemoteDesktop* remoteDesktop_;
  PipeWirePixelBuffer* pipewire_;

private:
  int rfbport_;
  GMainLoop* loop_;
};

#endif // __PORTAL_DESKTOP_H__
