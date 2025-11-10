/* Copyright 2025 Adam Halim for Cendio AB
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

#ifndef __PORTAL_PROXY_H__
#define __PORTAL_PROXY_H__

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>
#include <list>

#include <gio/gio.h>

class PortalProxy {
public:
  PortalProxy(const char* name, const char* objectPath,
              const char* interfaceName);
  ~PortalProxy();

  // Call a DBUS method using the proxy. Will call
  // signalCallback() when a response signal is received. A
  // requestHandleToken must be set to link the response signal.
  // signalCallback can be set to nullptr is no response is expected.
  void call(const char* method, GVariant* parameters,
            const char* requestHandleToken = nullptr,
            std::function<void(GVariant* parameters)>
              signalCallback = nullptr,
            GDBusCallFlags flags = G_DBUS_CALL_FLAGS_NONE);
  // Subscribe to a DBUS signal. signalCallback() is run whenever
  // the signal is emitted
  void subscribe(const char* member,
                 std::function<void(GVariant* parameters)>
                   signalCallback);
  // Unsubscribe from a DBUS signal
  void unsubscribe(uint32_t signalId);

  // Generates a unique request token
  static std::string newToken();
  // Generates a handle with a unique token
  static std::string newHandle();

  // Checks if interfaces are available
  static bool interfacesAvailable(std::vector<std::string> interfaces);

  GDBusProxy* getProxy() const { return proxy; }

private:
  // Generates a request handle from a token.
  std::string newRequestHandle(std::string token);

private:
  GDBusConnection* connection;
  GDBusProxy* proxy;
  std::list<uint32_t> pendingCalls;
  std::list<uint32_t> subscribedSignals;
};

#endif // __PORTAL_PROXY_H__
