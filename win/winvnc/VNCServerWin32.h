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

#ifndef __VNCSERVER_WIN32_H__
#define __VNCSERVER_WIN32_H__

#include <winsock2.h>
#include <network/TcpSocket.h>
#include <rfb/VNCServerST.h>
#include <rfb_win32/RegConfig.h>
#include <rfb_win32/SDisplay.h>
#include <rfb_win32/SocketManager.h>
#include <rfb_win32/TCharArray.h>
#include <winvnc/QueryConnectDialog.h>
#include <winvnc/ManagedListener.h>

namespace os {
    class Mutex;
    class Condition;
    class Thread;
}

namespace winvnc {

  class ListConnInfo;
  class STrayIconThread;

  class VNCServerWin32 : rfb::win32::QueryConnectionHandler,
                         rfb::win32::SocketManager::AddressChangeNotifier,
                         rfb::win32::RegConfig::Callback,
                         rfb::win32::EventHandler {
  public:
    VNCServerWin32();
    virtual ~VNCServerWin32();

    // Run the server in the current thread
    int run();

    // Cause the run() call to return
    // THREAD-SAFE
    void stop();

    // Determine whether a viewer is active
    // THREAD-SAFE
    bool isServerInUse() const {return isDesktopStarted;}

    // Connect out to the specified VNC Viewer
    // THREAD-SAFE
    bool addNewClient(const char* client);

    // Disconnect all connected clients
    // THREAD-SAFE
    bool disconnectClients(const char* reason=0);

    // Call used to notify VNCServerST of user accept/reject query completion
    // CALLED FROM AcceptConnectDialog THREAD
    void queryConnectionComplete();

    // Where to read the configuration settings from
    static const TCHAR* RegConfigPath;

    bool getClientsInfo(ListConnInfo* LCInfo);

    bool setClientsStatus(ListConnInfo* LCInfo);

  protected:
    // QueryConnectionHandler interface
    // Callback used to prompt user to accept or reject a connection.
    // CALLBACK IN VNCServerST "HOST" THREAD
    virtual void queryConnection(network::Socket* sock,
                                 const char* userName);

    // SocketManager::AddressChangeNotifier interface
    // Used to keep tray icon up to date
    virtual void processAddressChange();

    // RegConfig::Callback interface
    // Called via the EventManager whenever RegConfig sees the registry change
    virtual void regConfigChanged();

    // EventHandler interface
    // Used to perform queued commands
    virtual void processEvent(HANDLE event);

    void getConnInfo(ListConnInfo * listConn);
    void setConnStatus(ListConnInfo* listConn);

  protected:
    // Perform a particular internal function in the server thread
    typedef enum {NoCommand, DisconnectClients, AddClient, QueryConnectionComplete, SetClientsStatus, GetClientsInfo} Command;
    bool queueCommand(Command cmd, const void* data, int len, bool wait=true);
    Command command;
    const void* commandData;
    int commandDataLen;
    os::Mutex* commandLock;
    os::Condition* commandSig;
    rfb::win32::Handle commandEvent;
    rfb::win32::Handle sessionEvent;

    // VNCServerWin32 Server-internal state
    rfb::win32::SDisplay desktop;
    rfb::VNCServerST vncServer;
    os::Mutex* runLock;
    DWORD thread_id;
    bool runServer;
    bool isDesktopStarted;
    rfb::win32::SocketManager sockMgr;
    rfb::win32::RegConfig config;

    ManagedListener rfbSock;
    STrayIconThread* trayIcon;

    QueryConnectDialog* queryConnectDialog;
  };

};

#endif
