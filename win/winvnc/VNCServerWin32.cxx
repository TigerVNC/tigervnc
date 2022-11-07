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

// -=- WinVNC Version 4.0 Main Routine

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winvnc/VNCServerWin32.h>
#include <winvnc/resource.h>
#include <winvnc/ListConnInfo.h>
#include <winvnc/STrayIcon.h>

#include <os/Mutex.h>

#include <rfb_win32/ComputerName.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Service.h>

#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;
using namespace network;

static LogWriter vlog("VNCServerWin32");


const TCHAR* winvnc::VNCServerWin32::RegConfigPath = _T("Software\\TigerVNC\\WinVNC4");


static IntParameter port_number("PortNumber",
  "TCP/IP port on which the server will accept connections", 5900);
static StringParameter hosts("Hosts",
  "Filter describing which hosts are allowed access to this server", "+");
static BoolParameter localHost("LocalHost",
  "Only accept connections from via the local loop-back network interface", false);
static BoolParameter queryOnlyIfLoggedOn("QueryOnlyIfLoggedOn",
  "Only prompt for a local user to accept incoming connections if there is a user logged on", false);
static BoolParameter showTrayIcon("ShowTrayIcon",
  "Show the configuration applet in the system tray icon", true);


VNCServerWin32::VNCServerWin32()
  : command(NoCommand),
    commandEvent(CreateEvent(0, TRUE, FALSE, 0)),
    sessionEvent(isServiceProcess() ?
      CreateEvent(0, FALSE, FALSE, "Global\\SessionEventTigerVNC") : 0),
    vncServer(CStr(ComputerName().buf), &desktop),
    thread_id(-1), runServer(false), isDesktopStarted(false),
    config(&sockMgr), rfbSock(&sockMgr), trayIcon(0),
    queryConnectDialog(0)
{
  commandLock = new os::Mutex;
  commandSig = new os::Condition(commandLock);

  runLock = new os::Mutex;

  // Initialise the desktop
  desktop.setStatusLocation(&isDesktopStarted);
  desktop.setQueryConnectionHandler(this);

  // Register the desktop's event to be handled
  sockMgr.addEvent(desktop.getUpdateEvent(), &desktop);
  sockMgr.addEvent(desktop.getTerminateEvent(), this);

  // Register the queued command event to be handled
  sockMgr.addEvent(commandEvent, this);
  if (sessionEvent)
    sockMgr.addEvent(sessionEvent, this);
}

VNCServerWin32::~VNCServerWin32() {
  delete trayIcon;

  // Stop the SDisplay from updating our state
  desktop.setStatusLocation(0);

  // Join the Accept/Reject dialog thread
  if (queryConnectDialog) {
    queryConnectDialog->wait();
    delete queryConnectDialog;
  }

  delete runLock;

  delete commandSig;
  delete commandLock;
}


void VNCServerWin32::processAddressChange() {
  if (!trayIcon)
    return;

  // Tool-tip prefix depends on server mode
  const TCHAR* prefix = _T("VNC Server (User):");
  if (isServiceProcess())
    prefix = _T("VNC Server (Service):");

  // Fetch the list of addresses
  std::list<char*> addrs;
  if (rfbSock.isListening())
    TcpListener::getMyAddresses(&addrs);
  else
    addrs.push_front(strDup("Not accepting connections"));

  // Allocate space for the new tip
  std::list<char*>::iterator i, next_i;
  int length = _tcslen(prefix)+1;
  for (i=addrs.begin(); i!= addrs.end(); i++)
    length += strlen(*i) + 1;

  // Build the new tip
  TCharArray toolTip(length);
  _tcscpy(toolTip.buf, prefix);
  for (i=addrs.begin(); i!= addrs.end(); i=next_i) {
    next_i = i; next_i ++;
    TCharArray addr(*i);    // Assumes ownership of string
    _tcscat(toolTip.buf, addr.buf);
    if (next_i != addrs.end())
      _tcscat(toolTip.buf, _T(","));
  }
  
  // Pass the new tip to the tray icon
  vlog.info("Refreshing tray icon");
  trayIcon->setToolTip(toolTip.buf);
}

void VNCServerWin32::regConfigChanged() {
  // -=- Make sure we're listening on the right ports.
  rfbSock.setServer(&vncServer);
  rfbSock.setPort(port_number, localHost);

  // -=- Update the TCP address filter for both ports, if open.
  CharArray pattern(hosts.getData());
  rfbSock.setFilter(pattern.buf);

  // -=- Update the tray icon tooltip text with IP addresses
  processAddressChange();
}


int VNCServerWin32::run() {
  {
    os::AutoMutex a(runLock);
    thread_id = GetCurrentThreadId();
    runServer = true;
  }

  // - Create the tray icon (if possible)
  if (showTrayIcon)
	  trayIcon = new STrayIconThread(*this, IDI_ICON, IDI_CONNECTED,
                                 IDI_ICON_DISABLE, IDI_CONNECTED_DISABLE,
                                 IDR_TRAY);

  // - Register for notification of configuration changes
  config.setCallback(this);
  if (isServiceProcess())
    config.setKey(HKEY_LOCAL_MACHINE, RegConfigPath);
  else
    config.setKey(HKEY_CURRENT_USER, RegConfigPath);

  // - Set the address-changed handler for the RFB socket
  rfbSock.setAddressChangeNotifier(this);

  DWORD result = 0;
  try {
    vlog.debug("Entering message loop");

    // - Run the server until we're told to quit
    MSG msg;
    int result = 0;
    while (runServer) {
      result = sockMgr.getMessage(&msg, NULL, 0, 0);
      if (result < 0)
        throw rdr::SystemException("getMessage", GetLastError());
      if (!isServiceProcess() && (result == 0))
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    vlog.debug("Server exited cleanly");
  } catch (rdr::SystemException &s) {
    vlog.error("%s", s.str());
    result = s.err;
  } catch (rdr::Exception &e) {
    vlog.error("%s", e.str());
  }

  {
    os::AutoMutex a(runLock);
    runServer = false;
    thread_id = (DWORD)-1;
  }

  return result;
}

void VNCServerWin32::stop() {
  os::AutoMutex a(runLock);
  runServer = false;
  if (thread_id != (DWORD)-1)
    PostThreadMessage(thread_id, WM_QUIT, 0, 0);
}


bool VNCServerWin32::disconnectClients(const char* reason) {
  return queueCommand(DisconnectClients, reason, 0);
}

bool VNCServerWin32::addNewClient(const char* client) {
  TcpSocket* sock = 0;
  try {
    CharArray hostname;
    int port;
    getHostAndPort(client, &hostname.buf, &port, 5500);
    vlog.error("port=%d", port);
    sock = new TcpSocket(hostname.buf, port);
    if (queueCommand(AddClient, sock, 0))
      return true;
    delete sock;
  } catch (...) {
    delete sock;
  }
  return false;
}

bool VNCServerWin32::getClientsInfo(ListConnInfo* LCInfo) {
  return queueCommand(GetClientsInfo, LCInfo, 0);
}

bool VNCServerWin32::setClientsStatus(ListConnInfo* LCInfo) {
  return queueCommand(SetClientsStatus, LCInfo, 0);
}

void VNCServerWin32::queryConnection(network::Socket* sock,
                                     const char* userName)
{
  if (queryOnlyIfLoggedOn && CurrentUserToken().noUserLoggedOn()) {
    vncServer.approveConnection(sock, true, NULL);
    return;
  }
  if (queryConnectDialog) {
    vncServer.approveConnection(sock, false, "Another connection is currently being queried.");
    return;
  }
  queryConnectDialog = new QueryConnectDialog(sock, userName, this);
  queryConnectDialog->startDialog();
}

void VNCServerWin32::queryConnectionComplete() {
  queueCommand(QueryConnectionComplete, 0, 0, false);
}


bool VNCServerWin32::queueCommand(Command cmd, const void* data, int len, bool wait) {
  os::AutoMutex a(commandLock);
  while (command != NoCommand)
    commandSig->wait();
  command = cmd;
  commandData = data;
  commandDataLen = len;
  SetEvent(commandEvent);
  if (wait) {
    while (command != NoCommand)
      commandSig->wait();
    commandSig->signal();
  }
  return true;
}

void VNCServerWin32::processEvent(HANDLE event_) {
  ResetEvent(event_);

  if (event_ == commandEvent.h) {
    // If there is no command queued then return immediately
    {
      os::AutoMutex a(commandLock);
      if (command == NoCommand)
        return;
    }

    // Perform the required command
    switch (command) {

    case DisconnectClients:
      // Disconnect all currently active VNC Viewers
      vncServer.closeClients((const char*)commandData);
      break;

    case AddClient:
      // Make a reverse connection to a VNC Viewer
      sockMgr.addSocket((network::Socket*)commandData, &vncServer);
      break;
  case GetClientsInfo:
    getConnInfo((ListConnInfo*)commandData);
    break;
  case SetClientsStatus:
    setConnStatus((ListConnInfo*)commandData);
    break;

    case QueryConnectionComplete:
      // The Accept/Reject dialog has completed
      // Get the result, then clean it up
      vncServer.approveConnection(queryConnectDialog->getSock(),
                                  queryConnectDialog->isAccepted(),
                                  "Connection rejected by user");
      queryConnectDialog->wait();
      delete queryConnectDialog;
      queryConnectDialog = 0;
      break;

    default:
      vlog.error("unknown command %d queued", command);
    };

    // Clear the command and signal completion
    {
      os::AutoMutex a(commandLock);
      command = NoCommand;
      commandSig->signal();
    }
  } else if ((event_ == sessionEvent.h) ||
             (event_ == desktop.getTerminateEvent())) {
    stop();
  }
}

void VNCServerWin32::getConnInfo(ListConnInfo * listConn)
{
  std::list<network::Socket*> sockets;
  std::list<network::Socket*>::iterator i;

  listConn->Clear();
  listConn->setDisable(sockMgr.getDisable(&vncServer));

  vncServer.getSockets(&sockets);

  for (i = sockets.begin(); i != sockets.end(); i++) {
    rfb::SConnection* conn;
    int status;

    conn = vncServer.getConnection(*i);
    if (!conn)
      continue;

    if (!conn->authenticated())
      status = 3;
    else if (conn->accessCheck(rfb::SConnection::AccessPtrEvents |
                               rfb::SConnection::AccessKeyEvents |
                               rfb::SConnection::AccessView))
      status = 0;
    else if (conn->accessCheck(rfb::SConnection::AccessView))
      status = 1;
    else
      status = 2;

    listConn->addInfo((void*)(*i), (*i)->getPeerAddress(), status);
  }
}

void VNCServerWin32::setConnStatus(ListConnInfo* listConn)
{
  sockMgr.setDisable(&vncServer, listConn->getDisable());

  if (listConn->Empty())
    return;

  for (listConn->iBegin(); !listConn->iEnd(); listConn->iNext()) {
    network::Socket* sock;
    rfb::SConnection* conn;
    int status;

    sock = (network::Socket*)listConn->iGetConn();

    conn = vncServer.getConnection(sock);
    if (!conn)
      continue;

    status = listConn->iGetStatus();
    if (status == 3) {
      conn->close(0);
    } else {
      rfb::SConnection::AccessRights ar;

      ar = rfb::SConnection::AccessDefault;

      switch (status) {
      case 0:
        ar |= rfb::SConnection::AccessPtrEvents |
              rfb::SConnection::AccessKeyEvents |
              rfb::SConnection::AccessView;
        break;
      case 1:
        ar |= rfb::SConnection::AccessView;
        ar &= ~(rfb::SConnection::AccessPtrEvents |
                rfb::SConnection::AccessKeyEvents);
        break;
      case 2:
        ar &= ~(rfb::SConnection::AccessPtrEvents |
                rfb::SConnection::AccessKeyEvents |
                rfb::SConnection::AccessView);
        break;
      }
      conn->setAccessRights(ar);
      conn->framebufferUpdateRequest(vncServer.getPixelBuffer()->getRect(), false);
    }
  }
}
