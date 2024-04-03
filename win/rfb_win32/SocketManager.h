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

// -=- SocketManager.h

// Socket manager class for Win32.
// Passed a network::SocketListener and a rfb::VNCServer when
// constructed.  Uses WSAAsyncSelect to get notifications of network 
// connection attempts.  When an incoming connection is received,
// the manager will call rfb::VNCServer::addClient().  If
// addClient returns true then the manager registers interest in
// network events on that socket, and calls
// rfb::VNCServer::processSocketEvent().

#ifndef __RFB_WIN32_SOCKET_MGR_H__
#define __RFB_WIN32_SOCKET_MGR_H__

#include <map>
#include <rfb_win32/EventManager.h>

namespace network {
  class SocketListener;
  class Socket;
}

namespace rfb {
  class VNCServer;

  namespace win32 {

    class SocketManager : public EventManager, EventHandler {
    public:
      SocketManager();
      virtual ~SocketManager();

      // AddressChangeNotifier callback interface
      // If an object implementing this is passed to addListener then it will be
      // called whenever the SocketListener's address list changes
      class AddressChangeNotifier {
      public:
        virtual ~AddressChangeNotifier() {}
        virtual void processAddressChange() = 0;
      };

      // Add a listening socket.  Incoming connections will be added to the supplied
      // VNCServer.
      void addListener(network::SocketListener* sock_,
                       VNCServer* srvr,
                       AddressChangeNotifier* acn = nullptr);

      // Remove and delete a listening socket.
      void remListener(network::SocketListener* sock);

      // Add an already-connected socket.  Socket events will cause the supplied
      // VNCServer to be called.  The socket must ALREADY BE REGISTERED with
      // the VNCServer.
      void addSocket(network::Socket* sock_, VNCServer* srvr, bool outgoing=true);

      bool getDisable(VNCServer* srvr);
      void setDisable(VNCServer* srvr, bool disable);

    protected:
      int checkTimeouts() override;
      void processEvent(HANDLE event) override;
      virtual void remSocket(network::Socket* sock);

      struct ConnInfo {
        network::Socket* sock;
        VNCServer* server;
      };
      struct ListenInfo {
        network::SocketListener* sock;
        VNCServer* server;
        AddressChangeNotifier* notifier;
        bool disable;
      };
      std::map<HANDLE, ListenInfo> listeners;
      std::map<HANDLE, ConnInfo> connections;
   };

  }

}

#endif
