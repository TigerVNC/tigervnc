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

// -=- WinVNC Version 4.0 Service-Mode implementation

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winvnc/VNCServerService.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/TsSessions.h>
#include <rfb_win32/ModuleFileName.h>
#include <windows.h>
#include <wtsapi32.h>
#include <tlhelp32.h>

using namespace winvnc;
using namespace rfb;
using namespace win32;

const TCHAR* winvnc::VNCServerService::Name = _T("TigerVNC");

// SendSAS is not available until Windows 7, and missing from MinGW
static HMODULE sasLibrary = NULL;
typedef void WINAPI (*SendSAS_proto)(BOOL AsUser);
static SendSAS_proto _SendSAS = NULL;

VNCServerService::VNCServerService()
  : Service(Name)
  , stopServiceEvent(CreateEvent(0, FALSE, FALSE, 0))
  , sessionEvent(CreateEvent(0, FALSE, FALSE, "Global\\SessionEventTigerVNC"))
  , sessionEventCad(CreateEvent(0, FALSE, FALSE, "Global\\SessionEventTigerVNCCad")) {
  if (sasLibrary == NULL) {
    sasLibrary = LoadLibrary("sas.dll");
    if (sasLibrary != NULL)
      _SendSAS = (SendSAS_proto)GetProcAddress(sasLibrary, "SendSAS");
  }
  // - Set the service-mode logging defaults
  //   These will be overridden by the Log option in the
  //   registry, if present.
  logParams.setParam("*:EventLog:0,Connections:EventLog:100");
}


//////////////////////////////////////////////////////////////////////////////

DWORD GetLogonPid(DWORD dwSessionId)
{
    DWORD dwLogonPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof procEntry;

        if (Process32First(hSnap, &procEntry)) do
        {
            DWORD dwLogonSessionId = 0;
            if (_stricmp(procEntry.szExeFile, "winlogon.exe") == 0 &&
                ProcessIdToSessionId(procEntry.th32ProcessID, &dwLogonSessionId) &&
                dwLogonSessionId == dwSessionId)
            {
                dwLogonPid = procEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnap, &procEntry));
        CloseHandle(hSnap);
    }
    return dwLogonPid;
}

//////////////////////////////////////////////////////////////////////////////
BOOL GetSessionUserTokenWin(OUT LPHANDLE lphUserToken)
{
    BOOL bResult = FALSE;
    ConsoleSessionId ID_session;
    DWORD Id = GetLogonPid(ID_session.id);
    if (HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Id))
    {
        bResult = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, lphUserToken);
        CloseHandle(hProcess);
    }
    return bResult;
}

//////////////////////////////////////////////////////////////////////////////
// START the app as system 
HANDLE LaunchProcessWin(DWORD dwSessionId)
{
    HANDLE hProcess = NULL;
    HANDLE hToken = NULL;
    if (GetSessionUserTokenWin(&hToken))
    {
        ModuleFileName filename;
        TCharArray cmdLine;
        cmdLine.format("\"%s\" -noconsole -service_run", filename.buf);
        STARTUPINFO si;
        ZeroMemory(&si, sizeof si);
        si.cb = sizeof si;
        si.dwFlags = STARTF_USESHOWWINDOW;
        PROCESS_INFORMATION	pi;
        if (CreateProcessAsUser(hToken, NULL, cmdLine.buf, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hThread);
            hProcess = pi.hProcess;
        }
        CloseHandle(hToken);
    }
    return hProcess;
}

DWORD VNCServerService::serviceMain(int argc, TCHAR* argv[])
{
    ConsoleSessionId OlddwSessionId;

    HANDLE hProcess = NULL;
    //We use this event to notify the program that the session has changed
    //The program need to end so the service can restart the program in the correct session
    //wait_for_existing_process();
    HANDLE testevent[2] = { stopServiceEvent, sessionEventCad };
    setStatus(SERVICE_RUNNING);
    while (status.dwCurrentState == SERVICE_RUNNING)
    {
        DWORD dwEvent = WaitForMultipleObjects(2, testevent, FALSE, 1000);
        switch (dwEvent) 
        { 
        //stopServiceEvent, exit while loop
        case WAIT_OBJECT_0 + 0: 
            setStatus(SERVICE_STOP_PENDING);
            break; 

        //cad request
        case WAIT_OBJECT_0 + 1:
            if (_SendSAS != NULL)
                _SendSAS(FALSE);
            break; 

        case WAIT_TIMEOUT:
            {
                ConsoleSessionId dwSessionId;
                if (OlddwSessionId.id != dwSessionId.id)
                {
                    OlddwSessionId.id = dwSessionId.id;
                    SetEvent(sessionEvent);
                }
                DWORD dwExitCode = 0;
                if (hProcess == NULL ||
                    (GetExitCodeProcess(hProcess, &dwExitCode) &&
                    dwExitCode != STILL_ACTIVE &&
                    CloseHandle(hProcess)))
                {
                    hProcess = LaunchProcessWin(dwSessionId.id);
                }
            }
            break;
        }
    }

    SetEvent(sessionEvent);

    if (hProcess)
    {
        WaitForSingleObject(hProcess, 15000);
        CloseHandle(hProcess);
    }
    return 0;
}

void VNCServerService::stop() {
  SetEvent(stopServiceEvent);
  SetEvent(sessionEvent);
}
