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

// -=- Currentuser.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Service.h>
#include <lmcons.h>
#include <wtsapi32.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("CurrentUser");


const TCHAR* shellIconClass = _T("Shell_TrayWnd");

BOOL CALLBACK enumWindows(HWND hwnd, LPARAM lParam) {
  TCHAR className[16];
  if (GetClassName(hwnd, className, sizeof(className)) &&
      (_tcscmp(className, shellIconClass) == 0)) {
    vlog.debug("located tray icon window (%s)", (const char*)CStr(className));
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (!processId)
      return TRUE;
    Handle process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
    if (!process.h)
      return TRUE;
    if (!OpenProcessToken(process, MAXIMUM_ALLOWED, (HANDLE*)lParam))
      return TRUE;
    vlog.debug("obtained user token");
    return FALSE;
  }
  return TRUE;
}

BOOL CALLBACK enumDesktops(LPTSTR lpszDesktop, LPARAM lParam) {
  HDESK desktop = OpenDesktop(lpszDesktop, 0, FALSE, DESKTOP_ENUMERATE);
  vlog.debug("opening \"%s\"", lpszDesktop);
  if (!desktop) {
    vlog.info("desktop \"%s\" inaccessible", (const char*)CStr(lpszDesktop));
    return TRUE;
  }
  BOOL result = EnumDesktopWindows(desktop, enumWindows, lParam);
  if (!CloseDesktop(desktop))
    vlog.info("unable to close desktop: %ld", GetLastError());
  return result;
}


CurrentUserToken::CurrentUserToken() {
  if (isServiceProcess()) {
    // Try to get the user token using the Terminal Services APIs
    WTSQueryUserToken(-1, &h);
  } else {
    // Try to open the security token for the User-Mode process
    if (!OpenProcessToken(GetCurrentProcess(), GENERIC_ALL, &h)) {
      DWORD err = GetLastError();
      if (err != ERROR_CALL_NOT_IMPLEMENTED)
        throw rdr::SystemException("OpenProcessToken failed", err);
      h = INVALID_HANDLE_VALUE;
    }
  }
}


ImpersonateCurrentUser::ImpersonateCurrentUser() {
  RegCloseKey(HKEY_CURRENT_USER);
  if (!isServiceProcess())
    return;
  if (!token.canImpersonate())
    throw rdr::Exception("Cannot impersonate unsafe or null token");
  if (!ImpersonateLoggedOnUser(token)) {
    DWORD err = GetLastError();
    if (err != ERROR_CALL_NOT_IMPLEMENTED)
      throw rdr::SystemException("Failed to impersonate user", GetLastError());
  }
}

ImpersonateCurrentUser::~ImpersonateCurrentUser() {
  if (!RevertToSelf()) {
    DWORD err = GetLastError();
    if (err != ERROR_CALL_NOT_IMPLEMENTED)
      exit(err);
  }
  RegCloseKey(HKEY_CURRENT_USER);
}


UserName::UserName() : TCharArray(UNLEN+1) {
  DWORD len = UNLEN+1;
  if (!GetUserName(buf, &len))
    throw rdr::SystemException("GetUserName failed", GetLastError());
}


UserSID::UserSID() {
  CurrentUserToken token;
  if (!token.canImpersonate())
    return;
  setSID(Sid::FromToken(token.h));
}
