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

#ifndef __KEYTRANSLATEWIN32_H__
#define __KEYTRANSLATEWIN32_H__

#include "Keyboard.h"

class KeyboardWin32 : public Keyboard {
public:
  KeyboardWin32(KeyboardHandler* handler);
  virtual ~KeyboardWin32();

  virtual bool handleEvent(const void* event);
  virtual rdr::U32 translateToKeyCode(int systemKeyCode);
  virtual std::list<rdr::U32> translateToKeySyms(int systemKeyCode);

  virtual void reset();

  virtual unsigned getLEDState();
  virtual void setLEDState(unsigned state);

protected:
  rdr::U32 lookupVKeyMap(unsigned vkey, bool extended,
                         const unsigned map[][3], size_t size);
  rdr::U32 translateVKey(unsigned vkey, bool extended,
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
