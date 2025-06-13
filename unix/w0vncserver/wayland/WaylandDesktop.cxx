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
#include "objects/Display.h"
#include "objects/Output.h"
#include "GWaylandSource.h"
#include "WaylandPixelBuffer.h"
#include "WaylandDesktop.h"

static core::LogWriter vlog("WaylandDesktop");

WaylandDesktop::WaylandDesktop(GMainLoop* loop_)
  : server(nullptr), pb(nullptr), loop(loop_), waylandSource(nullptr),
    display(nullptr), seat(nullptr)
{
  assert(available());

  display = new wayland::Display();
  output = new wayland::Output(display);
}

WaylandDesktop::~WaylandDesktop()
{
  delete pb;
  delete waylandSource;
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
    server->setPixelBuffer(pb);
  };

  pb = new WaylandPixelBuffer(display, output, server, desktopReadyCb);

  waylandSource = new GWaylandSource(display);
  waylandSource->attach(g_main_loop_get_context(loop));
}

void WaylandDesktop::stop()
{
  server->setPixelBuffer(nullptr);

  delete waylandSource;
  waylandSource = nullptr;

  delete pb;
  pb = nullptr;
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

bool WaylandDesktop::available()
{
  wayland::Display display;

  return display.interfaceAvailable("zwlr_screencopy_manager_v1");
}
