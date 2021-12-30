/* Copyright (C) 2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winvnc/ManagedListener.h>
#include <rfb/LogWriter.h>

using namespace winvnc;
using namespace rfb;
using namespace win32;

static LogWriter vlog("ManagedListener");


ManagedListener::ManagedListener(SocketManager* mgr)
: filter(0), manager(mgr), addrChangeNotifier(0), server(0), port(0), localOnly(false) {
}

ManagedListener::~ManagedListener() {
  if (!sockets.empty()) {
    std::list<network::SocketListener*>::iterator iter;
    for (iter = sockets.begin(); iter != sockets.end(); ++iter)
      manager->remListener(*iter);
    sockets.clear();
  }
  delete filter;
}


void ManagedListener::setServer(network::SocketServer* svr) {
  if (svr == server)
    return;
  vlog.info("set server to %p", svr);
  server = svr;
  refresh();
}

void ManagedListener::setPort(int port_, bool localOnly_) {
  if ((port_ == port) && (localOnly == localOnly_))
    return;
  vlog.info("set port to %d", port_);
  port = port_;
  localOnly = localOnly_;
  refresh();
}

void ManagedListener::setFilter(const char* filterStr) {
  vlog.info("set filter to %s", filterStr);
  delete filter;
  filter = new network::TcpFilter(filterStr);
  if (!sockets.empty() && !localOnly) {
    std::list<network::SocketListener*>::iterator iter;
    for (iter = sockets.begin(); iter != sockets.end(); ++iter)
      (*iter)->setFilter(filter);
  }
}

void ManagedListener::setAddressChangeNotifier(SocketManager::AddressChangeNotifier* acn) {
  if (acn == addrChangeNotifier)
    return;
  addrChangeNotifier = acn;
  refresh();
}

bool ManagedListener::isListening() {
  return !sockets.empty();
}

void ManagedListener::refresh() {
  std::list<network::SocketListener*>::iterator iter;
  if (!sockets.empty()) {
    for (iter = sockets.begin(); iter != sockets.end(); ++iter)
      manager->remListener(*iter);
    sockets.clear();
  }
  if (!server)
    return;
  try {
    if (port) {
      if (localOnly)
        network::createLocalTcpListeners(&sockets, port);
      else
        network::createTcpListeners(&sockets, NULL, port);
    }
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
  }
  if (!sockets.empty()) {
    if (!localOnly) {
      for (iter = sockets.begin(); iter != sockets.end(); ++iter)
        (*iter)->setFilter(filter);
    }
    try {
      for (iter = sockets.begin(); iter != sockets.end(); ++iter)
        manager->addListener(*iter, server, addrChangeNotifier);
    } catch (...) {
      std::list<network::SocketListener*>::iterator iter2;
      for (iter2 = sockets.begin(); iter2 != iter; ++iter2)
        manager->remListener(*iter2);
      for (; iter2 != sockets.end(); ++iter2)
        delete *iter;
      sockets.clear();
      throw;
    }
  }
}
