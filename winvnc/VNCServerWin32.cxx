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

// -=- WinVNC Version 4.0 Main Routine

#include <winvnc/VNCServerWin32.h>
#include <winvnc/resource.h>
#include <winvnc/STrayIcon.h>

#include <rfb_win32/Win32Util.h>
#include <rfb_win32/Service.h>
#include <rfb/SSecurityFactoryStandard.h>
#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;
using namespace network;

static LogWriter vlog("VNCServerWin32");


const TCHAR* winvnc::VNCServerWin32::RegConfigPath = _T("Software\\TightVNC\\WinVNC4");

const UINT VNCM_REG_CHANGED = WM_USER;
const UINT VNCM_COMMAND = WM_USER + 1;


static IntParameter http_port("HTTPPortNumber",
  "TCP/IP port on which the server will serve the Java applet VNC Viewer ", 5800);
static IntParameter port_number("PortNumber",
  "TCP/IP port on which the server will accept connections", 5900);
static StringParameter hosts("Hosts",
  "Filter describing which hosts are allowed access to this server", "+");
static VncAuthPasswdConfigParameter vncAuthPasswd;
static BoolParameter localHost("LocalHost",
  "Only accept connections from via the local loop-back network interface", false);


// -=- ManagedListener
//     Wrapper class which simplifies the management of a listening socket
//     on a specified port, attached to a SocketManager and SocketServer.
//     Ensures that socket and filter are deleted and updated appropriately.

class ManagedListener {
public:
  ManagedListener(win32::SocketManager* mgr, SocketServer* svr)
    : sock(0), filter(0), port(0), manager(mgr),
      server(svr), localOnly(0) {}
  ~ManagedListener() {setPort(0);}
  void setPort(int port, bool localOnly=false);
  void setFilter(const char* filter);
  TcpListener* sock;
protected:
  TcpFilter* filter;
  win32::SocketManager* manager;
  SocketServer* server;
  int port;
  bool localOnly;
};

// - If the port number/localHost setting has changed then tell the
//   SocketManager to shutdown and delete it.  Also remove &
//   delete the filter.  Then try to open a socket on the new port.
void ManagedListener::setPort(int newPort, bool newLocalOnly) {
  if ((port == newPort) && (localOnly == newLocalOnly) && sock) return;
  if (sock) {
    vlog.info("Closed TcpListener on port %d", port);
    sock->setFilter(0);
    delete filter;
    manager->remListener(sock);
    sock = 0;
    filter = 0;
  }
  port = newPort;
  localOnly = newLocalOnly;
  if (port != 0) {
    try {
      sock = new TcpListener(port, localOnly);
      vlog.info("Created TcpListener on port %d%s", port,
                localOnly ? "(localhost)" : "(any)");
    } catch (rdr::Exception& e) {
      vlog.error("TcpListener on port %d failed (%s)", port, e.str());
    }
  }
  if (sock)
    manager->addListener(sock, server);
}

void ManagedListener::setFilter(const char* newFilter) {
  if (!sock) return;
  vlog.info("Updating TcpListener filter");
  sock->setFilter(0);
  delete filter;
  filter = new TcpFilter(newFilter);
  sock->setFilter(filter);
}


VNCServerWin32::VNCServerWin32()
  : vncServer(CStr(ComputerName().buf), &desktop),
    httpServer(0), runServer(false),
    isDesktopStarted(false),
    command(NoCommand), commandSig(commandLock),
    queryConnectDialog(0) {
  // Create the Java-viewer HTTP server
  httpServer = new JavaViewerServer(&vncServer);

  // Initialise the desktop
  desktop.setStatusLocation(&isDesktopStarted);

  // Initialise the VNC server
  vncServer.setQueryConnectionHandler(this);

  // Register the desktop's event to be handled
  sockMgr.addEvent(desktop.getUpdateEvent(), &desktop);
}

VNCServerWin32::~VNCServerWin32() {
  // Stop the SDisplay from updating our state
  desktop.setStatusLocation(0);

  // Destroy the HTTP server
  delete httpServer;
}


int VNCServerWin32::run() {
  { Lock l(runLock);
    hostThread = Thread::self();
    runServer = true;
  }

  // - Register for notification of configuration changes
  if (isServiceProcess())
    config.setKey(HKEY_LOCAL_MACHINE, RegConfigPath);
  else
    config.setKey(HKEY_CURRENT_USER, RegConfigPath);
  config.setNotifyThread(Thread::self(), VNCM_REG_CHANGED);

  // - Create the tray icon if possible
  STrayIconThread trayIcon(*this, IDI_ICON, IDI_CONNECTED, IDR_TRAY);

  DWORD result = 0;
  try {
    // - Create some managed listening sockets
    ManagedListener rfb(&sockMgr, &vncServer);
    ManagedListener http(&sockMgr, httpServer);

    // - Continue to operate until WM_QUIT is processed
    MSG msg;
    do {
      // -=- Make sure we're listening on the right ports.
      rfb.setPort(port_number, localHost);
      http.setPort(http_port, localHost);

      // -=- Update the Java viewer's web page port number.
      httpServer->setRFBport(rfb.sock ? port_number : 0);

      // -=- Update the TCP address filter for both ports, if open.
      CharArray pattern;
      pattern.buf = hosts.getData();
      if (!localHost) {
        rfb.setFilter(pattern.buf);
        http.setFilter(pattern.buf);
      }

      // - If there is a listening port then add the address to the
      //   tray icon's tool-tip text.
      {
        const TCHAR* prefix = isServiceProcess() ?
          _T("VNC Server (Service):") : _T("VNC Server (User):");

        std::list<char*> addrs;
        if (rfb.sock)
          rfb.sock->getMyAddresses(&addrs);
        else
          addrs.push_front(strDup("Not accepting connections"));

        std::list<char*>::iterator i, next_i;
        int length = _tcslen(prefix)+1;
        for (i=addrs.begin(); i!= addrs.end(); i++)
          length += strlen(*i) + 1;

        TCharArray toolTip(length);
        _tcscpy(toolTip.buf, prefix);
        for (i=addrs.begin(); i!= addrs.end(); i=next_i) {
          next_i = i; next_i ++;
          TCharArray addr = *i;    // Assumes ownership of string
          _tcscat(toolTip.buf, addr.buf);
          if (next_i != addrs.end())
            _tcscat(toolTip.buf, _T(","));
        }
        trayIcon.setToolTip(toolTip.buf);
      }

      vlog.debug("Entering message loop");

      // - Run the server until the registry changes, or we're told to quit
      while (sockMgr.getMessage(&msg, NULL, 0, 0)) {
        if (msg.hwnd == 0) {
          if (msg.message == VNCM_REG_CHANGED)
            break;
          if (msg.message == VNCM_COMMAND)
            doCommand();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

    } while ((msg.message != WM_QUIT) || runServer);

    vlog.debug("Server exited cleanly");
  } catch (rdr::SystemException &s) {
    vlog.error(s.str());
    result = s.err;
  } catch (rdr::Exception &e) {
    vlog.error(e.str());
  }

  { Lock l(runLock);
    runServer = false;
    hostThread = 0;
  }

  return result;
}

void VNCServerWin32::stop() {
  Lock l(runLock);
  runServer = false;
  PostThreadMessage(hostThread->getThreadId(), WM_QUIT, 0, 0);
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


VNCServerST::queryResult VNCServerWin32::queryConnection(network::Socket* sock,
                                            const char* userName,
                                            char** reason)
{
  if (queryConnectDialog) {
    *reason = rfb::strDup("Another connection is currently being queried.");
    return VNCServerST::REJECT;
  }
  queryConnectDialog = new QueryConnectDialog(sock, userName, this);
  queryConnectDialog->startDialog();
  return VNCServerST::PENDING;
}

void VNCServerWin32::queryConnectionComplete() {
  Thread* qcd = queryConnectDialog;
  queueCommand(QueryConnectionComplete, 0, 0);
  delete qcd->join();
}


bool VNCServerWin32::queueCommand(Command cmd, const void* data, int len) {
  Lock l(commandLock);
  while (command != NoCommand) commandSig.wait();
  command = cmd;
  commandData = data;
  commandDataLen = len;
  if (PostThreadMessage(hostThread->getThreadId(), VNCM_COMMAND, 0, 0))
    while (command != NoCommand) commandSig.wait();
  else
    return false;
  return true;
}

void VNCServerWin32::doCommand() {
  Lock l(commandLock);
  if (command == NoCommand) return;

  // Perform the required command
  switch (command) {

  case DisconnectClients:
    // Disconnect all currently active VNC Viewers
    vncServer.closeClients((const char*)commandData);
    break;

  case AddClient:
    // Make a reverse connection to a VNC Viewer
    vncServer.addClient((network::Socket*)commandData, true);
    sockMgr.addSocket((network::Socket*)commandData, &vncServer);
    break;

  case QueryConnectionComplete:
    // The Accept/Reject dialog has completed
    // Get the result, then clean it up
    vncServer.approveConnection(queryConnectDialog->getSock(),
                                queryConnectDialog->isAccepted(),
                                "Connection rejected by user");
    queryConnectDialog = 0;
    break;

  default:
    vlog.error("unknown command %d queued", command);
  };

  // Clear the command and signal completion
  command = NoCommand;
  commandSig.signal();
}
