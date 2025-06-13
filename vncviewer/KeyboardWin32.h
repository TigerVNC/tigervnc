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

#ifndef __KEYBOARDWIN32_H__
#define __KEYBOARDWIN32_H__

#include "Keyboard.h"

class KeyboardWin32 : public Keyboard
{
public:
  KeyboardWin32(KeyboardHandler* handler);
  virtual ~KeyboardWin32();

  bool handleEvent(const void* event) override;
  std::list<uint32_t> translateToKeySyms(int systemKeyCode) override;

  void reset() override;

  unsigned getLEDState() override;
  void setLEDState(unsigned state) override;

protected:
  uint32_t translateSystemKeyCode(int systemKeyCode);
  uint32_t lookupVKeyMap(unsigned vkey, bool extended,
                         const UINT map[][3], size_t size);
  uint32_t translateVKey(unsigned vkey, bool extended,
                         const unsigned char state[256]);

  bool hasAltGr();
  static void handleAltGrTimeout(void *data);
  void resolveAltGrDetection(bool isAltGrSequence);

private:
  int cachedHasAltGr;
  HKL currentLayout;

  bool altGrArmed;
  unsigned int altGrCtrlTime;

  bool leftShiftDown;
  bool rightShiftDown;
};

#endif
