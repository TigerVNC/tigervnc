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

#include <windows.h>

#include <assert.h>

#include <algorithm>

// Missing in at least some versions of MinGW
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR 2
#endif

#include <FL/Fl.H>

#include <core/LogWriter.h>

#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_KOREAN
#include <rfb/keysymdef.h>
#include <rfb/XF86keysym.h>
#include <rfb/ledStates.h>

#define NoSymbol 0

#include "i18n.h"
#include "keysym2ucs.h"
#include "KeyboardWin32.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

// Used to detect fake input (0xaa is not a real key)
static const WORD SCAN_FAKE = 0xaa;

static core::LogWriter vlog("KeyboardWin32");

// Layout independent keys
static const UINT vkey_map[][3] = {
  { VK_CANCEL,              NoSymbol,       XK_Break },
  { VK_BACK,                XK_BackSpace,   NoSymbol },
  { VK_TAB,                 XK_Tab,         NoSymbol },
  { VK_CLEAR,               XK_Clear,       NoSymbol },
  { VK_RETURN,              XK_Return,      XK_KP_Enter },
  { VK_SHIFT,               XK_Shift_L,     NoSymbol },
  { VK_CONTROL,             XK_Control_L,   XK_Control_R },
  { VK_MENU,                XK_Alt_L,       XK_Alt_R },
  { VK_PAUSE,               XK_Pause,       NoSymbol },
  { VK_CAPITAL,             XK_Caps_Lock,   NoSymbol },
  { VK_ESCAPE,              XK_Escape,      NoSymbol },
  { VK_CONVERT,             XK_Henkan,      NoSymbol },
  { VK_NONCONVERT,          XK_Muhenkan,    NoSymbol },
  { VK_PRIOR,               XK_KP_Prior,    XK_Prior },
  { VK_NEXT,                XK_KP_Next,     XK_Next },
  { VK_END,                 XK_KP_End,      XK_End },
  { VK_HOME,                XK_KP_Home,     XK_Home },
  { VK_LEFT,                XK_KP_Left,     XK_Left },
  { VK_UP,                  XK_KP_Up,       XK_Up },
  { VK_RIGHT,               XK_KP_Right,    XK_Right },
  { VK_DOWN,                XK_KP_Down,     XK_Down },
  { VK_SNAPSHOT,            XK_Sys_Req,     XK_Print },
  { VK_INSERT,              XK_KP_Insert,   XK_Insert },
  { VK_DELETE,              XK_KP_Delete,   XK_Delete },
  { VK_LWIN,                NoSymbol,       XK_Super_L },
  { VK_RWIN,                NoSymbol,       XK_Super_R },
  { VK_APPS,                NoSymbol,       XK_Menu },
  { VK_SLEEP,               NoSymbol,       XF86XK_Sleep },
  { VK_NUMPAD0,             XK_KP_0,        NoSymbol },
  { VK_NUMPAD1,             XK_KP_1,        NoSymbol },
  { VK_NUMPAD2,             XK_KP_2,        NoSymbol },
  { VK_NUMPAD3,             XK_KP_3,        NoSymbol },
  { VK_NUMPAD4,             XK_KP_4,        NoSymbol },
  { VK_NUMPAD5,             XK_KP_5,        NoSymbol },
  { VK_NUMPAD6,             XK_KP_6,        NoSymbol },
  { VK_NUMPAD7,             XK_KP_7,        NoSymbol },
  { VK_NUMPAD8,             XK_KP_8,        NoSymbol },
  { VK_NUMPAD9,             XK_KP_9,        NoSymbol },
  { VK_MULTIPLY,            XK_KP_Multiply, NoSymbol },
  { VK_ADD,                 XK_KP_Add,      NoSymbol },
  { VK_SUBTRACT,            XK_KP_Subtract, NoSymbol },
  { VK_DIVIDE,              NoSymbol,       XK_KP_Divide },
  /* VK_SEPARATOR and VK_DECIMAL left out on purpose. See further down. */
  { VK_F1,                  XK_F1,          NoSymbol },
  { VK_F2,                  XK_F2,          NoSymbol },
  { VK_F3,                  XK_F3,          NoSymbol },
  { VK_F4,                  XK_F4,          NoSymbol },
  { VK_F5,                  XK_F5,          NoSymbol },
  { VK_F6,                  XK_F6,          NoSymbol },
  { VK_F7,                  XK_F7,          NoSymbol },
  { VK_F8,                  XK_F8,          NoSymbol },
  { VK_F9,                  XK_F9,          NoSymbol },
  { VK_F10,                 XK_F10,         NoSymbol },
  { VK_F11,                 XK_F11,         NoSymbol },
  { VK_F12,                 XK_F12,         NoSymbol },
  { VK_F13,                 XK_F13,         NoSymbol },
  { VK_F14,                 XK_F14,         NoSymbol },
  { VK_F15,                 XK_F15,         NoSymbol },
  { VK_F16,                 XK_F16,         NoSymbol },
  { VK_F17,                 XK_F17,         NoSymbol },
  { VK_F18,                 XK_F18,         NoSymbol },
  { VK_F19,                 XK_F19,         NoSymbol },
  { VK_F20,                 XK_F20,         NoSymbol },
  { VK_F21,                 XK_F21,         NoSymbol },
  { VK_F22,                 XK_F22,         NoSymbol },
  { VK_F23,                 XK_F23,         NoSymbol },
  { VK_F24,                 XK_F24,         NoSymbol },
  { VK_NUMLOCK,             NoSymbol,       XK_Num_Lock },
  { VK_SCROLL,              XK_Scroll_Lock, NoSymbol },
  { VK_BROWSER_BACK,        NoSymbol,       XF86XK_Back },
  { VK_BROWSER_FORWARD,     NoSymbol,       XF86XK_Forward },
  { VK_BROWSER_REFRESH,     NoSymbol,       XF86XK_Refresh },
  { VK_BROWSER_STOP,        NoSymbol,       XF86XK_Stop },
  { VK_BROWSER_SEARCH,      NoSymbol,       XF86XK_Search },
  { VK_BROWSER_FAVORITES,   NoSymbol,       XF86XK_Favorites },
  { VK_BROWSER_HOME,        NoSymbol,       XF86XK_HomePage },
  { VK_VOLUME_MUTE,         NoSymbol,       XF86XK_AudioMute },
  { VK_VOLUME_DOWN,         NoSymbol,       XF86XK_AudioLowerVolume },
  { VK_VOLUME_UP,           NoSymbol,       XF86XK_AudioRaiseVolume },
  { VK_MEDIA_NEXT_TRACK,    NoSymbol,       XF86XK_AudioNext },
  { VK_MEDIA_PREV_TRACK,    NoSymbol,       XF86XK_AudioPrev },
  { VK_MEDIA_STOP,          NoSymbol,       XF86XK_AudioStop },
  { VK_MEDIA_PLAY_PAUSE,    NoSymbol,       XF86XK_AudioPlay },
  { VK_LAUNCH_MAIL,         NoSymbol,       XF86XK_Mail },
  { VK_LAUNCH_MEDIA_SELECT, NoSymbol,       XF86XK_AudioMedia },
  { VK_LAUNCH_APP1,         NoSymbol,       XF86XK_MyComputer },
  { VK_LAUNCH_APP2,         NoSymbol,       XF86XK_Calculator },
};

// Layout dependent keys, but without useful symbols

// Japanese
static const UINT vkey_map_jp[][3] = {
  { VK_KANA,                XK_Hiragana_Katakana, NoSymbol },
  { VK_KANJI,               XK_Kanji,       NoSymbol },
  { VK_OEM_ATTN,            XK_Eisu_toggle, NoSymbol },
  { VK_OEM_FINISH,          XK_Katakana,    NoSymbol },
  { VK_OEM_COPY,            XK_Hiragana,    NoSymbol },
  // These are really XK_Zenkaku/XK_Hankaku but we have no way of
  // keeping the client and server in sync
  { VK_OEM_AUTO,            XK_Zenkaku_Hankaku, NoSymbol },
  { VK_OEM_ENLW,            XK_Zenkaku_Hankaku, NoSymbol },
  { VK_OEM_BACKTAB,         XK_Romaji,      NoSymbol },
  { VK_ATTN,                XK_Romaji,      NoSymbol },
};

// Korean
static const UINT vkey_map_ko[][3] = {
  { VK_HANGUL,              XK_Hangul,      NoSymbol },
  { VK_HANJA,               XK_Hangul_Hanja, NoSymbol },
};

KeyboardWin32::KeyboardWin32(KeyboardHandler* handler_)
  : Keyboard(handler_), cachedHasAltGr(false), currentLayout(nullptr),
    altGrArmed(false), leftShiftDown(false), rightShiftDown(false)
{
}

KeyboardWin32::~KeyboardWin32()
{
  reset();
}

bool KeyboardWin32::handleEvent(const void* event)
{
  MSG *msg = (MSG*)event;

  assert(event);

  if ((msg->message == WM_MOUSEMOVE) ||
      (msg->message == WM_LBUTTONDOWN) ||
      (msg->message == WM_LBUTTONUP) ||
      (msg->message == WM_RBUTTONDOWN) ||
      (msg->message == WM_RBUTTONUP) ||
      (msg->message == WM_MBUTTONDOWN) ||
      (msg->message == WM_MBUTTONUP) ||
      (msg->message == WM_XBUTTONDOWN) ||
      (msg->message == WM_XBUTTONUP) ||
      (msg->message == WM_MOUSEWHEEL) ||
      (msg->message == WM_MOUSEHWHEEL)) {
    // We can't get a mouse event in the middle of an AltGr sequence, so
    // abort that detection
    if (altGrArmed)
      resolveAltGrDetection(false);
    return false; // We didn't really consume the mouse event
  } else if ((msg->message == WM_KEYDOWN) || (msg->message == WM_SYSKEYDOWN)) {
    UINT vKey;
    bool isExtended;
    int systemKeyCode, keyCode;
    uint32_t keySym;
    BYTE state[256];

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    systemKeyCode = ((msg->lParam >> 16) & 0xff);

    // Windows' touch keyboard doesn't set a scan code for the Alt
    // portion of the AltGr sequence, so we need to help it out
    if (!isExtended && (systemKeyCode == 0x00) && (vKey == VK_MENU)) {
      isExtended = true;
      systemKeyCode = 0x38;
    }

    // Windows doesn't have a proper AltGr, but handles it using fake
    // Ctrl+Alt. However the remote end might not be Windows, so we need
    // to merge those in to a single AltGr event. We detect this case
    // by seeing the two key events directly after each other with a very
    // short time between them (<50ms) and supress the Ctrl event.
    if (altGrArmed) {
      bool altPressed = isExtended &&
                        (systemKeyCode == 0x38) &&
                        (vKey == VK_MENU) &&
                        ((msg->time - altGrCtrlTime) < 50);
      resolveAltGrDetection(altPressed);
    }

    if (systemKeyCode == SCAN_FAKE) {
      vlog.debug("Ignoring fake key press (virtual key 0x%02x)", vKey);
      return true;
    }

    // Windows sets the scan code to 0x00 for multimedia keys, so we
    // have to do a reverse lookup based on the vKey.
    if (systemKeyCode == 0x00) {
      systemKeyCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
      if (systemKeyCode == 0x00) {
        if (isExtended)
          vlog.error(_("No scan code for extended virtual key 0x%02x"), (int)vKey);
        else
          vlog.error(_("No scan code for virtual key 0x%02x"), (int)vKey);
        return true;
      }
    }

    if (systemKeyCode & ~0x7f) {
      vlog.error(_("Invalid scan code 0x%02x"), (int)systemKeyCode);
      return true;
    }

    if (isExtended)
      systemKeyCode |= 0x80;

    keyCode = translateSystemKeyCode(systemKeyCode);

    GetKeyboardState(state);

    // Pressing Ctrl wreaks havoc with the symbol lookup, so turn
    // that off. But AltGr shows up as Ctrl+Alt in Windows, so keep
    // Ctrl if Alt is active.
    if (!(state[VK_LCONTROL] & 0x80) || !(state[VK_RMENU] & 0x80))
      state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;

    keySym = translateVKey(vKey, isExtended, state);

    if (keySym == NoSymbol) {
      // Most Ctrl+Alt combinations will fail to produce a symbol, so
      // try it again with Ctrl unconditionally disabled.
      state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;
      keySym = translateVKey(vKey, isExtended, state);
    }

    if (keySym == NoSymbol) {
      if (isExtended)
        vlog.error(_("No symbol for extended virtual key 0x%02x"), (int)vKey);
      else
        vlog.error(_("No symbol for virtual key 0x%02x"), (int)vKey);
    }

    // Windows sends the same vKey for both shifts, so we need to look
    // at the scan code to tell them apart
    if ((keySym == XK_Shift_L) && (systemKeyCode == 0x36))
      keySym = XK_Shift_R;

    // AltGr handling (see above)
    if (hasAltGr()) {
      if ((systemKeyCode == 0xb8) && (keySym == XK_Alt_R))
        keySym = XK_ISO_Level3_Shift;

      // Possible start of AltGr sequence?
      if ((systemKeyCode == 0x1d) && (keySym == XK_Control_L)) {
        altGrArmed = true;
        altGrCtrlTime = msg->time;
        Fl::add_timeout(0.1, handleAltGrTimeout, this);
        return true;
      }
    }

    handler->handleKeyPress(systemKeyCode, keyCode, keySym);

    // We don't get reliable WM_KEYUP for these
    switch (keySym) {
    case XK_Zenkaku_Hankaku:
    case XK_Eisu_toggle:
    case XK_Katakana:
    case XK_Hiragana:
    case XK_Romaji:
      handler->handleKeyRelease(systemKeyCode);
    }

    // Shift key tracking, see below
    if (systemKeyCode == 0x2a)
        leftShiftDown = true;
    if (systemKeyCode == 0x36)
        rightShiftDown = true;

    return true;
  } else if ((msg->message == WM_KEYUP) || (msg->message == WM_SYSKEYUP)) {
    UINT vKey;
    bool isExtended;
    int systemKeyCode, keyCode;

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    systemKeyCode = ((msg->lParam >> 16) & 0xff);

    // Touch keyboard AltGr (see above)
    if (!isExtended && (systemKeyCode == 0x00) && (vKey == VK_MENU)) {
      isExtended = true;
      systemKeyCode = 0x38;
    }

    // We can't get a release in the middle of an AltGr sequence, so
    // abort that detection
    if (altGrArmed)
      resolveAltGrDetection(false);

    if (systemKeyCode == SCAN_FAKE) {
      vlog.debug("Ignoring fake key release (virtual key 0x%02x)", vKey);
      return 1;
    }

    if (systemKeyCode == 0x00)
      systemKeyCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    if (isExtended)
      systemKeyCode |= 0x80;

    keyCode = translateSystemKeyCode(systemKeyCode);

    handler->handleKeyRelease(keyCode);

    // Windows has a rather nasty bug where it won't send key release
    // events for a Shift button if the other Shift is still pressed
    if ((systemKeyCode == 0x2a) || (systemKeyCode == 0x36)) {
      if (leftShiftDown)
        handler->handleKeyRelease(0x2a);
      if (rightShiftDown)
        handler->handleKeyRelease(0x36);
      leftShiftDown = false;
      rightShiftDown = false;
    }

    return true;
  }

  return false;
}

std::list<uint32_t> KeyboardWin32::translateToKeySyms(int systemKeyCode)
{
  unsigned vkey;
  bool extended;

  std::list<uint32_t> keySyms;
  unsigned mods;

  BYTE state[256];

  uint32_t ks;

  UINT ch;

  extended = systemKeyCode & 0x80;
  if (extended)
    systemKeyCode = 0xe0 | (systemKeyCode & 0x7f);

  vkey = MapVirtualKey(systemKeyCode, MAPVK_VSC_TO_VK_EX);
  if (vkey == 0)
    return keySyms;

  // Start with no modifiers
  memset(state, 0, sizeof(state));
  ks = translateVKey(vkey, extended, state);
  if (ks != NoSymbol)
    keySyms.push_back(ks);

  // Next just a single modifier at a time
  for (mods = 1; mods < 16; mods <<= 1) {
    std::list<uint32_t>::const_iterator iter;

    memset(state, 0, sizeof(state));
    if (mods & 0x1)
      state[VK_CONTROL] = state[VK_LCONTROL] = 0x80;
    if (mods & 0x2)
      state[VK_SHIFT] = state[VK_LSHIFT] = 0x80;
    if (mods & 0x4)
      state[VK_MENU] = state[VK_LMENU] = 0x80;
    if (mods & 0x8) {
      state[VK_CONTROL] = state[VK_LCONTROL] = 0x80;
      state[VK_MENU] = state[VK_RMENU] = 0x80;
    }

    ks = translateVKey(vkey, extended, state);
    if (ks == NoSymbol)
      continue;

    iter = std::find(keySyms.begin(), keySyms.end(), ks);
    if (iter != keySyms.end())
      continue;

    keySyms.push_back(ks);
  }

  // Finally everything
  for (mods = 0; mods < 16; mods++) {
    std::list<uint32_t>::const_iterator iter;

    memset(state, 0, sizeof(state));
    if (mods & 0x1)
      state[VK_CONTROL] = state[VK_LCONTROL] = 0x80;
    if (mods & 0x2)
      state[VK_SHIFT] = state[VK_LSHIFT] = 0x80;
    if (mods & 0x4)
      state[VK_MENU] = state[VK_LMENU] = 0x80;
    if (mods & 0x8) {
      state[VK_CONTROL] = state[VK_LCONTROL] = 0x80;
      state[VK_MENU] = state[VK_RMENU] = 0x80;
    }

    ks = translateVKey(vkey, extended, state);
    if (ks == NoSymbol)
      continue;

    iter = std::find(keySyms.begin(), keySyms.end(), ks);
    if (iter != keySyms.end())
      continue;

    keySyms.push_back(ks);
  }

  // As a final resort we use MapVirtualKey() as that gives us a Latin
  // character even on non-Latin keyboards, which is useful for
  // shortcuts
  //
  // FIXME: Can this give us anything but ASCII?

  ch = MapVirtualKeyW(vkey, MAPVK_VK_TO_CHAR);
  if (ch != 0) {
    if (ch & 0x80000000)
      ch = ucs2combining(ch & 0xffff);
    else
      ch = ch & 0xffff;

    ks = ucs2keysym(ch);
    if (ks != NoSymbol) {
      std::list<uint32_t>::const_iterator iter;

      iter = std::find(keySyms.begin(), keySyms.end(), ks);
      if (iter == keySyms.end())
        keySyms.push_back(ks);
    }
  }

  return keySyms;
}

void KeyboardWin32::reset()
{
  altGrArmed = false;
  Fl::remove_timeout(handleAltGrTimeout, this);

  leftShiftDown = false;
  rightShiftDown = false;
}

unsigned KeyboardWin32::getLEDState()
{
  unsigned state;

  state = 0;

  if (GetKeyState(VK_CAPITAL) & 0x1)
    state |= rfb::ledCapsLock;
  if (GetKeyState(VK_NUMLOCK) & 0x1)
    state |= rfb::ledNumLock;
  if (GetKeyState(VK_SCROLL) & 0x1)
    state |= rfb::ledScrollLock;

  return state;
}

void KeyboardWin32::setLEDState(unsigned state)
{
  INPUT input[6];
  UINT count;
  UINT ret;

  memset(input, 0, sizeof(input));
  count = 0;

  if (!!(state & rfb::ledCapsLock) != !!(GetKeyState(VK_CAPITAL) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_CAPITAL;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = 0;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP;
    count += 2;
  }

  if (!!(state & rfb::ledNumLock) != !!(GetKeyState(VK_NUMLOCK) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_NUMLOCK;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    count += 2;
  }

  if (!!(state & rfb::ledScrollLock) != !!(GetKeyState(VK_SCROLL) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_SCROLL;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = 0;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP;
    count += 2;
  }

  if (count == 0)
    return;

  ret = SendInput(count, input, sizeof(*input));
  if (ret < count)
    vlog.error(_("Failed to update keyboard LED state: %lu"), GetLastError());
}

uint32_t KeyboardWin32::translateSystemKeyCode(int systemKeyCode)
{
  // Fortunately RFB and Windows use the same scan code set (mostly),
  // so there is no conversion needed
  // (as long as we encode the extended keys with the high bit)

  // However Pause sends a code that conflicts with NumLock, so use
  // the code most RFB implementations use (part of the sequence for
  // Ctrl+Pause, i.e. Break)
  if (systemKeyCode == 0x45)
    return 0xc6;

  // And NumLock incorrectly has the extended bit set
  if (systemKeyCode == 0xc5)
    return 0x45;

  // And Alt+PrintScreen (i.e. SysRq) sends a different code than
  // PrintScreen
  if (systemKeyCode == 0xb7)
    return 0x54;

  return systemKeyCode;
}

uint32_t KeyboardWin32::lookupVKeyMap(unsigned vkey, bool extended,
                                          const UINT map[][3], size_t size)
{
  size_t i;

  for (i = 0;i < size;i++) {
    if (vkey != map[i][0])
      continue;

    if (extended)
      return map[i][2];
    else
      return map[i][1];
  }

  return NoSymbol;
}

uint32_t KeyboardWin32::translateVKey(unsigned vkey, bool extended,
                                      const unsigned char state[256])
{
  HKL layout;
  WORD lang, primary_lang;

  int ret;
  WCHAR wstr[10];

  // Start with keys that either don't generate a symbol, or
  // generate the same symbol as some other key.

  ret = lookupVKeyMap(vkey, extended, vkey_map, ARRAY_SIZE(vkey_map));
  if (ret != NoSymbol)
    return ret;

  layout = GetKeyboardLayout(0);
  lang = LOWORD(layout);
  primary_lang = PRIMARYLANGID(lang);

  if (primary_lang == LANG_JAPANESE) {
    ret = lookupVKeyMap(vkey, extended,
                        vkey_map_jp, ARRAY_SIZE(vkey_map_jp));
    if (ret != NoSymbol)
      return ret;
  }

  if (primary_lang == LANG_KOREAN) {
    ret = lookupVKeyMap(vkey, extended,
                        vkey_map_ko, ARRAY_SIZE(vkey_map_ko));
    if (ret != NoSymbol)
      return ret;
  }

  // Windows is not consistent in which virtual key it uses for
  // the numpad decimal key, and this is not likely to be fixed:
  // http://blogs.msdn.com/michkap/archive/2006/09/13/752377.aspx
  //
  // To get X11 behaviour, we instead look at the text generated
  // by they key.
  if ((vkey == VK_DECIMAL) || (vkey == VK_SEPARATOR)) {
    UINT ch;

    ch = MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
    switch (ch) {
    case ',':
      return XK_KP_Separator;
    case '.':
      return XK_KP_Decimal;
    default:
      return NoSymbol;
    }
  }

  // MapVirtualKey() doesn't look at modifiers, so it is
  // insufficient for mapping most keys to a symbol. ToUnicode()
  // does what we want though. Unfortunately it keeps state, so
  // we have to be careful around dead characters.

  // FIXME: Multi character results, like U+0644 U+0627
  //        on Arabic layout
  ret = ToUnicode(vkey, 0, state, wstr, sizeof(wstr)/sizeof(wstr[0]), 0);

  if (ret == 1)
    return ucs2keysym(wstr[0]);

  if (ret == -1) {
    WCHAR dead_char;

    dead_char = wstr[0];

    // Need to clear out the state that the dead key has caused.
    // This is the recommended method by Microsoft's engineers:
    // http://blogs.msdn.com/b/michkap/archive/2007/10/27/5717859.aspx
    do {
      ret = ToUnicode(vkey, 0, state, wstr, sizeof(wstr)/sizeof(wstr[0]), 0);
    } while (ret < 0);

    // Dead keys are represented by their spacing equivalent
    // (or something similar depending on the layout)
    return ucs2keysym(ucs2combining(dead_char));
  }

  return NoSymbol;
}

bool KeyboardWin32::hasAltGr()
{
  BYTE origState[256];
  BYTE altGrState[256];

  if (currentLayout == GetKeyboardLayout(0))
    return cachedHasAltGr;

  // Save current keyboard state so we can get things sane again after
  // we're done
  if (!GetKeyboardState(origState))
    return 0;

  // We press Ctrl+Alt (Windows fake AltGr) and then test every key
  // to see if it produces a printable character. If so then we assume
  // AltGr is used in the current layout.

  cachedHasAltGr = false;

  memset(altGrState, 0, sizeof(altGrState));
  altGrState[VK_CONTROL] = 0x80;
  altGrState[VK_MENU] = 0x80;

  for (UINT vkey = 0;vkey <= 0xff;vkey++) {
    int ret;
    WCHAR wstr[10];

    // Need to skip this one as it is a bit magical and will trigger
    // a false positive
    if (vkey == VK_PACKET)
      continue;

    ret = ToUnicode(vkey, 0, altGrState, wstr,
                    sizeof(wstr)/sizeof(wstr[0]), 0);
    if (ret == 1) {
      cachedHasAltGr = true;
      break;
    }

    if (ret == -1) {
      // Dead key, need to clear out state before we proceed
      do {
        ret = ToUnicode(vkey, 0, altGrState, wstr,
                        sizeof(wstr)/sizeof(wstr[0]), 0);
      } while (ret < 0);
    }
  }

  SetKeyboardState(origState);

  currentLayout = GetKeyboardLayout(0);

  return cachedHasAltGr;
}

void KeyboardWin32::handleAltGrTimeout(void *data)
{
  KeyboardWin32 *self = (KeyboardWin32 *)data;

  assert(self);

  self->altGrArmed = false;
  self->handler->handleKeyPress(0x1d, 0x1d, XK_Control_L);
}

void KeyboardWin32::resolveAltGrDetection(bool isAltGrSequence)
{
  altGrArmed = false;
  Fl::remove_timeout(handleAltGrTimeout);
  // when it's not an AltGr sequence we can't supress the Ctrl anymore
  if (!isAltGrSequence)
    handler->handleKeyPress(0x1d, 0x1d, XK_Control_L);
}
