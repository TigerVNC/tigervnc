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

  bool filterKeyPress(int keyCode, rdr::U32 keySym);
  bool filterKeyRelease(int keyCode);

protected:
  virtual void sendPointerEvent(const rfb::Point& pos, int buttonMask)=0;

  virtual bool handleTimeout(rfb::Timer *t);

  virtual void writeKeyEvent(rdr::U32 keySym, rdr::U32 keyCode, bool down)=0;

private:
  typedef const signed char Action[3];
  static Action mbStateTab[8][5];
  void sendAction(const rfb::Point& pos, int buttonMask, int action);

  int createButtonMask(int buttonMask);

  void mbFsmDoMod(int modAction);
  void mbFsmDoButton(const rfb::Point& pos, int buttonMask, int buttonAction);
  bool filterKeyPressRelease(bool is_press, int keyCode, rdr::U32 keySym);
  void mbFSM(Action action);
  void filterPointerEventMod(const rfb::Point& pos, int buttonMask);
  void filterPointerEventLR(const rfb::Point& pos, int buttonMask);

private:
  int state;
  int emulatedButtonMask;
  int lastButtonMask;
  rfb::Point lastPos, origPos;
  rfb::Timer timer;

  // Variables for mod-button1 emulation
  rdr::U32 emulateMiddleButtonModifierKey;	// The key sym used for emulation
  int mbState;
  int modKeyCode;				// Save the modifier keycode
  int mbLastButtonMask;
};

#endif
