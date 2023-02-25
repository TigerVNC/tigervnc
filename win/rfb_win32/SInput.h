/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

// -=- Input.h
//
// A number of routines that accept VNC-style input event data and perform
// the appropriate actions under Win32

#ifndef __RFB_WIN32_INPUT_H__
#define __RFB_WIN32_INPUT_H__

#include <rfb/Rect.h>
#include <rfb/Configuration.h>

#include <map>
#include <vector>

#include <stdint.h>

namespace rfb {

  namespace win32 {

    // -=- Pointer event handling

    class SPointer {
    public:
      SPointer();
      // - Create a pointer event at a the given coordinates, with the
      //   specified button state.  The event must be specified using
      //   Screen coordinates.
      void pointerEvent(const Point& pos, uint8_t buttonmask);
    protected:
      Point last_position;
      uint8_t last_buttonmask;
    };

    // -=- Keyboard event handling

    class SKeyboard {
    public:
      SKeyboard();
      void keyEvent(uint32_t keysym, uint32_t keycode, bool down);
      static BoolParameter deadKeyAware;
      static BoolParameter rawKeyboard;
    private:
      std::map<uint32_t,uint8_t> vkMap;
      std::map<uint32_t,bool> extendedMap;
      std::vector<uint8_t> deadChars;
    };

  }; // win32

}; // rfb

#endif // __RFB_WIN32_INPUT_H__
