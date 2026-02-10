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

#include <core/LogWriter.h>
#include <wayland-client-protocol.h>

#include "Display.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "Seat.h"

using namespace wayland;

static core::LogWriter vlog("WSeat");

const wl_seat_listener Seat::listener = {
  .capabilities = [](void* data, wl_seat*,
                     uint32_t capabilities) {
    ((Seat*)data)->seatCapabilities(capabilities);
  },
  .name = [](void*, wl_seat*, const char*) {},
};

Seat::Seat(Display* display_, std::function<void(unsigned int)> setLEDstate_)
  : Object(display_, "wl_seat", &wl_seat_interface),
    seat(nullptr), display(display_), keyboard(nullptr),
    pointer(nullptr), setLEDstate(setLEDstate_)
{
  seat = (wl_seat*)boundObject;

  wl_seat_add_listener(seat, &listener, this);
  display->roundtrip();
}

Seat::~Seat()
{
  if (seat)
    wl_seat_destroy(seat);

  delete keyboard;
  delete pointer;
}

void Seat::seatCapabilities(uint32_t capabilities)
{
  if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
    vlog.debug("Keyboard detected");
    delete keyboard;
    keyboard = new Keyboard(display, this, setLEDstate);
  }

  if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
      delete pointer;
      pointer = new Pointer(display, this);
  }
}
