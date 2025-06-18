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

#ifndef __WAYLAND_VIRTUAL_KEYBOARD_H__
#define __WAYLAND_VIRTUAL_KEYBOARD_H__

#include <stdint.h>

#include "Object.h"

struct zwp_virtual_keyboard_v1;
struct zwp_virtual_keyboard_manager_v1;

namespace wayland {
  class Display;
  class Seat;

  class VirtualKeyboard : public Object {
  public:
    VirtualKeyboard(Display* display, Seat* seat);
    ~VirtualKeyboard();

    void key(uint32_t keysym, uint32_t keycode, bool down);

  private:
    zwp_virtual_keyboard_manager_v1* manager;
    zwp_virtual_keyboard_v1* keyboard;
    Seat* seat;
  };
};

#endif // __WAYLAND_VIRTUAL_KEYBOARD_H__
