/* Copyright 2011-2021 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __KEYBOARDX11_H__
#define __KEYBOARDX11_H__

#include "Keyboard.h"

class KeyboardX11 : public Keyboard
{
public:
  KeyboardX11(KeyboardHandler* handler);
  virtual ~KeyboardX11();

  bool isKeyboardReset(const void* event) override;

  bool handleEvent(const void* event) override;
  std::list<uint32_t> translateToKeySyms(int systemKeyCode) override;

  unsigned getLEDState() override;
  void setLEDState(unsigned state) override;

protected:
  unsigned getModifierMask(uint32_t keysym);

private:
  void translateToKeySyms(int systemKeyCode, unsigned char group,
                          std::list<uint32_t>* keySyms);
  void translateToKeySyms(int systemKeyCode,
                          unsigned char group, unsigned char mods,
                          std::list<uint32_t>* keySyms);

private:
  int code_map_keycode_to_qnum[256];
};

#endif
