/* Copyright 2020 Alex Tanskanen for Cendio AB
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

#ifndef __EMULATEMB__
#define __EMULATEMB__

#include <rfb/Timer.h>
#include <rfb/Rect.h>
#include <rdr/types.h>

class EmulateMB : public rfb::Timer::Callback {
public:
  EmulateMB(rdr::U32 emulateMBModKey);

  void filterPointerEvent(const rfb::Point& pos, int buttonMask);

  // Middle button emulation by pressing ALT+button1. (or actually the modifier configued in emulateMiddleButtonModifierKey)
  bool filterKeyPress(int keyCode, rdr::U32 keySym);
  bool filterKeyRelease(int keyCode);

protected:
  virtual void sendPointerEvent(const rfb::Point& pos, int buttonMask)=0;

  virtual bool handleTimeout(rfb::Timer *t);

  virtual void writeKeyEvent(rdr::U32 keySym, rdr::U32 keyCode, bool down)=0;

private:
  void sendAction(const rfb::Point& pos, int buttonMask, int action);

  int createButtonMask(int buttonMask);

private:
  int state;
  int emulatedButtonMask;
  int lastButtonMask;
  rfb::Point lastPos, origPos;
  rfb::Timer timer;

  // Variables for mod-button1 emulation
  rdr::U32 emulateMiddleButtonModifierKey;	// The key sym used for emulation
  bool modifierPressed;				// Is the modifier currently pressed
  bool modifierBuffered;			// Is a modifier pressed but not sent to the server
  bool sentMiddleClick;				// Did the user clicked button1 while modifier was pressed
  int modKeyCode;				// buffer the modifier keycode
};

#endif
