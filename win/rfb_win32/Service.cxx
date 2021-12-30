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

// -=- Service.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/Service.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/ModuleFileName.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/Handle.h>
#include <logmessages/messages.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>


using namespace rdr;
using namespace rfb;
using namespace win32;

static LogWriter vlog("Service");


// - Internal service implementation functions

Service* service = 0;
bool runAsService = false;

VOID WINAPI serviceHandler(DWORD control) {
  switch (control) {
  case SERVICE_CONTROL_INTERROGATE:
    vlog.info("cmd: report status");
    service->setStatus();
    return;
  case SERVICE_CONTROL_PARAMCHANGE:
    vlog.info("cmd: param change");
    service->readParams();
    return;
  case SERVICE_CONTROL_SHUTDOWN:
    vlog.info("cmd: OS shutdown");
    service->osShuttingDown();
    return;
  case SERVICE_CONTROL_STOP:
    vlog.info("cmd: stop");
    service->setStatus(SERVICE_STOP_PENDING);
    service->stop();
    return;
  };
  vlog.debug("cmd: unknown %lu", control);
}


// -=- Service main procedure

VOID WINAPI serviceProc(DWORD dwArgc, LPTSTR* lpszArgv) {
  vlog.debug("entering %s serviceProc", service->getName());
  vlog.info("registering handler...");
  service->status_handle = RegisterServiceCtrlHandler(service->getName(), serviceHandler);
  if (!service->status_handle) {
    DWORD err = GetLastError();
    vlog.error("failed to register handler: %lu", err);
    ExitProcess(err);
  }
  vlog.debug("registered handler (%p)", service->status_handle);
  service->setStatus(SERVICE_START_PENDING);
  vlog.debug("entering %s serviceMain", service->getName());
  service->status.dwWin32ExitCode = service->serviceMain(dwArgc, lpszArgv);
  vlog.debug("leaving %s serviceMain", service->getName());
  service->setStatus(SERVICE_STOPPED);
}


// -=- Service

Service::Service(const TCHAR* name_) : name(name_) {
  vlog.debug("Service");
  status_handle = 0;
  status.dwControlsAccepted = SERVICE_CONTROL_INTERROGATE | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwServiceSpecificExitCode = 0;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 30000;
  status.dwCurrentState = SERVICE_STOPPED;
}

void
Service::start() {
  SERVICE_TABLE_ENTRY entry[2];
  entry[0].lpServiceName = (TCHAR*)name;
  entry[0].lpServiceProc = serviceProc;
  entry[1].lpServiceName = NULL;
  entry[1].lpServiceProc = NULL;
  vlog.debug("entering dispatcher");
  if (!SetProcessShutdownParameters(0x100, 0))
    vlog.error("unable to set shutdown parameters: %lu", GetLastError());
  service = this;
  if (!StartServiceCtrlDispatcher(entry))
    throw SystemException("unable to start service", GetLastError());
}

void
Service::setStatus() {
  setStatus(status.dwCurrentState);
}

void
Service::setStatus(DWORD state) {
  if (status_handle == 0) {
    vlog.debug("warning - cannot setStatus");
    return;
  }
  status.dwCurrentState = state;
  status.dwCheckPoint++;
  if (!SetServiceStatus(status_handle, &status)) {
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwWin32ExitCode = GetLastError();
    vlog.error("unable to set service status:%lu", status.dwWin32ExitCode);
  }
  vlog.debug("set status to %lu(%lu)", state, status.dwCheckPoint);
}

Service::~Service() {
  vlog.debug("~Service");
  service = 0;
}


// Find out whether this process is running as the WinVNC service
bool thisIsService() {
  return service && (service->status.dwCurrentState != SERVICE_STOPPED);
}


// -=- Desktop handling code

// Switch the current thread to the specified desktop
static bool
switchToDesktop(HDESK desktop) {
  HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
  if (!SetThreadDesktop(desktop)) {
    vlog.debug("switchToDesktop failed:%lu", GetLastError());
    return false;
  }
  if (!CloseDesktop(old_desktop))
    vlog.debug("unable to close old desktop:%lu", GetLastError());
  return true;
}

// Determine whether the thread's current desktop is the input one
static bool
inputDesktopSelected() {
  HDESK current = GetThreadDesktop(GetCurrentThreadId());
	HDESK input = OpenInputDesktop(0, FALSE,
  	DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!input) {
    vlog.debug("unable to OpenInputDesktop(1):%lu", GetLastError());
    return false;
  }

  DWORD size;
  char currentname[256];
  char inputname[256];

  if (!GetUserObjectInformation(current, UOI_NAME, currentname, 256, &size)) {
    vlog.debug("unable to GetUserObjectInformation(1):%lu", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!GetUserObjectInformation(input, UOI_NAME, inputname, 256, &size)) {
    vlog.debug("unable to GetUserObjectInformation(2):%lu", GetLastError());
    CloseDesktop(input);
    return false;
  }
  if (!CloseDesktop(input))
    vlog.debug("unable to close input desktop:%lu", GetLastError());

  // *** vlog.debug("current=%s, input=%s", currentname, inputname);
  bool result = strcmp(currentname, inputname) == 0;
  return result;
}

// Switch the current thread into the input desktop
static bool
selectInputDesktop() {
  // - Open the input desktop
  HDESK desktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
  if (!desktop) {
    vlog.debug("unable to OpenInputDesktop(2):%lu", GetLastError());
    return false;
  }

  // - Switch into it
  if (!switchToDesktop(desktop)) {
    CloseDesktop(desktop);
    return false;
  }

  // ***
  DWORD size = 256;
  char currentname[256];
  if (GetUserObjectInformation(desktop, UOI_NAME, currentname, 256, &size)) {
    vlog.debug("switched to %s", currentname);
  }
  // ***

  vlog.debug("switched to input desktop");

  return true;
}


// -=- Access points to desktop-switching routines

bool
rfb::win32::desktopChangeRequired() {
  return !inputDesktopSelected();
}

bool
rfb::win32::changeDesktop() {
  return selectInputDesktop();
}


// -=- Ctrl-Alt-Del emulation

bool
rfb::win32::emulateCtrlAltDel() {
  rfb::win32::Handle sessionEventCad = 
    CreateEvent(0, FALSE, FALSE, "Global\\SessionEventTigerVNCCad");
  SetEvent(sessionEventCad);
  return true;
}


// -=- Application Event Log target Logger class

class Logger_EventLog : public Logger {
public:
  Logger_EventLog(const TCHAR* srcname) : Logger("EventLog") {
    eventlog = RegisterEventSource(NULL, srcname);
    if (!eventlog)
      printf("Unable to open event log:%ld\n", GetLastError());
  }
  ~Logger_EventLog() {
    if (eventlog)
      DeregisterEventSource(eventlog);
  }

  virtual void write(int level, const char *logname, const char *message) {
    if (!eventlog) return;
    TStr log(logname), msg(message);
    const TCHAR* strings[] = {log, msg};
    WORD type = EVENTLOG_INFORMATION_TYPE;
    if (level == 0) type = EVENTLOG_ERROR_TYPE;
    if (!ReportEvent(eventlog, type, 0, VNC4LogMessage, NULL, 2, 0, strings, NULL)) {
      // *** It's not at all clear what is the correct behaviour if this fails...
      printf("ReportEvent failed:%ld\n", GetLastError());
    }
  }

protected:
  HANDLE eventlog;
};

static Logger_EventLog* logger = 0;

bool rfb::win32::initEventLogLogger(const TCHAR* srcname) {
  if (logger)
    return false;
  logger = new Logger_EventLog(srcname);
  logger->registerLogger();
  return true;
}


// -=- Registering and unregistering the service

bool rfb::win32::registerService(const TCHAR* name,
                                 const TCHAR* display,
                                 const TCHAR* desc,
                                 int argc, char** argv) {

  // - Initialise the default service parameters
  const TCHAR* defaultcmdline;
  defaultcmdline = _T("-service");

  // - Get the full pathname of our executable
  ModuleFileName buffer;

  // - Calculate the command-line length
  int cmdline_len = _tcslen(buffer.buf) + 4;
  int i;
  for (i=0; i<argc; i++) {
    cmdline_len += strlen(argv[i]) + 3;
  }

  // - Add the supplied extra parameters to the command line
  TCharArray cmdline(cmdline_len+_tcslen(defaultcmdline));
  _stprintf(cmdline.buf, _T("\"%s\" %s"), buffer.buf, defaultcmdline);
  for (i=0; i<argc; i++) {
    _tcscat(cmdline.buf, _T(" \""));
    _tcscat(cmdline.buf, TStr(argv[i]));
    _tcscat(cmdline.buf, _T("\""));
  }
    
  // - Register the service

  // - Open the SCM
  ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Add the service
  ServiceHandle service = CreateService(scm,
    name, display, SC_MANAGER_ALL_ACCESS,
    SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
    SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
    cmdline.buf, NULL, NULL, NULL, NULL, NULL);
  if (!service)
    throw rdr::SystemException("unable to create service", GetLastError());

  // - Set a description
  SERVICE_DESCRIPTION sdesc = {(LPTSTR)desc};
  ChangeServiceConfig2(service, SERVICE_CONFIG_DESCRIPTION, &sdesc);

  // - Register the event log source
  RegKey hk, hk2;

  hk2.createKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"));
  hk.createKey(hk2, name);

  for (i=_tcslen(buffer.buf); i>0; i--) {
    if (buffer.buf[i] == _T('\\')) {
      buffer.buf[i+1] = 0;
      break;
    }
  }

  const TCHAR* dllFilename = _T("logmessages.dll");
  TCharArray dllPath(_tcslen(buffer.buf) + _tcslen(dllFilename) + 1);
  _tcscpy(dllPath.buf, buffer.buf);
  _tcscat(dllPath.buf, dllFilename);

  hk.setExpandString(_T("EventMessageFile"), dllPath.buf);
  hk.setInt(_T("TypesSupported"), EVENTLOG_ERROR_TYPE | EVENTLOG_INFORMATION_TYPE);

  Sleep(500);

  return true;
}

bool rfb::win32::unregisterService(const TCHAR* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Create the service
  ServiceHandle service = OpenService(scm, name, SC_MANAGER_ALL_ACCESS);
  if (!service)
    throw rdr::SystemException("unable to locate the service", GetLastError());
  if (!DeleteService(service))
    throw rdr::SystemException("unable to remove the service", GetLastError());

  // - Register the event log source
  RegKey hk;
  hk.openKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"));
  hk.deleteKey(name);

  Sleep(500);

  return true;
}


// -=- Starting and stopping the service

bool rfb::win32::startService(const TCHAR* name) {

  // - Open the SCM
  ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle service = OpenService(scm, name, SERVICE_START);
  if (!service)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Start the service
  if (!StartService(service, 0, NULL))
    throw rdr::SystemException("unable to start the service", GetLastError());

  Sleep(500);

  return true;
}

bool rfb::win32::stopService(const TCHAR* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle service = OpenService(scm, name, SERVICE_STOP);
  if (!service)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Start the service
  SERVICE_STATUS status;
  if (!ControlService(service, SERVICE_CONTROL_STOP, &status))
    throw rdr::SystemException("unable to stop the service", GetLastError());

  Sleep(500);

  return true;
}

DWORD rfb::win32::getServiceState(const TCHAR* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle service = OpenService(scm, name, SERVICE_INTERROGATE);
  if (!service)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Get the service status
  SERVICE_STATUS status;
  if (!ControlService(service, SERVICE_CONTROL_INTERROGATE, (SERVICE_STATUS*)&status))
    throw rdr::SystemException("unable to query the service", GetLastError());

  return status.dwCurrentState;
}

char* rfb::win32::serviceStateName(DWORD state) {
  switch (state) {
  case SERVICE_RUNNING: return strDup("Running");
  case SERVICE_STOPPED: return strDup("Stopped");
  case SERVICE_STOP_PENDING: return strDup("Stopping");
  };
  CharArray tmp(32);
  sprintf(tmp.buf, "Unknown (%lu)", state);
  return tmp.takeBuf();
}


bool rfb::win32::isServiceProcess() {
  return runAsService;
}
