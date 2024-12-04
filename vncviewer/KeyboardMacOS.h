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

#ifndef __KEYBOARDMACOS_H__
#define __KEYBOARDMACOS_H__

#include "Keyboard.h"

#ifdef __OBJC__
@class NSEvent;
@class NSString;
#else
class NSEvent;
class NSString;
#endif

class KeyboardMacOS : public Keyboard
{
public:
  KeyboardMacOS(KeyboardHandler* handler);
  virtual ~KeyboardMacOS();

  bool handleEvent(const void* event) override;

  unsigned getLEDState() override;
  void setLEDState(unsigned state) override;

  // Special helper on macOS
  static bool isKeyboardSync(const void* event);

protected:
  bool isKeyboardEvent(const NSEvent* nsevent);
  bool isKeyPress(const NSEvent* nsevent);
  uint32_t translateSystemKeyCode(int systemKeyCode);
  unsigned getSystemKeyCode(const NSEvent* nsevent);

  NSString* keyTranslate(unsigned keyCode, unsigned modifierFlags);
  uint32_t translateEventKeysym(const NSEvent* nsevent);

  int openHID(unsigned int* ioc);
  int getModifierLockState(int modifier, bool* on);
  int setModifierLockState(int modifier, bool on);
};

#endif
