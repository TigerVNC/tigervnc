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

#ifndef __WAYLAND_OBJECT_H__
#define __WAYLAND_OBJECT_H__

#include <stdint.h>

struct wl_interface;
struct wl_proxy;

namespace wayland {
  class Display;

  class Object {
  protected:
    Object(Display* display, const char* name,
           const wl_interface* interface);
  public:
    virtual ~Object() {};
  protected:
    wl_proxy* boundObject;
  };
};

#endif // __WAYLAND_OBJECT_H__
