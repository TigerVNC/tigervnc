/* Copyright 2021-2025 Pierre Ossman for Cendio AB
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

#ifndef __HOTKEYHANDLER__
#define __HOTKEYHANDLER__

#include <set>
#include <map>

#include <stdint.h>

class HotKeyHandler {
public:
  HotKeyHandler();

  void setHotKeyCombo(unsigned mask);

  enum KeyAction {
    KeyNormal,
    KeyUnarm,
    KeyHotKey,
    KeyIgnore,
  };

  KeyAction handleKeyPress(int keyCode, uint32_t keySym);
  KeyAction handleKeyRelease(int keyCode);

  void reset();

public:
  enum ComboMask {
    Control = (1<<0),
    Shift =   (1<<1),
    Alt =     (1<<2),
    Super =   (1<<3),
  };

  static unsigned parseHotKey(const char* key);
  static const char* hotKeyString(unsigned key);

  static const char* comboPrefix(unsigned mask, bool justCombo=false);

private:
  unsigned keySymToMask(uint32_t keySym);

private:
  unsigned comboMask;

  enum State {
    Idle,
    Arming,
    Armed,
    Rearming,
    Firing,
    Wedged,
  };
  State state;

  std::set<int> firedKeys;
  std::map<int, uint32_t> pressedKeys;
};

#endif
