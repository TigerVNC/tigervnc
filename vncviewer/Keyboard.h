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

#ifndef __KEYTRANSLATE_H__
#define __KEYTRANSLATE_H__

#include <list>

#include <rdr/types.h>

class KeyboardHandler {
public:
  virtual void handleKeyPress(int systemKeyCode,
                              rdr::U32 keyCode, rdr::U32 keySym) = 0;
  virtual void handleKeyRelease(int systemKeyCode) = 0;
};

class Keyboard {
public:
  Keyboard(KeyboardHandler* handler) : handler(handler) {};
  virtual ~Keyboard() {};

  virtual bool handleEvent(const void* event) = 0;
  virtual rdr::U32 translateToKeyCode(int systemKeyCode) = 0;
  virtual std::list<rdr::U32> translateToKeySyms(int systemKeyCode) = 0;

  virtual void reset() {};

  virtual unsigned getLEDState() = 0;
  virtual void setLEDState(unsigned state) = 0;

protected:
  KeyboardHandler* handler;
};

#endif
