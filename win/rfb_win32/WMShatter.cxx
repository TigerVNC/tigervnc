/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

// -=- WMShatter.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/WMShatter.h>

#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMShatter");

bool
rfb::win32::IsSafeWM(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
  bool result = true;
  switch (msg) {
    // - UNSAFE MESSAGES
  case WM_TIMER:
    result = lParam == 0;
    break;
  };
  if (!result) {
    vlog.info("IsSafeWM: 0x%p received 0x%x(%I64u, %I64u) - not safe",
              window, msg, (long long)wParam, (long long)lParam);
  }
  return result;
}

LRESULT
rfb::win32::SafeDefWindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (IsSafeWM(window, msg, wParam, lParam))
    return DefWindowProc(window, msg, wParam, lParam);
  return 0;
}

LRESULT
rfb::win32::SafeDispatchMessage(const MSG* msg) {
  if (IsSafeWM(msg->hwnd, msg->message, msg->wParam, msg->lParam))
    return DispatchMessage(msg);
  return 0;
}
