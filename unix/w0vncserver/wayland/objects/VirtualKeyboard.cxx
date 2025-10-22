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

#include <time.h>

#include <virtual-keyboard-unstable-v1.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <core/LogWriter.h>
#include <rfb/KeysymStr.h>

#include "../../w0vncserver.h"
#include "../../parameters.h"
#include "Display.h"
#include "Seat.h"
#include "Keyboard.h"
#include "VirtualKeyboard.h"

using namespace wayland;

static core::LogWriter vlog("WaylandVirtualKeyboard");

VirtualKeyboard::VirtualKeyboard(Display* display, Seat* seat_)
  : Object(display, "zwp_virtual_keyboard_manager_v1",
           &zwp_virtual_keyboard_manager_v1_interface),
    manager(nullptr), keyboard(nullptr), seat(seat_)
{
  if (!seat->getKeyboard()->getSize()) {
    fatal_error("Keyboard keymap is not set");
    return;
  }

  manager = (zwp_virtual_keyboard_manager_v1*) boundObject;

  keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(manager,
                                                                     seat->getSeat());
  if (!keyboard) {
    fatal_error("Failed to create virtual keyboard");
    return;
  }

  zwp_virtual_keyboard_v1_keymap(keyboard,
                                 seat->getKeyboard()->getFormat(),
                                 seat->getKeyboard()->getFd(),
                                 seat->getKeyboard()->getSize());
}

VirtualKeyboard::~VirtualKeyboard()
{
  if (manager)
    zwp_virtual_keyboard_manager_v1_destroy(manager);
  if (keyboard)
    zwp_virtual_keyboard_v1_destroy(keyboard);
}

unsigned int VirtualKeyboard::getLEDState()
{
  return seat->getKeyboard()->getLEDState();
}

void VirtualKeyboard::key(uint32_t keysym, uint32_t keycode, bool down)
{
  timespec ts;
  uint32_t time;
  bool updated;
  int key;
  Keyboard* wKeyboard;
  uint32_t modsDepressed;
  uint32_t modsLatched;
  uint32_t modsLocked;
  uint32_t group;

  wKeyboard = seat->getKeyboard();

  if (!rawKeyboard) {
    key = wKeyboard->keysymToKeycode(keysym);
    if ((unsigned int)key == XKB_KEYCODE_INVALID) {
      vlog.debug("Unable to map keysym XK_%s (0x%04x), ignoring key press",
                 KeySymName(keysym), keysym);
      return;
    }
  } else {
    key = wKeyboard->rfbcodeToKeycode(keycode);
  }

  clock_gettime(CLOCK_MONOTONIC, &ts);
  time = (static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000) &
         0xFFFFFFFF;

  // The Wayland documentation states that:
  //   "clients must add 8 to the key event keycode"
  // when format xkb_v1 is used. Subtract 8 to get the correct keycode.
  if (wKeyboard->getFormat() == XKB_KEYMAP_FORMAT_TEXT_V1)
    zwp_virtual_keyboard_v1_key(keyboard, time, key - 8, down);
  else
    zwp_virtual_keyboard_v1_key(keyboard, time, key, down);

  updated = wKeyboard->updateState(key, down, &modsDepressed,
                                   &modsLatched, &modsLocked, &group);
  if (updated) {
    zwp_virtual_keyboard_v1_modifiers(keyboard, modsDepressed,
                                      modsLatched, modsLocked, group);
  }
}
