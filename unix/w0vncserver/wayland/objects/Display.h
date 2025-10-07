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

#ifndef __WAYLAND_DISPLAY_H__
#define __WAYLAND_DISPLAY_H__

#include <stdint.h>

#include <string>
#include <map>

struct wl_display;
struct wl_registry;
struct wl_registry_listener;

namespace wayland {
  struct ObjectInfo {
    uint32_t name;
    uint32_t version;
  };

  class Display {
  public:
    Display(const char* name = nullptr);
    ~Display();

    bool interfaceAvailable(const char* interface);
    void roundtrip();

    wl_display* getDisplay() const { return display; }
    wl_registry* getRegistry() const { return registry; }
    ObjectInfo* getObjectInfo(const char* interface);

  private:
    void handleGlobalRegistry(uint32_t name, const char *interface,
                              uint32_t version);
    void handleGlobalRemove(uint32_t name);

  private:
    wl_display* display;
    wl_registry* registry;
    static const struct wl_registry_listener listener;
    std::map<std::string, ObjectInfo> objects;
  };
};

#endif // __WAYLAND_DISPLAY_H__
