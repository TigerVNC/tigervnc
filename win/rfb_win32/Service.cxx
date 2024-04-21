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
#include <rfb/util.h>


using namespace rdr;
using namespace rfb;
using namespace win32;

static LogWriter vlog("Service");


// - Internal service implementation functions

Service* service = nullptr;
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

Service::Service(const char* name_) : name(name_) {
  vlog.debug("Service");
  status_handle = nullptr;
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
  entry[0].lpServiceName = (char*)name;
  entry[0].lpServiceProc = serviceProc;
  entry[1].lpServiceName = nullptr;
  entry[1].lpServiceProc = nullptr;
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
  if (status_handle == nullptr) {
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
  service = nullptr;
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
    CreateEvent(nullptr, FALSE, FALSE, "Global\\SessionEventTigerVNCCad");
  SetEvent(sessionEventCad);
  return true;
}


// -=- Application Event Log target Logger class

class Logger_EventLog : public Logger {
public:
  Logger_EventLog(const char* srcname) : Logger("EventLog") {
    eventlog = RegisterEventSource(nullptr, srcname);
    if (!eventlog)
      printf("Unable to open event log:%ld\n", GetLastError());
  }
  ~Logger_EventLog() {
    if (eventlog)
      DeregisterEventSource(eventlog);
  }

  void write(int level, const char *logname, const char *message) override {
    if (!eventlog) return;
    const char* strings[] = {logname, message};
    WORD type = EVENTLOG_INFORMATION_TYPE;
    if (level == 0) type = EVENTLOG_ERROR_TYPE;
    if (!ReportEvent(eventlog, type, 0, VNC4LogMessage, nullptr, 2, 0, strings, nullptr)) {
      // *** It's not at all clear what is the correct behaviour if this fails...
      printf("ReportEvent failed:%ld\n", GetLastError());
    }
  }

protected:
  HANDLE eventlog;
};

static Logger_EventLog* logger = nullptr;

bool rfb::win32::initEventLogLogger(const char* srcname) {
  if (logger)
    return false;
  logger = new Logger_EventLog(srcname);
  logger->registerLogger();
  return true;
}


// -=- Registering and unregistering the service

bool rfb::win32::registerService(const char* name,
                                 const char* display,
                                 const char* desc,
                                 int argc, char** argv) {

  // - Initialise the default service parameters
  const char* defaultcmdline;
  defaultcmdline = "-service";

  // - Get the full pathname of our executable
  ModuleFileName buffer;

  // - Calculate the command-line length
  int cmdline_len = strlen(buffer.buf) + 4;
  int i;
  for (i=0; i<argc; i++) {
    cmdline_len += strlen(argv[i]) + 3;
  }

  // - Add the supplied extra parameters to the command line
  std::string cmdline;
  cmdline = format("\"%s\" %s", buffer.buf, defaultcmdline);
  for (i=0; i<argc; i++) {
    cmdline += " \"";
    cmdline += argv[i];
    cmdline += "\"";
  }
    
  // - Register the service

  // - Open the SCM
  ServiceHandle scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Add the service
  ServiceHandle handle = CreateService(scm,
    name, display, SC_MANAGER_ALL_ACCESS,
    SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
    SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
    cmdline.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
  if (!handle)
    throw rdr::SystemException("unable to create service", GetLastError());

  // - Set a description
  SERVICE_DESCRIPTION sdesc = {(LPTSTR)desc};
  ChangeServiceConfig2(handle, SERVICE_CONFIG_DESCRIPTION, &sdesc);

  // - Register the event log source
  RegKey hk, hk2;

  hk2.createKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application");
  hk.createKey(hk2, name);

  for (i=strlen(buffer.buf); i>0; i--) {
    if (buffer.buf[i] == '\\') {
      buffer.buf[i+1] = 0;
      break;
    }
  }

  const char* dllFilename = "logmessages.dll";
  std::string dllPath;
  dllPath = buffer.buf;
  dllPath += dllFilename;

  hk.setExpandString("EventMessageFile", dllPath.c_str());
  hk.setInt("TypesSupported", EVENTLOG_ERROR_TYPE | EVENTLOG_INFORMATION_TYPE);

  Sleep(500);

  return true;
}

bool rfb::win32::unregisterService(const char* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Create the service
  ServiceHandle handle = OpenService(scm, name, SC_MANAGER_ALL_ACCESS);
  if (!handle)
    throw rdr::SystemException("unable to locate the service", GetLastError());
  if (!DeleteService(handle))
    throw rdr::SystemException("unable to remove the service", GetLastError());

  // - Register the event log source
  RegKey hk;
  hk.openKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application");
  hk.deleteKey(name);

  Sleep(500);

  return true;
}


// -=- Starting and stopping the service

bool rfb::win32::startService(const char* name) {

  // - Open the SCM
  ServiceHandle scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle handle = OpenService(scm, name, SERVICE_START);
  if (!handle)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Start the service
  if (!StartService(handle, 0, nullptr))
    throw rdr::SystemException("unable to start the service", GetLastError());

  Sleep(500);

  return true;
}

bool rfb::win32::stopService(const char* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle handle = OpenService(scm, name, SERVICE_STOP);
  if (!handle)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Start the service
  SERVICE_STATUS status;
  if (!ControlService(handle, SERVICE_CONTROL_STOP, &status))
    throw rdr::SystemException("unable to stop the service", GetLastError());

  Sleep(500);

  return true;
}

DWORD rfb::win32::getServiceState(const char* name) {
  // - Open the SCM
  ServiceHandle scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!scm)
    throw rdr::SystemException("unable to open Service Control Manager", GetLastError());

  // - Locate the service
  ServiceHandle handle = OpenService(scm, name, SERVICE_INTERROGATE);
  if (!handle)
    throw rdr::SystemException("unable to open the service", GetLastError());

  // - Get the service status
  SERVICE_STATUS status;
  if (!ControlService(handle, SERVICE_CONTROL_INTERROGATE, (SERVICE_STATUS*)&status))
    throw rdr::SystemException("unable to query the service", GetLastError());

  return status.dwCurrentState;
}

const char* rfb::win32::serviceStateName(DWORD state) {
  switch (state) {
  case SERVICE_RUNNING: return "Running";
  case SERVICE_STOPPED: return "Stopped";
  case SERVICE_STOP_PENDING: return "Stopping";
  };
  static char tmp[32];
  sprintf(tmp, "Unknown (%lu)", state);
  return tmp;
}


bool rfb::win32::isServiceProcess() {
  return runAsService;
}
