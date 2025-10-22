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

#ifndef __WAYLAND_KEYBOARD_H__
#define __WAYLAND_KEYBOARD_H__

#include <functional>

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

struct XkbContext;

namespace wayland {
  class Display;
  class Seat;

  class Keyboard {
  public:
    Keyboard(Display* display, Seat *seat, std::function<void(unsigned int)> setLEDstate);
    ~Keyboard();

    uint32_t getFormat() const { return keyboardFormat; }
    int getFd() const { return keyboardFd; }
    int getSize() const { return keyboardSize; }
    // Get the current LED state
    unsigned int getLEDState();

    // Update the keyboard state according to a key event.
    // Returns true if the modifier state changed as well as the current
    // state
    bool updateState(uint32_t keycode, bool down, uint32_t* modsDepressed,
                     uint32_t* modsLatched, uint32_t* modsLocked,
                     uint32_t* group);

    uint32_t keysymToKeycode(int keycode);
    int rfbcodeToKeycode(int rfbcode);

  private:
    void handleKeyMap(uint32_t format, int32_t fd, uint32_t size);
    void handleModifiers(uint32_t serial, uint32_t modsDepressed,
                         uint32_t modsLatched, uint32_t modsLocked,
                         uint32_t group);
    void generateKeycodeMap();

  private:
    uint32_t keyboardFormat;
    int keyboardFd;
    int keyboardSize;
    wl_keyboard* keyboard;
    char* keyMap;
    static const wl_keyboard_listener listener;
    XkbContext* context;
    // Called when the LED state has changed
    std::function<void(unsigned int)> setLEDstate;
    int code_map_qnum_to_keycode[256];
  };
};

#endif // __WAYLAND_KEYBOARD_H__
