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

#include <gio/gio.h>

#include <rfb/ScreenSet.h>
#include <rfb/VNCServerST.h>
#include <core/LogWriter.h>
#include <core/xdgdirs.h>

#include "../w0vncserver.h"
#include "../parameters.h"
#include "RemoteDesktop.h"
#include "PortalProxy.h"
#include "../pipewire/PipeWirePixelBuffer.h"
#include "PortalDesktop.h"

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;

static core::LogWriter vlog("PortalDesktop");

PortalDesktop::PortalDesktop()
  : server(nullptr), remoteDesktop(nullptr), pb(nullptr),
    restoreToken("")
{
}

PortalDesktop::~PortalDesktop()
{
  delete remoteDesktop;
}

void PortalDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void PortalDesktop::start()
{
  assert(!remoteDesktop);

  std::function<void(int, uint32_t)> startPipewire = [this](int fd, uint32_t id) {
    try {
      pb = new PipeWirePixelBuffer(fd, id, server);
    } catch (std::exception& e) {
      fatal_error("error initializing PipeWirePixelBuffer: %s", e.what());
    }
  };

  // FIXME: If the session startup is canceled (e.g. the local user
  // denies the connection), we need to close the connections to all
  // clients. This is not the cleanest solution, but it will do for now.
  std::function<void(const char*)> cancelStart = [this](const char* reason) {
    server->closeClients(reason);
  };

  remoteDesktop = new RemoteDesktop(restoreToken, startPipewire, cancelStart);
  remoteDesktop->createSession();
}

void PortalDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete pb;
  pb = nullptr;

  restoreToken = remoteDesktop->getRestoreToken();
  delete remoteDesktop;
  remoteDesktop = nullptr;
}

void PortalDesktop::queryConnection(network::Socket* sock,
                                    const char* /* userName */)
{
  // FIXME: Implement this.
  server->approveConnection(sock, false,
                            "Unable to query the local user to accept the connection.");
}

void PortalDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

unsigned int PortalDesktop::setScreenLayout(int /* fb_width */,
                                            int /* fb_height */,
                                            const rfb::ScreenSet& /*  layout */)
{
  return rfb::resultProhibited;
}

void PortalDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  // FIXME: The RemoteDesktop API does currently not specify this, but
  //        the keyboard methods expect evdev keycodes and
  //        xkbcommon keysyms.
  //        Using the Portals for input is not recommended. We should
  //        switch over to using libei entirely instead.
  if (rawKeyboard) {
    int evdevKeycode;

    if (keycode >= code_map_qnum_to_xorgevdev_len) {
      vlog.error("Could not map keycode %d to evdev key code", keycode);
      return;
    }

    evdevKeycode = code_map_qnum_to_xorgevdev[keycode];
    remoteDesktop->notifyKeyboardKeycode(evdevKeycode, down);
  } else {
    remoteDesktop->notifyKeyboardKeysym(keysym, down);
  }
}

void PortalDesktop::pointerEvent(const core::Point& pos,
                            uint16_t buttonMask)
{
  remoteDesktop->notifyPointerMotionAbsolute(pos.x, pos.y, buttonMask);
}

bool PortalDesktop::available()
{
  std::vector<std::string> interfaces = {
    "org.freedesktop.portal.RemoteDesktop",
    "org.freedesktop.portal.ScreenCast",
  };

  return PortalProxy::interfacesAvailable(interfaces);
}
