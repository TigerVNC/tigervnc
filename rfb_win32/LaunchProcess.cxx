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

// -=- LaunchProcess.cxx

#include <rfb_win32/LaunchProcess.h>
#include <rfb_win32/Win32Util.h>

#include <rfb/util.h>

using namespace rfb;
using namespace win32;


LaunchProcess::LaunchProcess(const TCHAR* exeName_, const TCHAR* params_)
: exeName(tstrDup(exeName_)), params(tstrDup(params_)) {
  memset(&procInfo, 0, sizeof(procInfo));
}

LaunchProcess::~LaunchProcess() {
  await();
}


void LaunchProcess::start(HANDLE userToken) {
  if (procInfo.hProcess && (WaitForSingleObject(procInfo.hProcess, 0) != WAIT_OBJECT_0))
    return;
  await();

  // - Create storage for the process startup information
  STARTUPINFO sinfo;
  memset(&sinfo, 0, sizeof(sinfo));
  sinfo.cb = sizeof(sinfo);

  // - Concoct a suitable command-line
  TCharArray exePath;
  if (!tstrContains(exeName.buf, _T('\\'))) {
    ModuleFileName filename;
    TCharArray path; splitPath(filename.buf, &path.buf, 0);
    exePath.buf = new TCHAR[_tcslen(path.buf) + _tcslen(exeName.buf) + 2];
    _stprintf(exePath.buf, _T("%s\\%s"), path.buf, exeName.buf);
  } else {
    exePath.buf = tstrDup(exeName.buf);
  }

  // - Start the VNC server
  // Note: We specify the exe's precise path in the ApplicationName parameter,
  //       AND include the name as the first part of the CommandLine parameter,
  //       because CreateProcess doesn't make ApplicationName argv[0] in C programs.
  TCharArray cmdLine(_tcslen(exeName.buf) + 3 + _tcslen(params.buf) + 1);
  _stprintf(cmdLine.buf, _T("\"%s\" %s"), exeName.buf, params.buf);
#ifdef _DEBUG
  DWORD flags = CREATE_NEW_CONSOLE;
#else
  DWORD flags = CREATE_NO_WINDOW;
#endif
  BOOL success;
  if (userToken)
    success = CreateProcessAsUser(userToken, exePath.buf, cmdLine.buf, 0, 0, FALSE, flags, 0, 0, &sinfo, &procInfo);
  else
    success = CreateProcess(exePath.buf, cmdLine.buf, 0, 0, FALSE, flags, 0, 0, &sinfo, &procInfo);
  if (!success)
    throw rdr::SystemException("unable to launch process", GetLastError());

  // Wait for it to finish initialising
  WaitForInputIdle(procInfo.hProcess, 15000);
}

void LaunchProcess::await() {
  if (!procInfo.hProcess)
    return;
  WaitForSingleObject(procInfo.hProcess, INFINITE);
  CloseHandle(procInfo.hProcess);
  CloseHandle(procInfo.hThread);
  memset(&procInfo, 0, sizeof(procInfo));
}

