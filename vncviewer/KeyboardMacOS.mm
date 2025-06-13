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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <algorithm>

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>

// This wasn't added until 10.12
#if !defined(MAC_OS_X_VERSION_10_12) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
const int kVK_RightCommand = 0x36;
#endif
// And this is still missing
const int kVK_Menu = 0x6E;

#include <core/LogWriter.h>

#define XK_LATIN1
#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#include <rfb/XF86keysym.h>
#include <rfb/ledStates.h>

#define NoSymbol 0

#include "i18n.h"
#include "keysym2ucs.h"
#include "KeyboardMacOS.h"

extern const unsigned short code_map_osx_to_qnum[];
extern const unsigned int code_map_osx_to_qnum_len;

static core::LogWriter vlog("KeyboardMacOS");

static const unsigned kvk_map[][2] = {
  { kVK_Return,         XK_Return },
  { kVK_Tab,            XK_Tab },
  { kVK_Space,          XK_space },
  { kVK_Delete,         XK_BackSpace },
  { kVK_Escape,         XK_Escape },
  { kVK_RightCommand,   XK_Super_R },
  { kVK_Command,        XK_Super_L },
  { kVK_Shift,          XK_Shift_L },
  { kVK_CapsLock,       XK_Caps_Lock },
  { kVK_Option,         XK_Alt_L },
  { kVK_Control,        XK_Control_L },
  { kVK_RightShift,     XK_Shift_R },
  { kVK_RightOption,    XK_Alt_R },
  { kVK_RightControl,   XK_Control_R },
  { kVK_F17,            XK_F17 },
  { kVK_VolumeUp,       XF86XK_AudioRaiseVolume },
  { kVK_VolumeDown,     XF86XK_AudioLowerVolume },
  { kVK_Mute,           XF86XK_AudioMute },
  { kVK_F18,            XK_F18 },
  { kVK_F19,            XK_F19 },
  { kVK_F20,            XK_F20 },
  { kVK_F5,             XK_F5 },
  { kVK_F6,             XK_F6 },
  { kVK_F7,             XK_F7 },
  { kVK_F3,             XK_F3 },
  { kVK_F8,             XK_F8 },
  { kVK_F9,             XK_F9 },
  { kVK_F11,            XK_F11 },
  { kVK_F13,            XK_F13 },
  { kVK_F16,            XK_F16 },
  { kVK_F14,            XK_F14 },
  { kVK_F10,            XK_F10 },
  { kVK_Menu,           XK_Menu },
  { kVK_F12,            XK_F12 },
  { kVK_F15,            XK_F15 },
  // Should we send Insert here?
  { kVK_Help,           XK_Help },
  { kVK_Home,           XK_Home },
  { kVK_PageUp,         XK_Page_Up },
  { kVK_ForwardDelete,  XK_Delete },
  { kVK_F4,             XK_F4 },
  { kVK_End,            XK_End },
  { kVK_F2,             XK_F2 },
  { kVK_PageDown,       XK_Page_Down },
  { kVK_F1,             XK_F1 },
  { kVK_LeftArrow,      XK_Left },
  { kVK_RightArrow,     XK_Right },
  { kVK_DownArrow,      XK_Down },
  { kVK_UpArrow,        XK_Up },

  // The OS X headers claim these keys are not layout independent.
  // Could it be because of the state of the decimal key?
  /* { kVK_ANSI_KeypadDecimal,     XK_KP_Decimal }, */ // see below
  { kVK_ANSI_KeypadMultiply,    XK_KP_Multiply },
  { kVK_ANSI_KeypadPlus,        XK_KP_Add },
  // OS X doesn't have NumLock, so is this really correct?
  { kVK_ANSI_KeypadClear,       XK_Num_Lock },
  { kVK_ANSI_KeypadDivide,      XK_KP_Divide },
  { kVK_ANSI_KeypadEnter,       XK_KP_Enter },
  { kVK_ANSI_KeypadMinus,       XK_KP_Subtract },
  { kVK_ANSI_KeypadEquals,      XK_KP_Equal },
  { kVK_ANSI_Keypad0,           XK_KP_0 },
  { kVK_ANSI_Keypad1,           XK_KP_1 },
  { kVK_ANSI_Keypad2,           XK_KP_2 },
  { kVK_ANSI_Keypad3,           XK_KP_3 },
  { kVK_ANSI_Keypad4,           XK_KP_4 },
  { kVK_ANSI_Keypad5,           XK_KP_5 },
  { kVK_ANSI_Keypad6,           XK_KP_6 },
  { kVK_ANSI_Keypad7,           XK_KP_7 },
  { kVK_ANSI_Keypad8,           XK_KP_8 },
  { kVK_ANSI_Keypad9,           XK_KP_9 },
  // Japanese Keyboard Support
  { kVK_JIS_Eisu,               XK_Eisu_toggle },
  { kVK_JIS_Kana,               XK_Hiragana_Katakana },
};

KeyboardMacOS::KeyboardMacOS(KeyboardHandler* handler_)
  : Keyboard(handler_)
{
}

KeyboardMacOS::~KeyboardMacOS()
{
}

bool KeyboardMacOS::isKeyboardReset(const void* event)
{
  const NSEvent* nsevent = (const NSEvent*)event;

  assert(event);

  // If we get a NSFlagsChanged event with key code 0 then this isn't
  // an actual keyboard event but rather the system trying to sync up
  // modifier state after it has stolen input for some reason (e.g.
  // Cmd+Tab)

  if ([nsevent type] != NSFlagsChanged)
    return false;
  if ([nsevent keyCode] != 0)
    return false;

  return true;
}

bool KeyboardMacOS::handleEvent(const void* event)
{
  const NSEvent* nsevent = (NSEvent*)event;
  unsigned systemKeyCode;

  assert(event);

  if (!isKeyboardEvent(nsevent))
    return false;

  systemKeyCode = getSystemKeyCode(nsevent);

  if (isKeyPress(nsevent)) {
    uint32_t keyCode;
    uint32_t keySym;
    unsigned modifiers;

    keyCode = translateSystemKeyCode(systemKeyCode);

    // We want a "normal" symbol out of the event, which basically means
    // we only respect the shift and alt/altgr modifiers. Cocoa can help
    // us if we only wanted shift, but as we also want alt/altgr, we'll
    // have to do some lookup ourselves. This matches our behaviour on
    // other platforms.

    modifiers = 0;
    if ([nsevent modifierFlags] & NSAlphaShiftKeyMask)
      modifiers |= alphaLock;
    if ([nsevent modifierFlags] & NSShiftKeyMask)
      modifiers |= shiftKey;
    if ([nsevent modifierFlags] & NSAlternateKeyMask)
      modifiers |= optionKey;

    keySym = translateToKeySym([nsevent keyCode], modifiers);
    if (keySym == NoSymbol) {
      vlog.error(_("No symbol for key code 0x%02x (in the current state)"),
                 systemKeyCode);
    }

    handler->handleKeyPress(systemKeyCode, keyCode, keySym);

    // We don't get any release events for CapsLock, so we have to
    // send the release right away.
    if (keySym == XK_Caps_Lock)
      handler->handleKeyRelease(systemKeyCode);
  } else {
    handler->handleKeyRelease(systemKeyCode);
  }

  return true;
}

std::list<uint32_t> KeyboardMacOS::translateToKeySyms(int systemKeyCode)
{
  std::list<uint32_t> keySyms;
  unsigned mods;

  uint32_t ks;

  // Start with no modifiers
  ks = translateToKeySym(systemKeyCode, 0);
  if (ks != NoSymbol)
    keySyms.push_back(ks);

  // Next just a single modifier at a time
  for (mods = cmdKey; mods <= controlKey; mods <<= 1) {
    std::list<uint32_t>::const_iterator iter;

    ks = translateToKeySym(systemKeyCode, mods);
    if (ks == NoSymbol)
      continue;

    iter = std::find(keySyms.begin(), keySyms.end(), ks);
    if (iter != keySyms.end())
      continue;

    keySyms.push_back(ks);
  }

  // Finally everything
  for (mods = cmdKey; mods < (controlKey << 1); mods += cmdKey) {
    std::list<uint32_t>::const_iterator iter;

    ks = translateToKeySym(systemKeyCode, mods);
    if (ks == NoSymbol)
      continue;

    iter = std::find(keySyms.begin(), keySyms.end(), ks);
    if (iter != keySyms.end())
      continue;

    keySyms.push_back(ks);
  }

  return keySyms;
}

unsigned KeyboardMacOS::getLEDState()
{
  unsigned state;
  int ret;
  bool on;

  state = 0;

  ret = getModifierLockState(kIOHIDCapsLockState, &on);
  if (ret != 0) {
    vlog.error(_("Failed to get keyboard LED state: %d"), ret);
    return rfb::ledUnknown;
  }
  if (on)
    state |= rfb::ledCapsLock;

  ret = getModifierLockState(kIOHIDNumLockState, &on);
  if (ret != 0) {
    vlog.error(_("Failed to get keyboard LED state: %d"), ret);
    return rfb::ledUnknown;
  }
  if (on)
    state |= rfb::ledNumLock;

  // No support for Scroll Lock //

  return state;
}

void KeyboardMacOS::setLEDState(unsigned state)
{
  int ret;

  ret = setModifierLockState(kIOHIDCapsLockState, state & rfb::ledCapsLock);
  if (ret != 0) {
    vlog.error(_("Failed to update keyboard LED state: %d"), ret);
    return;
  }

  ret = setModifierLockState(kIOHIDNumLockState, state & rfb::ledNumLock);
  if (ret != 0) {
    vlog.error(_("Failed to update keyboard LED state: %d"), ret);
    return;
  }

  // No support for Scroll Lock //
}

bool KeyboardMacOS::isKeyboardEvent(const NSEvent* nsevent)
{
  switch ([nsevent type]) {
  case NSKeyDown:
  case NSKeyUp:
    return true;
  case NSFlagsChanged:
    if (isKeyboardReset(nsevent))
      return false;
    return true;
  default:
    return false;
  }
}

bool KeyboardMacOS::isKeyPress(const NSEvent* nsevent)
{
  if ([nsevent type] == NSKeyDown)
    return true;

  if ([nsevent type] == NSFlagsChanged) {
    UInt32 mask;

    // We don't see any event on release of CapsLock
    if ([nsevent keyCode] == kVK_CapsLock)
      return true;

    // These are entirely undocumented, but I cannot find any other way
    // of differentiating between left and right keys
    switch ([nsevent keyCode]) {
    case kVK_RightCommand:
      mask = 0x0010;
      break;
    case kVK_Command:
      mask = 0x0008;
      break;
    case kVK_Shift:
      mask = 0x0002;
      break;
    case kVK_CapsLock:
      // We don't see any event on release of CapsLock
      return 1;
    case kVK_Option:
      mask = 0x0020;
      break;
    case kVK_Control:
      mask = 0x0001;
      break;
    case kVK_RightShift:
      mask = 0x0004;
      break;
    case kVK_RightOption:
      mask = 0x0040;
      break;
    case kVK_RightControl:
      mask = 0x2000;
      break;
    default:
      return false;
    }

    if ([nsevent modifierFlags] & mask)
      return true;
    else
      return false;
  }

  return false;
}

unsigned KeyboardMacOS::getSystemKeyCode(const NSEvent* nsevent)
{
  unsigned keycode;

  keycode = [nsevent keyCode];

  // macOS swaps these two keys for unknown reasons for ISO layouts
  if (KBGetLayoutType(LMGetKbdType()) == kKeyboardISO) {
    if (keycode == kVK_ANSI_Grave)
      return kVK_ISO_Section;
    if (keycode == kVK_ISO_Section)
      return kVK_ANSI_Grave;
  }

  return keycode;
}

uint32_t KeyboardMacOS::translateSystemKeyCode(int systemKeyCode)
{
  if ((unsigned)systemKeyCode >= code_map_osx_to_qnum_len)
    return 0;

  return code_map_osx_to_qnum[systemKeyCode];
}

uint32_t KeyboardMacOS::translateToKeySym(unsigned keyCode,
                                          unsigned modifierFlags)
{
  const UCKeyboardLayout *layout;
  OSStatus err;

  TISInputSourceRef keyboard;
  CFDataRef uchr;

  UInt32 dead_state;
  UniCharCount max_len, actual_len;
  UniChar string[255];

  // Start with keys that either don't generate a symbol, or
  // generate the same symbol as some other key.
  for (size_t i = 0;i < sizeof(kvk_map)/sizeof(kvk_map[0]);i++) {
    if (keyCode == kvk_map[i][0])
      return kvk_map[i][1];
  }

  keyboard = TISCopyCurrentKeyboardLayoutInputSource();
  uchr = (CFDataRef)TISGetInputSourceProperty(keyboard,
                                              kTISPropertyUnicodeKeyLayoutData);
  if (uchr == nullptr)
    return NoSymbol;

  layout = (const UCKeyboardLayout*)CFDataGetBytePtr(uchr);
  if (layout == nullptr)
    return NoSymbol;

  dead_state = 0;
  max_len = sizeof(string)/sizeof(*string);

  modifierFlags = (modifierFlags >> 8) & 0xff;

  err = UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierFlags,
                       LMGetKbdType(), 0, &dead_state, max_len, &actual_len,
                       string);
  if (err != noErr)
    return NoSymbol;

  // Dead key?
  if (dead_state != 0) {
    unsigned combining;

    // We have no fool proof way of asking what dead key this is.
    // Assume we get a spacing equivalent if we press the
    // same key again, and try to deduce something from that.
    err = UCKeyTranslate(layout, keyCode, kUCKeyActionDown, modifierFlags,
                         LMGetKbdType(), 0, &dead_state, max_len, &actual_len,
                         string);
    if (err != noErr)
      return NoSymbol;

    // FIXME: Some dead keys are given as NBSP + combining character
    if (actual_len != 1)
      return NoSymbol;

    combining = ucs2combining(string[0]);
    if (combining == (unsigned)-1)
      return NoSymbol;

    return ucs2keysym(combining);
  }

  // Sanity check
  if (actual_len != 1)
    return NoSymbol;

  // OS X always sends the same key code for the decimal key on the
  // num pad, but X11 wants different keysyms depending on if it should
  // be a comma or full stop.
  if (keyCode == 0x41) {
    switch (string[0]) {
    case ',':
      return XK_KP_Separator;
    case '.':
      return XK_KP_Decimal;
    default:
      return NoSymbol;
    }
  }

  return ucs2keysym(string[0]);
}

int KeyboardMacOS::openHID(unsigned int* ioc)
{
  kern_return_t ret;
  io_service_t ios;
  CFMutableDictionaryRef mdict;

  mdict = IOServiceMatching(kIOHIDSystemClass);
  ios = IOServiceGetMatchingService(kIOMasterPortDefault,
                                    (CFDictionaryRef) mdict);
  if (!ios)
    return KERN_FAILURE;

  ret = IOServiceOpen(ios, mach_task_self(), kIOHIDParamConnectType, ioc);
  IOObjectRelease(ios);
  if (ret != KERN_SUCCESS)
    return ret;

  return KERN_SUCCESS;
}

int KeyboardMacOS::getModifierLockState(int modifier, bool* on)
{
  kern_return_t ret;
  io_connect_t ioc;

  ret = openHID(&ioc);
  if (ret != KERN_SUCCESS)
    return ret;

  ret = IOHIDGetModifierLockState(ioc, modifier, on);
  IOServiceClose(ioc);
  if (ret != KERN_SUCCESS)
    return ret;

  return KERN_SUCCESS;
}

int KeyboardMacOS::setModifierLockState(int modifier, bool on)
{
  kern_return_t ret;
  io_connect_t ioc;

  ret = openHID(&ioc);
  if (ret != KERN_SUCCESS)
    return ret;

  ret = IOHIDSetModifierLockState(ioc, modifier, on);
  IOServiceClose(ioc);
  if (ret != KERN_SUCCESS)
    return ret;

  return KERN_SUCCESS;
}
