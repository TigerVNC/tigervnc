#include "WDesktop.h"
#include "Pipewire.h"
#include <rfb/ScreenSet.h>
#include "WaylandBackend.h"
#include <rfb/VNCServerST.h>
#include <assert.h>
#include <core/LogWriter.h>

static core::LogWriter vlog("WDesktop");

WDesktop::WDesktop(WaylandBackend* backend)
  : pb_(nullptr), backend_(backend)
{
}

WDesktop::~WDesktop()
{
  delete pb_;
}

void WDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void WDesktop::start()
{
  assert(pb_);
  server->setPixelBuffer(pb_);
}

void WDesktop::add_changed(const core::Region& region)
{
  server->add_changed(region);
}

void WDesktop::queryConnection(network::Socket* sock,
                               const char* userName)
{
  (void)sock;
  (void)userName;
  // FIXME: Implement this.
}

void WDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

unsigned int WDesktop::setScreenLayout(int fb_width, int fb_height,
                                       const rfb::ScreenSet& layout)
{
  (void) fb_width;
  (void) fb_height;
  (void) layout;
  vlog.debug("setScreenLayout");
  return -1;
}

void WDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  backend_->keyEvent(keysym, keycode, down);
}

void WDesktop::pointerEvent(const core::Point& pos,
                            uint16_t buttonMask)
{
  backend_->pointerEvent(pos, buttonMask);
}

void WDesktop::handleClipboardRequest()
{
}

void WDesktop::handleClipboardAnnounce(bool available)
{
  (void) available;
}

void WDesktop::handleClipboardData(const char* data)
{
  (void) data;
}

