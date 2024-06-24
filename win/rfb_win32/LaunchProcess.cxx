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

// -=- LaunchProcess.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/LaunchProcess.h>
#include <rfb_win32/ModuleFileName.h>
#include <rfb_win32/Win32Util.h>
#include <rdr/Exception.h>
#include <stdio.h>

using namespace rfb;
using namespace win32;


LaunchProcess::LaunchProcess(const char* exeName_, const char* params_)
: exeName(exeName_), params(params_) {
  memset(&procInfo, 0, sizeof(procInfo));
}

LaunchProcess::~LaunchProcess() {
  await();
}


void LaunchProcess::start(HANDLE userToken, bool createConsole) {
  if (procInfo.hProcess && (WaitForSingleObject(procInfo.hProcess, 0) != WAIT_OBJECT_0))
    return;
  await();
  returnCode = STILL_ACTIVE;

  DWORD size;
  char desktopName[256];
  char buf[256];
  HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
  if (!GetUserObjectInformation(desktop, UOI_NAME, buf, 256, &size))
    throw rdr::SystemException("unable to launch process", GetLastError());

  snprintf(desktopName, 256, "WinSta0\\%s", buf);

  // - Create storage for the process startup information
  STARTUPINFO sinfo;
  memset(&sinfo, 0, sizeof(sinfo));
  sinfo.cb = sizeof(sinfo);
  sinfo.lpDesktop = desktopName;

  // - Concoct a suitable command-line
  std::string exePath;
  if (exeName.find('\\') == std::string::npos) {
    ModuleFileName filename;
    std::string path(filename.buf);
    if (path.rfind('\\') != std::string::npos)
      path.resize(path.rfind('\\'));
    exePath = path + '\\' + exeName;
  } else {
    exePath = exeName;
  }

  // - Start the process
  // Note: We specify the exe's precise path in the ApplicationName parameter,
  //       AND include the name as the first part of the CommandLine parameter,
  //       because CreateProcess doesn't make ApplicationName argv[0] in C programs.
  std::string cmdLine;
  cmdLine = (std::string)"\"" + exeName + "\" " + params;
  DWORD flags = createConsole ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;
  BOOL success;
  if (userToken != INVALID_HANDLE_VALUE)
    success = CreateProcessAsUser(userToken, exePath.c_str(),
                                  (char*)cmdLine.c_str(),
                                  nullptr, nullptr, FALSE,
                                  flags, nullptr, nullptr,
                                  &sinfo, &procInfo);
  else
    success = CreateProcess(exePath.c_str(), (char*)cmdLine.c_str(),
                            nullptr, nullptr, FALSE,
                            flags, nullptr, nullptr,
                            &sinfo, &procInfo);
  if (!success)
    throw rdr::SystemException("unable to launch process", GetLastError());

  // Wait for it to finish initialising
  WaitForInputIdle(procInfo.hProcess, 15000);
}

void LaunchProcess::detach()
{
  if (!procInfo.hProcess)
    return;
  CloseHandle(procInfo.hProcess);
  CloseHandle(procInfo.hThread);
  memset(&procInfo, 0, sizeof(procInfo));
}

bool LaunchProcess::await(DWORD timeoutMs) {
  if (!procInfo.hProcess)
    return true;
  DWORD result = WaitForSingleObject(procInfo.hProcess, timeoutMs);
  if (result == WAIT_OBJECT_0) {
    GetExitCodeProcess(procInfo.hProcess, &returnCode);
    detach();
    return true;
  } else if (result == WAIT_FAILED) {
    throw rdr::SystemException("await() failed", GetLastError());
  }
  return false;
}
