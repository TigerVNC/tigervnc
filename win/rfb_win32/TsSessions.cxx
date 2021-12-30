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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/TsSessions.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>
#include <tchar.h>
#include <wtsapi32.h>

static rfb::LogWriter vlog("TsSessions");

namespace rfb {
namespace win32 {

  ProcessSessionId::ProcessSessionId(DWORD processId) {
    id = 0;
    if (processId == (DWORD)-1)
      processId = GetCurrentProcessId();
    if (!ProcessIdToSessionId(GetCurrentProcessId(), &id))
      throw rdr::SystemException("ProcessIdToSessionId", GetLastError());
  }

  ProcessSessionId mySessionId;

  ConsoleSessionId::ConsoleSessionId() {
    id = WTSGetActiveConsoleSessionId();
  }

  bool inConsoleSession() {
    ConsoleSessionId console;
    return console.id == mySessionId.id;
  }

  void setConsoleSession(DWORD sessionId) {
    if (sessionId == (DWORD)-1)
      sessionId = mySessionId.id;

    // Try to reconnect our session to the console
    ConsoleSessionId console;
    vlog.info("Console session is %lu", console.id);
    if (!WTSConnectSession(sessionId, console.id, (PTSTR)_T(""), 0))
      throw rdr::SystemException("Unable to connect session to Console", GetLastError());

    // Lock the newly connected session, for security
    LockWorkStation();
  }

};
};
