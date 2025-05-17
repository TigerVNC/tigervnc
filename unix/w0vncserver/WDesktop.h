
#ifndef __WDESKTOP_H__
#define __WDESKTOP_H__

#include <rfb/SDesktop.h>
#include <rfb/VNCServer.h>
#include <core/Timer.h>
#include "Pipewire.h"
#include "WPixelBuffer.h"
#include "WaylandBackend.h"

class RemoteDesktop;

class WDesktop : public rfb::SDesktop
{
public:
  WDesktop(WaylandBackend* backend);
  virtual ~WDesktop();

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

  // -=- Methods used by Pipewire
  WPixelBuffer* pixelBuffer() const { return pb_; }
  void setPixelBuffer(WPixelBuffer* pb) { pb_ = pb; }
  void add_changed(const core::Region& region);
  void setCursor(int width, int height, int hotX, int hotY,
                 const unsigned char *rgbaData);
  void setCursorPos(int x, int y);;

protected:
  rfb::VNCServer* server;
  WPixelBuffer* pb_;

private:
  WaylandBackend* backend_;
};

#endif