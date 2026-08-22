/* Copyright (C) 2026 Gaurav Ujjwal.  All Rights Reserved.
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

#ifndef __NETWORK_DNS_SD_H__
#define __NETWORK_DNS_SD_H__

#include <list>
#include <string>

#include <network/Socket.h>

namespace network
{
  namespace dnssd
  {
    // Bridge between DnsSD implementation & event loop of the app
    struct WatchAdapter {

      // Called by DnsSD to get notified when given fd becomes readable/writable
      virtual void addWatch(int fd, bool read, bool write) = 0;
      virtual void removeWatch(int fd) = 0;

      // Called by the app to report when fd is ready
      // This is set by initialize()
      void (*handleReadyWatch)(int fd) = NULL;
    };

    void initialize(WatchAdapter* adapter);
    void advertise(std::string name, std::list<SocketListener*>& listeners);
    void shutdown();
  }
}

#endif
