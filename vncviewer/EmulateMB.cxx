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

/*
 * Based on xf86-input-evdev
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 * Copyright 2002 by SuSE Linux AG, Author: Egbert Eich
 * Copyright 1994-2002 by The XFree86 Project, Inc.
 * Copyright 2002 by Paul Elliott
 * (Ported from xf86-input-mouse, above copyrights taken from there)
 * Copyright © 2008 University of South Australia
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the authors
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The authors make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include "parameters.h"
#include "i18n.h"
#include "EmulateMB.h"

static rfb::LogWriter vlog("EmulateMB");
/*
 * There are two different middle button emulation mechanism:
 *   1) Pressing the left + right buttons to emulate middle button (enabled by the setting emulateMiddleButton)
 *   2) Pressing a modifier + left button to emulate middle button (ebabled by the setting emulateMiddleButtonMod)
 */

/*
 * 1) Pressing the left + right buttons to emulate middle button:
 *
 * Lets create a simple finite-state machine for 3 button emulation:
 *
 * We track buttons 1 and 3 (left and right).  There are 11 states:
 *   0 ground           - initial state
 *   1 delayed left     - left pressed, waiting for right
 *   2 delayed right    - right pressed, waiting for left
 *   3 pressed middle   - right and left pressed, emulated middle sent
 *   4 pressed left     - left pressed and sent
 *   5 pressed right    - right pressed and sent
 *   6 released left    - left released after emulated middle
 *   7 released right   - right released after emulated middle
 *   8 repressed left   - left pressed after released left
 *   9 repressed right  - right pressed after released right
 *  10 pressed both     - both pressed, not emulating middle
 *
 * At each state, we need handlers for the following events
 *   0: no buttons down
 *   1: left button down
 *   2: right button down
 *   3: both buttons down
 *   4: emulate3Timeout passed without a button change
 * Note that button events are not deltas, they are the set of buttons being
 * pressed now.  It's possible (ie, mouse hardware does it) to go from (eg)
 * left down to right down without anything in between, so all cases must be
 * handled.
 *
 * a handler consists of three values:
 *   0: action1
 *   1: action2
 *   2: new emulation state
 *
 * action > 0: ButtonPress
 * action = 0: nothing
 * action < 0: ButtonRelease
 *
 * The comment preceeding each section is the current emulation state.
 * The comments to the right are of the form
 *      <button state> (<events>) -> <new emulation state>
 * which should be read as
 *      If the buttons are in <button state>, generate <events> then go to
 *      <new emulation state>.
 */

static const signed char stateTab[11][5][3] = {
/* 0 ground */
  {
    {  0,  0,  0 },   /* nothing -> ground (no change) */
    {  0,  0,  1 },   /* left -> delayed left */
    {  0,  0,  2 },   /* right -> delayed right */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  0,  0, -1 }    /* timeout N/A */
  },
/* 1 delayed left */
  {
    {  1, -1,  0 },   /* nothing (left event) -> ground */
    {  0,  0,  1 },   /* left -> delayed left (no change) */
    {  1, -1,  2 },   /* right (left event) -> delayed right */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  1,  0,  4 },   /* timeout (left press) -> pressed left */
  },
/* 2 delayed right */
  {
    {  3, -3,  0 },   /* nothing (right event) -> ground */
    {  3, -3,  1 },   /* left (right event) -> delayed left (no change) */
    {  0,  0,  2 },   /* right -> delayed right (no change) */
    {  2,  0,  3 },   /* left & right (middle press) -> pressed middle */
    {  3,  0,  5 },   /* timeout (right press) -> pressed right */
  },
/* 3 pressed middle */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    {  0,  0,  7 },   /* left -> released right */
    {  0,  0,  6 },   /* right -> released left */
    {  0,  0,  3 },   /* left & right -> pressed middle (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 4 pressed left */
  {
    { -1,  0,  0 },   /* nothing (left release) -> ground */
    {  0,  0,  4 },   /* left -> pressed left (no change) */
    { -1,  0,  2 },   /* right (left release) -> delayed right */
    {  3,  0, 10 },   /* left & right (right press) -> pressed both */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 5 pressed right */
  {
    { -3,  0,  0 },   /* nothing (right release) -> ground */
    { -3,  0,  1 },   /* left (right release) -> delayed left */
    {  0,  0,  5 },   /* right -> pressed right (no change) */
    {  1,  0, 10 },   /* left & right (left press) -> pressed both */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 6 released left */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    { -2,  0,  1 },   /* left (middle release) -> delayed left */
    {  0,  0,  6 },   /* right -> released left (no change) */
    {  1,  0,  8 },   /* left & right (left press) -> repressed left */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 7 released right */
  {
    { -2,  0,  0 },   /* nothing (middle release) -> ground */
    {  0,  0,  7 },   /* left -> released right (no change) */
    { -2,  0,  2 },   /* right (middle release) -> delayed right */
    {  3,  0,  9 },   /* left & right (right press) -> repressed right */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 8 repressed left */
  {
    { -2, -1,  0 },   /* nothing (middle release, left release) -> ground */
    { -2,  0,  4 },   /* left (middle release) -> pressed left */
    { -1,  0,  6 },   /* right (left release) -> released left */
    {  0,  0,  8 },   /* left & right -> repressed left (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 9 repressed right */
  {
    { -2, -3,  0 },   /* nothing (middle release, right release) -> ground */
    { -3,  0,  7 },   /* left (right release) -> released right */
    { -2,  0,  5 },   /* right (middle release) -> pressed right */
    {  0,  0,  9 },   /* left & right -> repressed right (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
/* 10 pressed both */
  {
    { -1, -3,  0 },   /* nothing (left release, right release) -> ground */
    { -3,  0,  4 },   /* left (right release) -> pressed left */
    { -1,  0,  5 },   /* right (left release) -> pressed right */
    {  0,  0, 10 },   /* left & right -> pressed both (no change) */
    {  0,  0, -1 },   /* timeout N/A */
  },
};

/*
 * 2) Pressing a modifier + left button to emulate middle button:
 *
 * At each state, we need handlers for the following events
 *   0: Modifier pressed
 *   1: Modifier released
 *   2: Left button pressed / released
 *   3: Other button pressed / released
 *   4: A key is pressed/released (not the modifier)
 *
 * a handler consists of three values:
 *   0: action1 - Modifler to send
 *   1: action2 - Button emulation mode
 *   2: Next emulation state
 *
 * action1:
 * action = 2: Send modifier down followed by modifier up
 * action = 1: Send modifier down
 * action = 0: nothing
 * action = -1: Send modifier up
 * action = 10: print error, nothing
 *
 * action2:
 * action = 0: Dont Emulate
 * action < 0: Emulate
 *
 * Note that a key pressed event (where the key is not the emulation modifier) always sends the key.
 */

EmulateMB::Action EmulateMB::mbStateTab[8][5] = {
/* 0 ground */
  {
    {  0,  0,  1 },   /* mod down -> mod down */
    { 10,  0,  0 },   /* mod up -> error, self */
    {  0,  0,  2 },   /* left button -> button down */
    {  0,  0,  0 },   /* other button -> error, self*/
    {  0,  0,  0 }    /* key pressed -> self */
  },
/* 1 mod down */
  {
    {  0,  0,  1 },   /* mod down -> self */
    {  2,  0,  0 },   /* mod up -> ground */
    {  0,  1,  3 },   /* left button -> emulating both down */
    {  1,  0,  6 },   /* other button -> non emulated, mod down */
    {  1,  0,  6 }    /* key pressed -> non emulated, mod down */
  },
/* 2 button down */
  {
    {  1,  0,  5 },   /* mod down -> non emulated, both down */
    { 10,  0,  0 },   /* mod up -> error, ground */
    {  0,  0,  0 },   /* left button -> ground */
    {  0,  0,  2 },   /* other button -> self */
    {  0,  0,  2 }    /* key pressed -> self */
  },
/* 3 emulating both down */
  {
    { 10,  0,  0 },   /* mod down -> error, ground */
    {  0,  0,  4 },   /* mod up -> after emulation, button down, mod up */
    {  0,  1,  7 },   /* left button -> after emulation, button up, mod down */
    {  0,  1,  3 },   /* other button -> self */
    {  0,  0,  3 }    /* key pressed -> self */
  },
/* 4 after emulation, button down, mod up */
  {
    {  0,  0,  3 },   /* mod down -> emulating both down */
    { 10,  0,  0 },   /* mod up -> error, ground */
    {  0,  1,  0 },   /* left button -> error, ground */
    {  0,  1,  4 },   /* other button -> self */
    {  0,  0,  4 }    /* key pressed -> self */
  },
/* 5 non emulated, both down, mod sent to server */
  {
    { 10,  0,  0 },   /* mod down -> error, ground */
    { -1,  0,  2 },   /* mod up -> button down */
    {  0,  0,  6 },   /* left button -> non emulated, mod physicall down. button up */
    {  0,  0,  5 },   /* other button -> self */
    {  0,  0,  5 }    /* key pressed -> self */
  },
/* 6 non emulated, mod physicall down. button up. mod down sent to server */
  {
    { 10,  0,  0 },   /* mod down -> error, ground */
    { -1,  0,  0 },   /* mod up -> ground */
    { -1,  1,  3 },   /* left button -> emulating both down */
    {  0,  0,  6 },   /* other button -> self */
    {  0,  0,  6 }    /* key pressed -> self */
  },
/* 7 after emulation, button up, mod down. mod not sent to server */
  {
    { 10,  0,  0 },   /* mod down -> error, ground */
    {  0,  0,  0 },   /* mod up -> ground */
    {  0,  1,  3 },   /* left button -> emulating both down */
    {  1,  0,  6 },   /* other button -> non emulated, mod physicall down. button up */
    {  1,  0,  6 }    /* key pressed -> non emulated, mod physicall down. button up */
  }
};

EmulateMB::EmulateMB(rdr::U32 emulateMBModKey)
  : state(0), emulatedButtonMask(0), timer(this), emulateMiddleButtonModifierKey(emulateMBModKey),
    mbState(0), mbLastButtonMask(0)
{
}

void EmulateMB::mbFsmDoMod(int modAction)
{
  if (modAction == 1 || modAction == 2)
    writeKeyEvent(emulateMiddleButtonModifierKey, modKeyCode, true);
  if (modAction == -1 || modAction == 2)
    writeKeyEvent(emulateMiddleButtonModifierKey, modKeyCode, false);
}

bool EmulateMB::filterKeyPressRelease(bool is_press, int keyCode, rdr::U32 keySym)
{
  if (!emulateMiddleButtonMod)
    return false;

  Action *action;
  if (keySym == emulateMiddleButtonModifierKey || keyCode == modKeyCode) {
    if (is_press) {
      action = &mbStateTab[mbState][0];
      modKeyCode = keyCode;
    }
    else
      action = &mbStateTab[mbState][1];
  }
  else {
    action = &mbStateTab[mbState][4];
  }

  mbFSM(*action);
  // If another key was pressed then signal the caller to send it also.
  return keyCode == modKeyCode;
}

bool EmulateMB::filterKeyPress(int keyCode, rdr::U32 keySym) {
  return filterKeyPressRelease(true, keyCode, keySym);
}

bool EmulateMB::filterKeyRelease(int keyCode)
{
  return filterKeyPressRelease(false, keyCode, -1);
}

void EmulateMB::mbFSM(Action action)
{
  if (action[0] == 10) {
    vlog.error(_("Bad state transition. In state %d\n"), mbState);
    mbState = 0;
    return;
  }
  mbFsmDoMod(action[0]);
  mbState = action[2];
}

void EmulateMB::filterPointerEventMod(const rfb::Point& pos, int buttonMask)
{
  Action *action;

  if (buttonMask != mbLastButtonMask) {
    // Button 1 changed.
    if ((buttonMask & 1) != (mbLastButtonMask & 1))
      action = &mbStateTab[mbState][2]; // Left button
    else
      action = &mbStateTab[mbState][3]; // Other button

    mbLastButtonMask = buttonMask;
    if ((*action)[1]) {
      // Emulate. Set button 2 to equal button 1 and turn off button 1.
      int button1 = buttonMask & 0x1;
      buttonMask = (buttonMask & ~0x3) | button1 << 1;
    }

    mbFSM(*action);
  }
  sendPointerEvent(pos, buttonMask);
}

void EmulateMB::filterPointerEvent(const rfb::Point& pos, int buttonMask)
{
  if (emulateMiddleButton)
    filterPointerEventLR(pos, buttonMask);
  else if (emulateMiddleButtonMod)
    filterPointerEventMod(pos, buttonMask);
  else
    sendPointerEvent(pos, buttonMask);
}

void EmulateMB::filterPointerEventLR(const rfb::Point& pos, int buttonMask)
{
  int btstate;
  int action1, action2;
  int lastState;

  lastButtonMask = buttonMask;
  lastPos = pos;

  btstate = 0;

  if (buttonMask & 0x1)
    btstate |= 0x1;

  if (buttonMask & 0x4)
    btstate |= 0x2;

  if ((state > 10) || (state < 0))
    throw rfb::Exception(_("Invalid state for 3 button emulation"));

  action1 = stateTab[state][btstate][0];

  if (action1 != 0) {
    // Some presses are delayed, that means we have to check if that's
    // the case and send the position corresponding to where the event
    // first was initiated
    if ((stateTab[state][4][2] >= 0) && action1 > 0)
      // We have a timeout state and a button press (a delayed press),
      // always use the original position when leaving a timeout state,
      // whether the timeout was triggered or not
      sendAction(origPos, buttonMask, action1);
    else
      // Normal non-delayed event
      sendAction(pos, buttonMask, action1);
  }

  action2 = stateTab[state][btstate][1];

  // In our case with the state machine, action2 always occurs during a button
  // release but if this change we need handle action2 accordingly
  if (action2 != 0) {
    if ((stateTab[state][4][2] >= 0) && action2 > 0)
      sendAction(origPos, buttonMask, action2);
    else
      // Normal non-delayed event
      sendAction(pos, buttonMask, action2);
  }

  // Still send a pointer move event even if there are no actions.
  // However if the timer is running then we are supressing _all_
  // events, even movement. The pointer's actual position will be
  // sent once the timer fires or is abandoned.
  if ((action1 == 0) && (action2 == 0) && !timer.isStarted()) {
    buttonMask = createButtonMask(buttonMask);
    sendPointerEvent(pos, buttonMask);
  }

  lastState = state;
  state = stateTab[state][btstate][2];

  if (lastState != state) {
    timer.stop();

    if (stateTab[state][4][2] >= 0) {
      // We need to save the original position so that
      // drags start from the correct position
      origPos = pos;
      timer.start(50);
    }
  }
}

bool EmulateMB::handleTimeout(rfb::Timer *t)
{
  int action1, action2;
  int buttonMask;

  if (&timer != t)
    return false;

  if ((state > 10) || (state < 0))
    throw rfb::Exception(_("Invalid state for 3 button emulation"));

  // Timeout shouldn't trigger when there's no timeout action
  assert(stateTab[state][4][2] >= 0);

  action1 = stateTab[state][4][0];
  if (action1 != 0)
    sendAction(origPos, lastButtonMask, action1);

  action2 = stateTab[state][4][1];
  if (action2 != 0)
    sendAction(origPos, lastButtonMask, action2);

  buttonMask = lastButtonMask;

  // Pointer move events are not sent when waiting for the timeout.
  // However, we can't let the position get out of sync so when
  // the pointer has moved we have to send the latest position here.
  if (!origPos.equals(lastPos)) {
    buttonMask = createButtonMask(buttonMask);
    sendPointerEvent(lastPos, buttonMask);
  }

  state = stateTab[state][4][2];

  return false;
}

void EmulateMB::sendAction(const rfb::Point& pos, int buttonMask, int action)
{
  assert(action != 0);

  if (action < 0)
    emulatedButtonMask &= ~(1 << ((-action) - 1));
  else
    emulatedButtonMask |= (1 << (action - 1));

  buttonMask = createButtonMask(buttonMask);
  sendPointerEvent(pos, buttonMask);
}

int EmulateMB::createButtonMask(int buttonMask)
{
  // Unset left and right buttons in the mask
  buttonMask &= ~0x5;

  // Set the left and right buttons according to the action
  return buttonMask |= emulatedButtonMask;
}
