/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

#include <windows.h>
#include <lmcons.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/OSVersion.h>

using namespace rfb;
using namespace win32;


CurrentUserToken::CurrentUserToken() : isValid_(false) {
  // - If the OS doesn't support security, ignore it
  if (!isServiceProcess()) {
    // - Running in User-Mode - just get our current token
    if (!OpenProcessToken(GetCurrentProcess(), GENERIC_ALL, &h)) {
      DWORD err = GetLastError();
      if (err != ERROR_CALL_NOT_IMPLEMENTED)
       throw rdr::SystemException("OpenProcessToken failed", GetLastError());
    }
    isValid_ = true;
  } else {
    // - Under XP/2003 and above, we can just ask the operating system
    typedef BOOL (WINAPI *WTSQueryUserToken_proto)(ULONG, PHANDLE);
    DynamicFn<WTSQueryUserToken_proto> _WTSQueryUserToken(_T("wtsapi32.dll"), "WTSQueryUserToken");
    if (_WTSQueryUserToken.isValid()) {
      (*_WTSQueryUserToken)(-1, &h);
      isValid_ = true;
    } else {
      // - Under NT/2K we have to resort to a nasty hack...
      HWND tray = FindWindow(_T("Shell_TrayWnd"), 0);
      if (!tray)
        return;
      DWORD processId = 0;
      GetWindowThreadProcessId(tray, &processId);
      if (!processId)
        return;
      Handle process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
      if (!process.h)
        return;
      OpenProcessToken(process, MAXIMUM_ALLOWED, &h);
      isValid_ = true;
    }
  }
}


ImpersonateCurrentUser::ImpersonateCurrentUser() {
  RegCloseKey(HKEY_CURRENT_USER);
  if (!isServiceProcess())
    return;
  if (!token.isValid())
    throw rdr::Exception("CurrentUserToken is not valid");
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
}


UserName::UserName() : TCharArray(UNLEN+1) {
  DWORD len = UNLEN+1;
  if (!GetUserName(buf, &len))
    throw rdr::SystemException("GetUserName failed", GetLastError());
}
