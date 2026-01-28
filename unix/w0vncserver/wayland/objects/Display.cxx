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
#include <string.h>

#include <stdexcept>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include <core/string.h>
#include <core/LogWriter.h>

#include "../../w0vncserver.h"
#include "Object.h"
#include "Display.h"

using namespace wayland;

static core::LogWriter vlog("WDisplay");

const wl_registry_listener Display::listener = {
  .global = [](void* data, wl_registry*, uint32_t name,
               const char* interface, uint32_t version) {
    ((Display*)data)->handleGlobalRegistry(name, interface, version);
  },
  .global_remove = [](void* data, wl_registry*, uint32_t name) {
    ((Display*)data)->handleGlobalRemove(name);
  }
};

Display::Display(const char* name)
  : display(nullptr), registry(nullptr)
{
  display = wl_display_connect(name);
  if (!display)
    throw std::runtime_error("Failed to connect to wayland display");

  registry = wl_display_get_registry(display);
  if (!registry) {
    wl_display_disconnect(display);
    throw std::runtime_error("Failed to get registry");
  }

  wl_registry_add_listener(registry, &listener, this);
  wl_display_roundtrip(display);
}

Display::~Display()
{
  wl_display_flush(display);
  wl_registry_destroy(registry);
  wl_display_disconnect(display);
}

bool Display::interfaceAvailable(const char* interface)
{
  return objects.find(interface) != objects.end();
}

void Display::roundtrip()
{
  // Display errors are fatal, the display can no longer be used
  if (wl_display_roundtrip(display) < 0) {
    if (wl_display_get_error(display))
      fatal_error("Failed to roundtrip: %s",
                  strerror(wl_display_get_error(display)));
  }
}

ObjectInfo* Display::getObjectInfo(const char* interface)
{
  return &objects[interface];
}

void Display::handleGlobalRegistry(uint32_t name,
                                    const char* interface,
                                    uint32_t version)
{
  objects[interface] = { name, version };
}

void Display::handleGlobalRemove(uint32_t name)
{
  vlog.debug("Removing global: %d", name);

  for (auto it = objects.begin(); it != objects.end(); ++it) {
    if (it->second.name == name) {
      objects.erase(it);
      break;
    }
  }
}
