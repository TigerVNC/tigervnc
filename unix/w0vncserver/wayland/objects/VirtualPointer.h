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

#ifndef __WAYLAND_VIRTUAL_POINTER_H__
#define __WAYLAND_VIRTUAL_POINTER_H__

#include "Object.h"

namespace core { struct Point; }

struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;

namespace wayland {
  class Display;
  class Seat;

  class VirtualPointer : public Object {
  public:
    VirtualPointer(Display* display, Seat* seat);
    ~VirtualPointer();

    void motionAbsolute(uint32_t x, uint32_t y, uint32_t xExetent, uint32_t yExtent);
    void button(uint16_t button, bool down);
    void axisDiscrete(int16_t button);

  private:
    void frame();

  private:
    zwlr_virtual_pointer_manager_v1* manager;
    zwlr_virtual_pointer_v1* cursor;
  };
};

#endif // __WAYLAND_VIRTUAL_POINTER_H__
