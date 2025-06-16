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

#ifndef __WAYLAND_SEAT_H__
#define __WAYLAND_SEAT_H__

#include "Object.h"

struct wl_seat;
struct wl_seat_listener;

namespace wayland {
  class Display;
  class Seat : public Object {
  public:
    Seat(Display* display);
    ~Seat();

    wl_seat* getSeat() const { return seat; }

  private:
    void seatCapabilities(uint32_t capabilities);

  private:
    wl_seat* seat;
    Display* display;
    static const wl_seat_listener listener;
  };
};

#endif // __WAYLAND_SEAT_H__
