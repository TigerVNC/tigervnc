/* Copyright 2026 Adam Halim for Cendio AB
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

#ifndef __WAYLAND_POINTER_H__
#define __WAYLAND_POINTER_H__

struct wl_pointer;

namespace wayland {
  class Display;
  class Seat;

  class Pointer  {
  public:
    Pointer(Display* display, Seat* seat);
    ~Pointer();

    wl_pointer* getPointer() const { return pointer; }

  private:
    wl_pointer* pointer;
  };
}

#endif // __WAYLAND_POINTER_H__
