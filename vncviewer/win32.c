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

#include "win32.h"

static HANDLE thread;
static DWORD thread_id;

static HHOOK kbd_hook = 0;
static HHOOK msg_hook = 0;
static BYTE kbd_state[256];
static HWND target_wnd = 0;

static LRESULT CALLBACK keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0) {
    KBDLLHOOKSTRUCT* msgInfo = (KBDLLHOOKSTRUCT*)lParam;

    BYTE vkey;
    BYTE scanCode;
    BYTE flags;

    vkey = msgInfo->vkCode;
    scanCode = msgInfo->scanCode;
    flags = msgInfo->flags;

    // If the key was pressed before the grab was activated, then we
    // need to avoid intercepting the release event or Windows will get
    // confused about the state of the key
    if (((wParam == WM_KEYUP) || (wParam == WM_SYSKEYUP)) &&
        (kbd_state[msgInfo->vkCode] & 0x80)) {
      kbd_state[msgInfo->vkCode] &= ~0x80;
    } else {
      PostMessage(target_wnd, wParam, vkey,
                  scanCode << 16 | flags << 24);
      return 1;
    }
  }

  return CallNextHookEx(kbd_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK message_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0) {
    MSG* msg = (MSG*)lParam;

    if ((msg->message == WM_KEYDOWN) ||
        (msg->message == WM_SYSKEYDOWN) ||
        (msg->message == WM_KEYUP) ||
        (msg->message == WM_SYSKEYUP)) {
      int down;
      BYTE state[256];

      // Windows stops updating our local keyboard state when we do a
      // low-level intercept of events, so we need to do that manually

      GetKeyboardState(state);

      // First the key itself
      down = !(msg->lParam & (1<<31));
      if (down)
        state[msg->wParam] |= 0x80;
      else
        state[msg->wParam] &= ~0x80;

      // Then the combined fake vkeys for the modifiers

      if ((state[VK_LSHIFT] & 0x80) || (state[VK_RSHIFT] & 0x80))
        state[VK_SHIFT] |= 0x80;
      else
        state[VK_SHIFT] &= ~0x80;

      if ((state[VK_LCONTROL] & 0x80) || (state[VK_RCONTROL] & 0x80))
        state[VK_CONTROL] |= 0x80;
      else
        state[VK_CONTROL] &= ~0x80;

      if ((state[VK_LMENU] & 0x80) || (state[VK_RMENU] & 0x80))
        state[VK_MENU] |= 0x80;
      else
        state[VK_MENU] &= ~0x80;

      SetKeyboardState(state);

      // We get the low level vkeys here, but the application code
      // expects this to have been translated to the generic ones
      switch (msg->wParam) {
      case VK_LSHIFT:
      case VK_RSHIFT:
        msg->wParam = VK_SHIFT;
        // The extended bit is also always missing for right shift
        msg->lParam &= ~(1 << 24);
        break;
      case VK_LCONTROL:
      case VK_RCONTROL:
        msg->wParam = VK_CONTROL;
        break;
      case VK_LMENU:
      case VK_RMENU:
        msg->wParam = VK_MENU;
        break;
      }
    }
  }

  return CallNextHookEx(msg_hook, nCode, wParam, lParam);
}

static DWORD WINAPI keyboard_thread(LPVOID data)
{
  MSG msg;

  target_wnd = (HWND)data;

  // Make sure a message queue is created
  PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE | PM_NOYIELD);

  // We need to know which keys are currently pressed
  GetKeyboardState(kbd_state);

  kbd_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, GetModuleHandle(0), 0);

  // If something goes wrong then there is not much we can do.
  // Just sit around and wait for WM_QUIT...

  while (GetMessage(&msg, NULL, 0, 0));

  if (kbd_hook)
    UnhookWindowsHookEx(kbd_hook);

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

  msg_hook = SetWindowsHookEx(WH_GETMESSAGE, message_hook, 0, GetCurrentThreadId());
  if (msg_hook == NULL) {
    win32_disable_lowlevel_keyboard(hwnd);
    return 1;
  }

  return 0;
}

void win32_disable_lowlevel_keyboard(HWND hwnd)
{
  if (hwnd != target_wnd)
    return;

  PostThreadMessage(thread_id, WM_QUIT, 0, 0);

  CloseHandle(thread);
  thread = NULL;

  if (msg_hook)
    UnhookWindowsHookEx(msg_hook);
  msg_hook = NULL;
}
