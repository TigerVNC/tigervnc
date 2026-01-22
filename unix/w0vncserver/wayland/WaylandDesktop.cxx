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
#include <unistd.h>
#include <sys/mman.h>

#include <glib.h>
#include <wayland-client.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>

#include "../w0vncserver.h"
#include "../parameters.h"
#include "objects/Display.h"
#include "objects/DataControl.h"
#include "objects/Output.h"
#include "objects/Seat.h"
#include "objects/VirtualPointer.h"
#include "objects/VirtualKeyboard.h"
#include "GWaylandSource.h"
#include "WaylandPixelBuffer.h"
#include "WaylandDesktop.h"

static core::LogWriter vlog("WaylandDesktop");

#define BUTTONS 9

WaylandDesktop::WaylandDesktop(GMainLoop* loop_)
  : server(nullptr), pb(nullptr), loop(loop_), waylandSource(nullptr),
    display(nullptr), seat(nullptr), virtualPointer(nullptr),
    virtualKeyboard(nullptr), dataControl(nullptr)
{
  assert(available());

  display = new wayland::Display();
  output = new wayland::Output(display);
  seat = new wayland::Seat(display, std::bind(&WaylandDesktop::setLEDState,
                                              this, std::placeholders::_1));
}

WaylandDesktop::~WaylandDesktop()
{
  delete pb;
  delete waylandSource;
  delete virtualPointer;
  delete virtualKeyboard;
  delete seat;
  delete output;
  delete display;
}

void WaylandDesktop::init(rfb::VNCServer* vs)
{
  server = vs;
}

void WaylandDesktop::start()
{
  std::function<void()> desktopReadyCb = [this]() {
    try {
      virtualPointer = new wayland::VirtualPointer(display, seat);
    } catch (std::exception& e) {
      vlog.error("%s - pointer will be disabled", e.what());
    }
    try {
      virtualKeyboard = new wayland::VirtualKeyboard(display, seat);
    } catch (std::exception& e) {
      vlog.error("%s - keyboard will be disabled", e.what());
    }

    if (display->interfaceAvailable("ext_data_control_manager_v1")) {
      std::function<void(bool available)> clipboardAnnounceCb = [this](bool available) {
        server->announceClipboard(available);
      };
      std::function<void(const char* data)> sendClipboardData = [this](const char* data) {
        server->sendClipboardData(data);
      };
      std::function<void()> clipboardRequestCb = [this]() {
        server->requestClipboard();
      };

      dataControl = new wayland::DataControl(display, seat,
                                             clipboardAnnounceCb,
                                             clipboardRequestCb,
                                             sendClipboardData);
    } else {
      vlog.info("ext-data-control-v1 not available, Clipboard disabled");
    }

    server->setPixelBuffer(pb);
    server->setLEDState(virtualKeyboard->getLEDState());
  };

  try {
    pb = new WaylandPixelBuffer(display, output, server, desktopReadyCb);
  } catch (std::exception& e) {
    vlog.error("Error initializing pixel buffer: %s", e.what());
    server->closeClients("Failed to start remote desktop session");
  }

  waylandSource = new GWaylandSource(display);
  waylandSource->attach(g_main_loop_get_context(loop));
}

void WaylandDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete virtualKeyboard;
  virtualKeyboard = nullptr;

  delete waylandSource;
  waylandSource = nullptr;

  delete virtualPointer;
  virtualPointer = nullptr;

  delete pb;
  pb = nullptr;

  delete dataControl;
  dataControl = nullptr;
}

void WaylandDesktop::pointerEvent(const core::Point& pos, uint16_t buttonMask)
{
  if (!virtualPointer)
    return;

  virtualPointer->motionAbsolute(pos.x, pos.y, pb->width(), pb->height());

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        virtualPointer->axisDiscrete(i);
      else
        virtualPointer->button(i, buttonMask & (1 << i));
    }
  }

  oldButtonMask = buttonMask;
}

void WaylandDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  if (!virtualKeyboard)
    return;

  virtualKeyboard->key(keysym, keycode, down);
}

void WaylandDesktop::queryConnection(network::Socket* sock,
                                     const char* /* userName */)
{
  // FIXME: Implement this.
  server->approveConnection(sock, false,
                            "Unable to query the local user to accept the connection.");
}

void WaylandDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

void WaylandDesktop::handleClipboardRequest()
{
  if (!dataControl)
    return;

  dataControl->receive();
}

void WaylandDesktop::handleClipboardAnnounce(bool available)
{
  if (!dataControl)
    return;

  if (available) {
    dataControl->setSelection();
    if (setPrimary)
      dataControl->setPrimarySelection();
  } else {
    dataControl->clearSelection();
    if (setPrimary)
      dataControl->clearPrimarySelection();
  }
}

void WaylandDesktop::handleClipboardData(const char* data)
{
  if (!dataControl)
    return;

  dataControl->writePending(data);
}

bool WaylandDesktop::available()
{
  wayland::Display display;

  // We need either wlr-screencopy OR ext-image-copy-capture
  return (display.interfaceAvailable("zwlr_screencopy_manager_v1") ||
         (
            display.interfaceAvailable("ext_image_copy_capture_manager_v1") &&
            display.interfaceAvailable("ext_output_image_capture_source_manager_v1")
         )) &&
         display.interfaceAvailable("zwlr_virtual_pointer_manager_v1") &&
         display.interfaceAvailable("zwp_virtual_keyboard_manager_v1");
}

void WaylandDesktop::setLEDState(unsigned int state)
{
  if (server)
    server->setLEDState(state);
}
