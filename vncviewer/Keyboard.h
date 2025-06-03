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

#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

#include <list>

class KeyboardHandler
{
public:
  virtual void handleKeyPress(int systemKeyCode,
                              uint32_t keyCode, uint32_t keySym) = 0;
  virtual void handleKeyRelease(int systemKeyCode) = 0;
};

class Keyboard
{
public:
  Keyboard(KeyboardHandler* handler_) : handler(handler_) {};
  virtual ~Keyboard() {};

  virtual bool isKeyboardReset(const void* event) { (void)event; return false; }

  virtual bool handleEvent(const void* event) = 0;
  virtual std::list<uint32_t> translateToKeySyms(int systemKeyCode) = 0;

  virtual void reset() {};

  virtual unsigned getLEDState() = 0;
  virtual void setLEDState(unsigned state) = 0;

protected:
  KeyboardHandler* handler;
};

#endif
