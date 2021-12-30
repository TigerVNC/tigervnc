/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#include <stdio.h>

#define XK_MISCELLANY
#define XK_XKB_KEYS
#define XK_KOREAN
#include <rfb/keysymdef.h>
#include <rfb/XF86keysym.h>

#include "keysym2ucs.h"

#define NoSymbol 0

// Missing in at least some versions of MinGW
#ifndef MAPVK_VK_TO_CHAR
#define MAPVK_VK_TO_CHAR 2
#endif

int has_altgr;
HKL current_layout = 0;

static HANDLE thread;
static DWORD thread_id;

static HHOOK hook = 0;
static HWND target_wnd = 0;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

static int is_system_hotkey(int vkCode) {
  switch (vkCode) {
  case VK_LWIN:
  case VK_RWIN:
  case VK_SNAPSHOT:
    return 1;
  case VK_TAB:
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
      return 1;
  case VK_ESCAPE:
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
      return 1;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
      return 1;
  }
  return 0;
}

static LRESULT CALLBACK keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0) {
    KBDLLHOOKSTRUCT* msgInfo = (KBDLLHOOKSTRUCT*)lParam;

    // Grabbing everything seems to mess up some keyboard state that
    // FLTK relies on, so just grab the keys that we normally cannot.
    if (is_system_hotkey(msgInfo->vkCode)) {
      PostMessage(target_wnd, wParam, msgInfo->vkCode,
                  (msgInfo->scanCode & 0xff) << 16 |
                  (msgInfo->flags & 0xff) << 24);
      return 1;
    }
  }

  return CallNextHookEx(hook, nCode, wParam, lParam);
}

static DWORD WINAPI keyboard_thread(LPVOID data)
{
  MSG msg;

  target_wnd = (HWND)data;

  // Make sure a message queue is created
  PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD);

  hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, GetModuleHandle(0), 0);
  // If something goes wrong then there is not much we can do.
  // Just sit around and wait for WM_QUIT...

  while (GetMessage(&msg, NULL, 0, 0));

  if (hook)
    UnhookWindowsHookEx(hook);

  target_wnd = 0;

  return 0;
}

int win32_enable_lowlevel_keyboard(HWND hwnd)
{
  // Only one target at a time for now
  if (thread != NULL) {
    if (hwnd == target_wnd)
      return 0;

    return 1;
  }

  // We create a separate thread as it is crucial that hooks are processed
  // in a timely manner.
  thread = CreateThread(NULL, 0, keyboard_thread, hwnd, 0, &thread_id);
  if (thread == NULL)
    return 1;

  return 0;
}

void win32_disable_lowlevel_keyboard(HWND hwnd)
{
  if (hwnd != target_wnd)
    return;

  PostThreadMessage(thread_id, WM_QUIT, 0, 0);

  CloseHandle(thread);
  thread = NULL;
}

// Layout independent keys
static const int vkey_map[][3] = {
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
  { VK_LAUNCH_APP2,         NoSymbol,       XF86XK_Calculator },
};

// Layout dependent keys, but without useful symbols

// Japanese
static const int vkey_map_jp[][3] = {
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
static const int vkey_map_ko[][3] = {
  { VK_HANGUL,              XK_Hangul,      NoSymbol },
  { VK_HANJA,               XK_Hangul_Hanja, NoSymbol },
};

static int lookup_vkey_map(UINT vkey, int extended, const int map[][3], size_t size)
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

int win32_vkey_to_keysym(UINT vkey, int extended)
{
  HKL layout;
  WORD lang, primary_lang;

  BYTE state[256];
  int ret;
  WCHAR wstr[10];

  // Start with keys that either don't generate a symbol, or
  // generate the same symbol as some other key.

  ret = lookup_vkey_map(vkey, extended, vkey_map, ARRAY_SIZE(vkey_map));
  if (ret != NoSymbol)
    return ret;

  layout = GetKeyboardLayout(0);
  lang = LOWORD(layout);
  primary_lang = PRIMARYLANGID(lang);

  if (primary_lang == LANG_JAPANESE) {
    ret = lookup_vkey_map(vkey, extended,
                          vkey_map_jp, ARRAY_SIZE(vkey_map_jp));
    if (ret != NoSymbol)
      return ret;
  }

  if (primary_lang == LANG_KOREAN) {
    ret = lookup_vkey_map(vkey, extended,
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

  GetKeyboardState(state);

  // Pressing Ctrl wreaks havoc with the symbol lookup, so turn
  // that off. But AltGr shows up as Ctrl+Alt in Windows, so keep
  // Ctrl if Alt is active.
  if (!(state[VK_LCONTROL] & 0x80) || !(state[VK_RMENU] & 0x80))
    state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;

  // FIXME: Multi character results, like U+0644 U+0627
  //        on Arabic layout
  ret = ToUnicode(vkey, 0, state, wstr, sizeof(wstr)/sizeof(wstr[0]), 0);

  if (ret == 0) {
    // Most Ctrl+Alt combinations will fail to produce a symbol, so
    // try it again with Ctrl unconditionally disabled.
    state[VK_CONTROL] = state[VK_LCONTROL] = state[VK_RCONTROL] = 0;
    ret = ToUnicode(vkey, 0, state, wstr, sizeof(wstr)/sizeof(wstr[0]), 0);
  }

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

int win32_has_altgr(void)
{
  BYTE orig_state[256];
  BYTE altgr_state[256];

  if (current_layout == GetKeyboardLayout(0))
    return has_altgr;

  // Save current keyboard state so we can get things sane again after
  // we're done
  if (!GetKeyboardState(orig_state))
    return 0;

  // We press Ctrl+Alt (Windows fake AltGr) and then test every key
  // to see if it produces a printable character. If so then we assume
  // AltGr is used in the current layout.

  has_altgr = 0;

  memset(altgr_state, 0, sizeof(altgr_state));
  altgr_state[VK_CONTROL] = 0x80;
  altgr_state[VK_MENU] = 0x80;

  for (UINT vkey = 0;vkey <= 0xff;vkey++) {
    int ret;
    WCHAR wstr[10];

    // Need to skip this one as it is a bit magical and will trigger
    // a false positive
    if (vkey == VK_PACKET)
      continue;

    ret = ToUnicode(vkey, 0, altgr_state, wstr,
                    sizeof(wstr)/sizeof(wstr[0]), 0);
    if (ret == 1) {
      has_altgr = 1;
      break;
    }

    if (ret == -1) {
      // Dead key, need to clear out state before we proceed
      do {
        ret = ToUnicode(vkey, 0, altgr_state, wstr,
                        sizeof(wstr)/sizeof(wstr[0]), 0);
      } while (ret < 0);
    }
  }

  SetKeyboardState(orig_state);

  current_layout = GetKeyboardLayout(0);

  return has_altgr;
}
