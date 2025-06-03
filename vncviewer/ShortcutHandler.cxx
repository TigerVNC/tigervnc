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

#include "ShortcutHandler.h"
#include "i18n.h"

ShortcutHandler::ShortcutHandler() :
  modifierMask(0), state(Idle)
{
}

void ShortcutHandler::setModifiers(unsigned mask)
{
  modifierMask = mask;
  reset();
}

ShortcutHandler::KeyAction ShortcutHandler::handleKeyPress(int keyCode,
                                                           uint32_t keySym)
{
  unsigned modifier, pressedMask;
  std::map<int, uint32_t>::const_iterator iter;

  pressedKeys[keyCode] = keySym;

  if (modifierMask == 0)
    return KeyNormal;

  modifier = keySymToModifier(keySym);

  pressedMask = 0;
  for (iter = pressedKeys.begin(); iter != pressedKeys.end(); ++iter)
    pressedMask |= keySymToModifier(iter->second);

  switch (state) {
  case Idle:
  case Arming:
  case Rearming:
    if (pressedMask == modifierMask) {
      // All triggering modifier keys are pressed
      state = Armed;
    } if (modifier && ((modifier & modifierMask) == modifier)) {
      // The new key is part of the triggering set
      if (state == Idle)
        state = Arming;
    } else {
      // The new key was something else
      state = Wedged;
    }
    return KeyNormal;
  case Armed:
    if (modifier && ((modifier & modifierMask) == modifier)) {
      // The new key is part of the triggering set
      return KeyNormal;
    } else if (modifier) {
      // The new key is some other modifier
      state = Wedged;
      return KeyNormal;
    } else {
      // The new key was something else
      state = Firing;
      firedKeys.insert(keyCode);
      return KeyShortcut;
    }
    break;
  case Firing:
    if (modifier) {
      // The new key is a modifier (may or may not be part of the
      // triggering set)
      return KeyIgnore;
    } else {
      // The new key was something else
      firedKeys.insert(keyCode);
      return KeyShortcut;
    }
  default:
    break;
  }

  return KeyNormal;
}

ShortcutHandler::KeyAction ShortcutHandler::handleKeyRelease(int keyCode)
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
    pressedMask |= keySymToModifier(iter->second);

  switch (state) {
  case Arming:
    action = KeyNormal;
    break;
  case Armed:
    if (pressedKeys.empty())
      action = KeyUnarm;
    else if (pressedMask == modifierMask)
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
      action = KeyShortcut;
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

void ShortcutHandler::reset()
{
  state = Idle;
  firedKeys.clear();
  pressedKeys.clear();
}

// Keep list of valid values in sync with shortcutModifiers
unsigned ShortcutHandler::parseModifier(const char* key)
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

const char* ShortcutHandler::modifierString(unsigned key)
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

const char* ShortcutHandler::modifierPrefix(unsigned mask,
                                            bool justPrefix)
{
  static char prefix[256];

  prefix[0] = '\0';
  if (mask & Control) {
#ifdef __APPLE__
    strcat(prefix, "⌃");
#else
    strcat(prefix, _("Ctrl"));
    strcat(prefix, "+");
#endif
  }
  if (mask & Shift) {
#ifdef __APPLE__
    strcat(prefix, "⇧");
#else
    strcat(prefix, _("Shift"));
    strcat(prefix, "+");
#endif
  }
  if (mask & Alt) {
#ifdef __APPLE__
    strcat(prefix, "⌥");
#else
    strcat(prefix, _("Alt"));
    strcat(prefix, "+");
#endif
  }
  if (mask & Super) {
#ifdef __APPLE__
    strcat(prefix, "⌘");
#else
    strcat(prefix, _("Win"));
    strcat(prefix, "+");
#endif
  }

  if (prefix[0] == '\0')
    return "";

  if (justPrefix) {
#ifndef __APPLE__
    prefix[strlen(prefix)-1] = '\0';
#endif
    return prefix;
  }

#ifdef __APPLE__
  strcat(prefix, "\xc2\xa0"); // U+00A0 NO-BREAK SPACE
#endif

  return prefix;
}

unsigned ShortcutHandler::keySymToModifier(uint32_t keySym)
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
