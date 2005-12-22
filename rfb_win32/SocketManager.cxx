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

// -=- SocketManager.cxx

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <assert.h>

#include <rfb/LogWriter.h>
#include <rfb_win32/SocketManager.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SocketManager");


// -=- SocketManager

SocketManager::SocketManager() : sockets(0), events(0), nSockets(0), nAvail(0) {
}

SocketManager::~SocketManager() {
  for (int i=0; i<nSockets; i++) {
    if (!sockets[i].is_event)
      WSACloseEvent(events[i]);
  }
  delete [] events;
  delete [] sockets;
}


void SocketManager::addListener(network::SocketListener* sock_, network::SocketServer* srvr) {
  WSAEVENT event = WSACreateEvent();
  assert(event != WSA_INVALID_EVENT);
  addListener(sock_, event, srvr);
}

void SocketManager::addSocket(network::Socket* sock_, network::SocketServer* srvr) {
  WSAEVENT event = WSACreateEvent();
  assert(event != WSA_INVALID_EVENT);
  addSocket(sock_, event, srvr);
}


BOOL SocketManager::getMessage(MSG* msg, HWND hwnd, UINT minMsg, UINT maxMsg) {
  while (true) {
    // First check for idle timeout

    network::SocketServer* server = 0;
    int timeout = 0;
    for (int i=0; i<nSockets; i++) {
      if (!sockets[i].is_event &&
          sockets[i].server != server) {
        server = sockets[i].server;
        int t = server->checkTimeouts();
        if (t > 0 && (timeout == 0 || t < timeout))
          timeout = t;
      }
    }
    if (timeout == 0)
      timeout = INFINITE;

    // - Network IO is less common than messages - process it first
    DWORD result;
    if (nSockets) {
      result = WaitForMultipleObjects(nSockets, events, FALSE, 0);
      if (result == WAIT_TIMEOUT) {
        if (PeekMessage(msg, hwnd, minMsg, maxMsg, PM_REMOVE)) 
          return msg->message != WM_QUIT;

        result = MsgWaitForMultipleObjects(nSockets, events, FALSE, timeout,
                                           QS_ALLINPUT);
        if (result == WAIT_OBJECT_0 + nSockets) {
          if (PeekMessage(msg, hwnd, minMsg, maxMsg, PM_REMOVE)) 
            return msg->message != WM_QUIT;
          continue;
        }
      }
    } else
      return GetMessage(msg, hwnd, minMsg, maxMsg);

    if ((result >= WAIT_OBJECT_0) && (result < (WAIT_OBJECT_0 + nSockets))) {
      int index = result - WAIT_OBJECT_0;

      // - Process a socket event

      if (sockets[index].is_event) {
        // Process a general Win32 event
        // NB: The handler must reset the event!

        if (!sockets[index].handler->processEvent(events[index])) {
          removeSocket(index);
          continue;
        }
      } else if (sockets[index].is_conn) {
        // Process data from an active connection

        // Cancel event notification for this socket
        if (WSAEventSelect(sockets[index].fd, events[index], 0) == SOCKET_ERROR)
          vlog.info("unable to disable WSAEventSelect:%u", WSAGetLastError());

        // Reset the event object
        WSAResetEvent(events[index]);

        // Call the socket server to process the event
        if (!sockets[index].server->processSocketEvent(sockets[index].sock.conn)) {
          removeSocket(index);
          continue;
        }

        // Re-instate the required socket event
        // If the read event is still valid, the event object gets set here
        if (WSAEventSelect(sockets[index].fd, events[index], FD_READ | FD_CLOSE) == SOCKET_ERROR)
          throw rdr::SystemException("unable to re-enable WSAEventSelect:%u", WSAGetLastError());

      } else {
        // Accept an incoming connection
        vlog.debug("accepting incoming connection");

        // What kind of event is this?
        WSANETWORKEVENTS network_events;
        WSAEnumNetworkEvents(sockets[index].fd, events[index], &network_events);
        if (network_events.lNetworkEvents & FD_ACCEPT) {
          network::Socket* new_sock = sockets[index].sock.listener->accept();
          if ((sockets[index].server)->getDisable()) {
            delete new_sock;
            new_sock = 0;
          }
          if (new_sock) {
            sockets[index].server->addClient(new_sock);
            addSocket(new_sock, sockets[index].server);
          }
        } else if (network_events.lNetworkEvents & FD_CLOSE) {
          vlog.info("deleting listening socket");
          network::SocketListener* s = sockets[index].sock.listener;
          removeSocket(index);
          delete s;
        } else {
          vlog.error("unknown network event for listener");
        }

      }
    } else if (result == WAIT_FAILED) {
      throw rdr::SystemException("unable to wait for events", GetLastError());
    }
  }
}


void SocketManager::resizeArrays(int numSockets) {
  if (nAvail >= numSockets) return;
  while (nAvail < numSockets)
    nAvail = max(16, nAvail*2);

  SocketInfo* newinfo = new SocketInfo[nAvail];
  HANDLE* newevents = new HANDLE[nAvail];
  for (int i=0; i<nSockets; i++) {
    newinfo[i] = sockets[i];
    newevents[i] = events[i];
  }
  delete [] sockets;
  delete [] events;
  sockets = newinfo;
  events = newevents;
}

void SocketManager::addSocket(network::Socket* sock, HANDLE event, network::SocketServer* server) {
  resizeArrays(nSockets+1);

  sockets[nSockets].sock.conn = sock;
  sockets[nSockets].fd = sock->getFd();
  sockets[nSockets].server = server;
  events[nSockets] = event;
  sockets[nSockets].is_conn = true;
  sockets[nSockets].is_event = false;

  if (WSAEventSelect(sock->getFd(), event, FD_READ | FD_CLOSE) == SOCKET_ERROR)
    throw rdr::SystemException("unable to select on socket", WSAGetLastError());
  nSockets++;
}

void SocketManager::addListener(network::SocketListener* sock, HANDLE event, network::SocketServer* server) {
  resizeArrays(nSockets+1);

  sockets[nSockets].sock.listener = sock;
  sockets[nSockets].fd = sock->getFd();
  sockets[nSockets].server = server;
  events[nSockets] = event;
  sockets[nSockets].is_conn = false;
  sockets[nSockets].is_event = false;

  if (WSAEventSelect(sock->getFd(), event, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
    throw rdr::SystemException("unable to select on listener", WSAGetLastError());
  nSockets++;
}

void SocketManager::remListener(network::SocketListener* sock) {
  for (int index=0; index<nSockets; index++) {
    if (!sockets[index].is_conn &&
        !sockets[index].is_event) {
      vlog.debug("removing listening socket");
      removeSocket(index);
      delete sock;
    }
  }
}

void SocketManager::addEvent(HANDLE event, EventHandler* ecb) {
  resizeArrays(nSockets+1);

  sockets[nSockets].handler = ecb;
  events[nSockets] = event;
  sockets[nSockets].is_conn = false;
  sockets[nSockets].is_event = true;

  nSockets++;
}

void SocketManager::removeSocket(int index) {
  if (index >= nSockets)
    throw rdr::Exception("attempting to remove unregistered socket");

  if (!sockets[index].is_event)
    WSACloseEvent(events[index]);

  for (int i=index; i<nSockets-1; i++) {
    sockets[i] = sockets[i+1];
    events[i] = events[i+1];
  }

  nSockets--;
}

