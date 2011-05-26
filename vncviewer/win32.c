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

#include <windows.h>

static HANDLE thread;
static DWORD thread_id;

static HHOOK hook = 0;
static HWND target_wnd = 0;

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
