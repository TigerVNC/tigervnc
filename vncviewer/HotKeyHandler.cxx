/* Copyright 2021-2025 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define XK_MISCELLANY
#include <rfb/keysymdef.h>

#include "HotKeyHandler.h"
#include "i18n.h"

HotKeyHandler::HotKeyHandler() :
  comboMask(0), state(Idle)
{
}

void HotKeyHandler::setHotKeyCombo(unsigned mask)
{
  comboMask = mask;
  reset();
}

HotKeyHandler::KeyAction HotKeyHandler::handleKeyPress(int keyCode,
                                                       uint32_t keySym)
{
  unsigned mask, pressedMask;
  std::map<int, uint32_t>::const_iterator iter;

  pressedKeys[keyCode] = keySym;

  if (comboMask == 0)
    return KeyNormal;

  mask = keySymToMask(keySym);

  pressedMask = 0;
  for (iter = pressedKeys.begin(); iter != pressedKeys.end(); ++iter)
    pressedMask |= keySymToMask(iter->second);

  switch (state) {
  case Idle:
  case Arming:
  case Rearming:
    if (pressedMask == comboMask) {
      // All combo keys are pressed
      state = Armed;
    } if (mask && ((mask & comboMask) == mask)) {
      // The new key is part of the combo
      if (state == Idle)
        state = Arming;
    } else {
      // The new key was something else
      state = Wedged;
    }
    return KeyNormal;
  case Armed:
    if (mask && ((mask & comboMask) == mask)) {
      // The new key is part of the combo
      return KeyNormal;
    } else if (mask) {
      // The new key is some other modifier
      state = Wedged;
      return KeyNormal;
    } else {
      // The new key was something else
      state = Firing;
      firedKeys.insert(keyCode);
      return KeyHotKey;
    }
    break;
  case Firing:
    if (mask) {
      // The new key is a modifier (may or may not be part of the combo)
      return KeyIgnore;
    } else {
      // The new key was something else
      firedKeys.insert(keyCode);
      return KeyHotKey;
    }
  default:
    break;
  }

  return KeyNormal;
}

HotKeyHandler::KeyAction HotKeyHandler::handleKeyRelease(int keyCode)
{
  bool firedKey;
  unsigned pressedMask;
  std::map<int, uint32_t>::const_iterator iter;
  KeyAction action;

  firedKey = firedKeys.count(keyCode) != 0;

  firedKeys.erase(keyCode);
  pressedKeys.erase(keyCode);

  pressedMask = 0;
  for (iter = pressedKeys.begin(); iter != pressedKeys.end(); ++iter)
    pressedMask |= keySymToMask(iter->second);

  switch (state) {
  case Arming:
    action = KeyNormal;
    break;
  case Armed:
    if (pressedKeys.empty())
      action = KeyUnarm;
    else if (pressedMask == comboMask)
      action = KeyNormal;
    else {
      action = KeyNormal;
      state = Rearming;
    }
    break;
  case Rearming:
    if (pressedKeys.empty())
      action = KeyUnarm;
    else
      action = KeyNormal;
    break;
  case Firing:
    if (firedKey)
      action = KeyHotKey;
    else
      action = KeyIgnore;
    break;
  default:
    action = KeyNormal;
  }

  if (pressedKeys.empty())
    state = Idle;

  return action;
}

void HotKeyHandler::reset()
{
  state = Idle;
  firedKeys.clear();
  pressedKeys.clear();
}

// Keep list of valid values in sync with hotKeyCombo
unsigned HotKeyHandler::parseHotKey(const char* key)
{
  if (strcasecmp(key, "Ctrl") == 0)
    return Control;
  else if (strcasecmp(key, "Shift") == 0)
    return Shift;
  else if (strcasecmp(key, "Alt") == 0)
    return Alt;
  else if (strcasecmp(key, "Win") == 0)
    return Super;
  else if (strcasecmp(key, "Super") == 0)
    return Super;
  else if (strcasecmp(key, "Option") == 0)
    return Alt;
  else if (strcasecmp(key, "Cmd") == 0)
    return Super;
  else
    return 0;
}

const char* HotKeyHandler::hotKeyString(unsigned key)
{
  if (key == Control)
    return "Ctrl";
  if (key == Shift)
    return "Shift";
  if (key == Alt)
    return "Alt";
  if (key == Super)
    return "Super";

  return "";
}

const char* HotKeyHandler::comboPrefix(unsigned mask, bool justCombo)
{
  static char combo[256];

  combo[0] = '\0';
  if (mask & Control) {
#ifdef __APPLE__
    strcat(combo, "⌃");
#else
    strcat(combo, _("Ctrl"));
    strcat(combo, "+");
#endif
  }
  if (mask & Shift) {
#ifdef __APPLE__
    strcat(combo, "⇧");
#else
    strcat(combo, _("Shift"));
    strcat(combo, "+");
#endif
  }
  if (mask & Alt) {
#ifdef __APPLE__
    strcat(combo, "⌥");
#else
    strcat(combo, _("Alt"));
    strcat(combo, "+");
#endif
  }
  if (mask & Super) {
#ifdef __APPLE__
    strcat(combo, "⌘");
#else
    strcat(combo, _("Win"));
    strcat(combo, "+");
#endif
  }

  if (combo[0] == '\0')
    return "";

  if (justCombo) {
#ifndef __APPLE__
    combo[strlen(combo)-1] = '\0';
#endif
    return combo;
  }

#ifdef __APPLE__
  strcat(combo, "\xc2\xa0"); // U+00A0 NO-BREAK SPACE
#endif

  return combo;
}

unsigned HotKeyHandler::keySymToMask(uint32_t keySym)
{
  switch (keySym) {
  case XK_Control_L:
  case XK_Control_R:
    return Control;
  case XK_Shift_L:
  case XK_Shift_R:
    return Shift;
  case XK_Alt_L:
  case XK_Alt_R:
    return Alt;
  case XK_Super_L:
  case XK_Super_R:
  case XK_Hyper_L:
  case XK_Hyper_R:
    return Super;
  }

  return 0;
}
