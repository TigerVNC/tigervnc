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

// -=- SocketManager.h

// Socket manager class for Win32.
// Passed a network::SocketListener and a network::SocketServer when
// constructed.  Uses WSAAsyncSelect to get notifications of network 
// connection attempts.  When an incoming connection is received,
// the manager will call network::SocketServer::addClient().  If
// addClient returns true then the manager registers interest in
// network events on that socket, and calls
// network::SocketServer::processSocketEvent().

#ifndef __RFB_WIN32_SOCKET_MGR_H__
#define __RFB_WIN32_SOCKET_MGR_H__

#include <list>

#include <network/Socket.h>
#include <rfb_win32/MsgWindow.h>

namespace rfb {

  namespace win32 {

    class SocketManager {
    public:
      SocketManager();
      virtual ~SocketManager();

      // Add a listening socket.  Incoming connections will be added to the supplied
      // SocketServer.
      void addListener(network::SocketListener* sock_, network::SocketServer* srvr);

      // Remove and delete a listening socket.
      void remListener(network::SocketListener* sock);

      // Add an already-connected socket.  Socket events will cause the supplied
      // SocketServer to be called.  The socket must ALREADY BE REGISTERED with
      // the SocketServer.
      void addSocket(network::Socket* sock_, network::SocketServer* srvr);

      // Add a Win32 event & handler for it to the SocketManager
      // This event will be blocked on along with the registered Sockets, and the
      // handler called whenever it is discovered to be set.
      // NB: SocketManager does NOT call ResetEvent on the event!
      // NB: If processEvent returns false then the event is no longer registered,
      //     and the event object is assumed to have been closed by processEvent()
      struct EventHandler {
        virtual ~EventHandler() {}
        virtual bool processEvent(HANDLE event) = 0;
      };
      void addEvent(HANDLE event, EventHandler* ecb);

      // getMessage
      //
      // Either return a message from the thread's message queue or process a socket
      // event.
      // Returns whenever a message needs processing.  Returns false if message is
      // WM_QUIT, true for all other messages.
      BOOL getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg);

    protected:
      void addListener(network::SocketListener* sock, HANDLE event, network::SocketServer* server);
      void addSocket(network::Socket* sock, HANDLE event, network::SocketServer* server);
      void resizeArrays(int numSockets);
      void removeSocket(int index);
      struct SocketInfo {
        union {
          network::Socket* conn;
          network::SocketListener* listener;
        } sock;
        SOCKET fd;
        bool is_conn;
        bool is_event;
        union {
          network::SocketServer* server;
          EventHandler* handler;
        };
      };
      SocketInfo* sockets;
      HANDLE* events;
      int nSockets;
      int nAvail;
   };

  }

}

#endif
