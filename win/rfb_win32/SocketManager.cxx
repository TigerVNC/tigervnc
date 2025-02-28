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

// -=- SocketManager.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winsock2.h>
#include <list>

#include <core/Exception.h>
#include <core/LogWriter.h>
#include <core/Timer.h>
#include <core/time.h>

#include <rdr/FdOutStream.h>

#include <network/Socket.h>

#include <rfb/VNCServer.h>
#include <rfb_win32/SocketManager.h>

using namespace core;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SocketManager");


// -=- SocketManager

SocketManager::SocketManager() {
}

SocketManager::~SocketManager() {
}


static void requestAddressChangeEvents(network::SocketListener* sock_) {
  DWORD dummy = 0;
  if (WSAIoctl(sock_->getFd(), SIO_ADDRESS_LIST_CHANGE, nullptr, 0, nullptr, 0, &dummy, nullptr, nullptr) == SOCKET_ERROR) {
    DWORD err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK)
      vlog.error("Unable to track address changes: 0x%08x", (unsigned)err);
  }
}


void SocketManager::addListener(network::SocketListener* sock_,
                                VNCServer* srvr,
                                AddressChangeNotifier* acn) {
  WSAEVENT event = WSACreateEvent();
  long flags = FD_ACCEPT | FD_CLOSE;
  if (acn)
    flags |= FD_ADDRESS_LIST_CHANGE;
  try {
    if (event && (WSAEventSelect(sock_->getFd(), event, flags) == SOCKET_ERROR))
      throw core::socket_error("Unable to select on listener", WSAGetLastError());

    // requestAddressChangeEvents MUST happen after WSAEventSelect, so that the socket is non-blocking
    if (acn)
      requestAddressChangeEvents(sock_);

    // addEvent is the last thing we do, so that the event is NOT registered if previous steps fail
    if (!event || !addEvent(event, this))
      throw std::runtime_error("Unable to add listener");
  } catch (std::exception& e) {
    if (event)
      WSACloseEvent(event);
    delete sock_;
    vlog.error("%s", e.what());
    throw;
  }

  ListenInfo li;
  li.sock = sock_;
  li.server = srvr;
  li.notifier = acn;
  li.disable = false;
  listeners[event] = li;
}

void SocketManager::remListener(network::SocketListener* sock) {
  std::map<HANDLE,ListenInfo>::iterator i;
  for (i=listeners.begin(); i!=listeners.end(); i++) {
    if (i->second.sock == sock) {
      removeEvent(i->first);
      WSACloseEvent(i->first);
      delete sock;
      listeners.erase(i);
      return;
    }
  }
  throw std::runtime_error("Listener not registered");
}


void SocketManager::addSocket(network::Socket* sock_, VNCServer* srvr, bool outgoing) {
  WSAEVENT event = WSACreateEvent();
  if (!event || !addEvent(event, this) ||
      (WSAEventSelect(sock_->getFd(), event, FD_READ | FD_CLOSE) == SOCKET_ERROR)) {
    if (event)
      WSACloseEvent(event);
    delete sock_;
    vlog.error("Unable to add connection");
    return;
  }
  ConnInfo ci;
  ci.sock = sock_;
  ci.server = srvr;
  connections[event] = ci;
  srvr->addSocket(sock_, outgoing);
}

void SocketManager::remSocket(network::Socket* sock_) {
  std::map<HANDLE,ConnInfo>::iterator i;
  for (i=connections.begin(); i!=connections.end(); i++) {
    if (i->second.sock == sock_) {
      i->second.server->removeSocket(sock_);
      removeEvent(i->first);
      WSACloseEvent(i->first);
      delete sock_;
      connections.erase(i);
      return;
    }
  }
  throw std::runtime_error("Socket not registered");
}

bool SocketManager::getDisable(VNCServer* srvr)
{
  std::map<HANDLE,ListenInfo>::iterator i;
  for (i=listeners.begin(); i!=listeners.end(); i++) {
    if (i->second.server == srvr) {
      return i->second.disable;
    }
  }
  throw std::runtime_error("Listener not registered");
}

void SocketManager::setDisable(VNCServer* srvr, bool disable)
{
  bool found = false;
  std::map<HANDLE,ListenInfo>::iterator i;
  for (i=listeners.begin(); i!=listeners.end(); i++) {
    if (i->second.server == srvr) {
      i->second.disable = disable;
      // There might be multiple sockets for the same server, so
      // continue iterating
      found = true;
    }
  }
  if (!found)
    throw std::runtime_error("Listener not registered");
}

int SocketManager::checkTimeouts() {
  int timeout = EventManager::checkTimeouts();

  int nextTimeout = Timer::checkTimeouts();
  if (nextTimeout >= 0 && nextTimeout < timeout)
    timeout = nextTimeout;

  std::list<network::Socket*> shutdownSocks;
  std::map<HANDLE,ConnInfo>::iterator j, j_next;
  for (j=connections.begin(); j!=connections.end(); j=j_next) {
    j_next = j; j_next++;
    if (j->second.sock->isShutdown())
      shutdownSocks.push_back(j->second.sock);
    else {
      long eventMask = FD_READ | FD_CLOSE;
      if (j->second.sock->outStream().hasBufferedData())
        eventMask |= FD_WRITE;
      if (WSAEventSelect(j->second.sock->getFd(), j->first, eventMask) == SOCKET_ERROR)
        throw core::socket_error("unable to adjust WSAEventSelect:%u", WSAGetLastError());
    }
  }

  std::list<network::Socket*>::iterator k;
  for (k=shutdownSocks.begin(); k!=shutdownSocks.end(); k++)
    remSocket(*k);

  return timeout;
}


void SocketManager::processEvent(HANDLE event) {
  if (listeners.count(event)) {
    ListenInfo li = listeners[event];

    // Accept an incoming connection
    vlog.debug("Accepting incoming connection");

    // What kind of event is this?
    WSANETWORKEVENTS network_events;
    WSAEnumNetworkEvents(li.sock->getFd(), event, &network_events);
    if (network_events.lNetworkEvents & FD_ACCEPT) {
      network::Socket* new_sock = li.sock->accept();
      if (new_sock && li.disable) {
        delete new_sock;
        new_sock = nullptr;
      }
      if (new_sock)
        addSocket(new_sock, li.server, false);
    } else if (network_events.lNetworkEvents & FD_CLOSE) {
      vlog.info("Deleting listening socket");
      remListener(li.sock);
    } else if (network_events.lNetworkEvents & FD_ADDRESS_LIST_CHANGE) {
      li.notifier->processAddressChange();
      requestAddressChangeEvents(li.sock);
    } else {
      vlog.error("Unknown listener event: %lx", network_events.lNetworkEvents);
    }
  } else if (connections.count(event)) {
    ConnInfo ci = connections[event];

    try {
      // Process data from an active connection

      WSANETWORKEVENTS network_events;
      long eventMask;

      // Fetch why this event notification triggered
      if (WSAEnumNetworkEvents(ci.sock->getFd(), event, &network_events) == SOCKET_ERROR)
        throw core::socket_error("Unable to get WSAEnumNetworkEvents:%u", WSAGetLastError());

      // Cancel event notification for this socket
      if (WSAEventSelect(ci.sock->getFd(), event, 0) == SOCKET_ERROR)
        throw core::socket_error("unable to disable WSAEventSelect:%u", WSAGetLastError());

      // Reset the event object
      WSAResetEvent(event);


      // Call the socket server to process the event
      if (network_events.lNetworkEvents & FD_WRITE) {
        ci.server->processSocketWriteEvent(ci.sock);
        if (ci.sock->isShutdown()) {
          remSocket(ci.sock);
          return;
        }
      }
      if (network_events.lNetworkEvents & (FD_READ | FD_CLOSE)) {
        ci.server->processSocketReadEvent(ci.sock);
        if (ci.sock->isShutdown()) {
          remSocket(ci.sock);
          return;
        }
      }

      // Re-instate the required socket event
      // If the read event is still valid, the event object gets set here
      eventMask = FD_READ | FD_CLOSE;
      if (ci.sock->outStream().hasBufferedData())
        eventMask |= FD_WRITE;
      if (WSAEventSelect(ci.sock->getFd(), event, eventMask) == SOCKET_ERROR)
        throw core::socket_error("unable to re-enable WSAEventSelect:%u", WSAGetLastError());
    } catch (std::exception& e) {
      vlog.error("%s", e.what());
      remSocket(ci.sock);
    }
  }
}
