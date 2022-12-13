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

#ifndef __KEYTRANSLATEMACOS_H__
#define __KEYTRANSLATEMACOS_H__

#include "Keyboard.h"

#ifdef __OBJC__
@class NSEvent;
#else
class NSEvent;
#endif

class KeyboardMacOS : public Keyboard {
public:
  KeyboardMacOS(KeyboardHandler* handler);
  virtual ~KeyboardMacOS();

  virtual bool handleEvent(const void* event);
  virtual rdr::U32 translateToKeyCode(int systemKeyCode);
  virtual std::list<rdr::U32> translateToKeySyms(int systemKeyCode);

  virtual unsigned getLEDState();
  virtual void setLEDState(unsigned state);

  // Special helper on macOS
  static bool isKeyboardSync(const void* event);

protected:
  bool isKeyboardEvent(const NSEvent* nsevent);
  bool isKeyPress(const NSEvent* nsevent);
  unsigned getSystemKeyCode(const NSEvent* nsevent);
  
  rdr::U32 translateToKeySym(unsigned keyCode, unsigned modifierFlags);

  int openHID(unsigned int *ioc);
  int getModifierLockState(int modifier, bool *on);
  int setModifierLockState(int modifier, bool on);
};

#endif
