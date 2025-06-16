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
#include <time.h>
#include <linux/input-event-codes.h>

#include <stdexcept>

#include <wlr-virtual-pointer-unstable-v1.h>

#include <core/Rect.h>

#include "Seat.h"
#include "Display.h"
#include "VirtualPointer.h"

using namespace wayland;

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

static int getInputCode(uint32_t button)
{
  switch (button) {
  case 0x0:
    return BTN_LEFT;
  case 0x1:
    return BTN_MIDDLE;
  case 0x2:
    return BTN_RIGHT;
  case 0x7:
    return BTN_SIDE;
  case 0x8:
    return BTN_EXTRA;
  default:
    throw std::runtime_error("Unknown mouse button");
  }
}

VirtualPointer::VirtualPointer(Display* display, Seat* seat)
: Object(display, "zwlr_virtual_pointer_manager_v1",
         &zwlr_virtual_pointer_manager_v1_interface)
{
  manager = (zwlr_virtual_pointer_manager_v1*) boundObject;

  cursor = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(manager,
                                                                  seat->getSeat());
  if (!cursor)
    throw std::runtime_error("Failed to create virtual pointer");

  zwlr_virtual_pointer_v1_axis_source(cursor, WL_POINTER_AXIS_SOURCE_WHEEL);
}

VirtualPointer::~VirtualPointer()
{
  if (manager)
    zwlr_virtual_pointer_manager_v1_destroy(manager);
  if (cursor)
    zwlr_virtual_pointer_v1_destroy(cursor);
}

void VirtualPointer::motionAbsolute(uint32_t x, uint32_t y,
                                    uint32_t xExetent, uint32_t yExtent)
{
  timespec ts;
  uint32_t time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  // Avoid overflow
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000)
          & 0xFFFFFFFF;

  zwlr_virtual_pointer_v1_motion_absolute(cursor, time, x, y, xExetent, yExtent);
  frame();
}

void VirtualPointer::button(uint16_t button, bool down)
{
  timespec ts;
  uint32_t time;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  // Avoid overflow
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000)
          & 0xFFFFFFFF;

  zwlr_virtual_pointer_v1_button(cursor, time, getInputCode(button),
                                 down);
  frame();
}

void VirtualPointer::axisDiscrete(int16_t button)
{
  int32_t axis;
  int value;
  int discrete;
  timespec ts;
  uint32_t time;

  assert(button > 2 && button < 7);

  clock_gettime(CLOCK_MONOTONIC, &ts);
  // Avoid overflow
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000)
          & 0xFFFFFFFF;

  // FIXME: Not sure what this is supposed to be.
  value = 1;

  switch (button) {
  case WHEEL_VERTICAL_DOWN:
    axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
    discrete = -1;
    break;
    case WHEEL_VERTICAL_UP:
    axis = WL_POINTER_AXIS_VERTICAL_SCROLL;
    discrete = 1;
    break;
  case WHEEL_HORIZONTAL_LEFT:
    axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
    discrete = -1;
    break;
  case WHEEL_HORIZONTAL_RIGHT:
    axis = WL_POINTER_AXIS_HORIZONTAL_SCROLL;
    discrete = -1;
    break;
  default:
    assert(false);
  }

  zwlr_virtual_pointer_v1_axis_discrete(cursor, time, axis, value,
                                        discrete);
}

void VirtualPointer::frame()
{
  zwlr_virtual_pointer_v1_frame(cursor);
}
