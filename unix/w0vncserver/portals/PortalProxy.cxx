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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <uuid/uuid.h>

#include <algorithm>
#include <stdexcept>

#include <glib.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "../w0vncserver.h"
#include "PortalProxy.h"

static core::LogWriter vlog("PortalProxy");

PortalProxy::PortalProxy(const char* name, const char* objectPath,
                         const char* interfaceName)
  : proxy(nullptr)
{
  GError* error = nullptr;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
                                nullptr, name, objectPath,
                                interfaceName, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }
}

PortalProxy::~PortalProxy() {
  if (proxy)
    g_object_unref(proxy);

  if (connection) {
    for (uint32_t pendingCall : pendingCalls)
      g_dbus_connection_signal_unsubscribe(connection, pendingCall);
    for (uint32_t signalId : subscribedSignals)
      g_dbus_connection_signal_unsubscribe(connection, signalId);

    g_object_unref(connection);
  }
}

void PortalProxy::call(const char* method, GVariant* parameters,
                       const char* requestHandleToken,
                       std::function<void(GVariant* parameters)> signalCallback,
                       GDBusCallFlags flags)
{
  GError* error = nullptr;
  GVariant* result;

  struct PortalCall {
    std::function<void(GVariant* parameters)> callback;
    uint32_t signalId;
    PortalProxy* parent;
  };

  if (signalCallback) {
    assert(requestHandleToken);

    std::string requestHandle;
    requestHandle = newRequestHandle(requestHandleToken);

    PortalCall* call;
    call = new PortalCall{signalCallback, 0, this};

    call->signalId = g_dbus_connection_signal_subscribe(
                       connection,
                       "org.freedesktop.portal.Desktop",
                       "org.freedesktop.portal.Request",
                       "Response", requestHandle.c_str(),
                       nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
                       [](GDBusConnection*, const char*, const char*,
                          const char*, const char*,
                          GVariant *parameters_, void* userData) {
                            PortalCall* call_ = (PortalCall*)userData;
                            PortalProxy* self = call_->parent;

                            g_dbus_connection_signal_unsubscribe(self->connection,
                                                                 call_->signalId);
                            self->pendingCalls.remove(call_->signalId);

                            call_->callback(parameters_);
                       },
                       call, [](void* userData) {
                         delete (PortalCall*)userData;
                       });

    pendingCalls.push_back(call->signalId);
  }

  result = g_dbus_proxy_call_sync(proxy, method, parameters,
                                  flags, 3000, nullptr, &error);

  if (error) {
    // FIXME: We probably want to improve the error handling here
    vlog.error("call(): error calling %s: %s", method,
               error->message);
    g_error_free(error);
    return;
  }

  g_variant_unref(result);
}

void PortalProxy::subscribe(const char* member,
                            std::function<void(GVariant* parameters)>
                              signalCallback)
{
  struct Callback {
    std::function<void(GVariant* parameters)> callback;
  };

  Callback* callback;
  uint32_t signalId;

  callback = new Callback;
  callback->callback = signalCallback;

  signalId = g_dbus_connection_signal_subscribe(
               connection,
               "org.freedesktop.portal.Desktop",
               g_dbus_proxy_get_interface_name(proxy),
               member, "/org/freedesktop/portal/desktop",
               nullptr,
               G_DBUS_SIGNAL_FLAGS_NONE,
               [](GDBusConnection*, const char*, const char*,
                 const char*, const char*,
                 GVariant* parameters, void* userData) {
                   ((Callback*)userData)->callback(parameters);
               },
               callback, [](void* userData) {
                   delete (Callback*)userData;
               });

  subscribedSignals.push_back(signalId);
}

void PortalProxy::unsubscribe(uint32_t signalId)
{
  g_dbus_connection_signal_unsubscribe(connection, signalId);
  subscribedSignals.remove(signalId);
}

std::string PortalProxy::newToken()
{
  std::string token;
  uuid_t uuid;
  char uuidStr[37];

  uuid_generate(uuid);
  uuid_unparse(uuid, uuidStr);

  // Valid DBus object paths may only contain
  // the ASCII characters "[A-Z][a-z][0-9]_"
  for (char* p = uuidStr; *p; ++p) {
    if (*p == '-')
      *p = '_';
  }

  token = uuidStr;

  return token;
}

std::string PortalProxy::newHandle()
{
  return core::format("w0vncserver_%s", newToken().c_str());
}

bool PortalProxy::interfacesAvailable(std::vector<std::string> interfaces)
{
  GError* error = nullptr;
  GDBusConnection* connection;
  GVariant* result;
  GDBusNodeInfo* nodeInfo;
  GDBusInterfaceInfo* interfaceInfo;
  const char* introspectionXml;
  bool interfaceMissing;

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  result = g_dbus_connection_call_sync(connection,
                              "org.freedesktop.portal.Desktop",
                              "/org/freedesktop/portal/desktop",
                              "org.freedesktop.DBus.Introspectable",
                              "Introspect",
                              nullptr,
                              G_VARIANT_TYPE("(s)"),
                              G_DBUS_CALL_FLAGS_NONE,
                              3000,
                              nullptr,
                              &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error("Introspect call failed: " + error_message);
  }

  g_variant_get(result, "(&s)", &introspectionXml);

  nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
  g_variant_unref(result);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  interfaceMissing = false;
  for (std::string interface : interfaces) {
    interfaceInfo = g_dbus_node_info_lookup_interface(nodeInfo,
                                                      interface.c_str());
    if (!interfaceInfo) {
      vlog.debug("Interface '%s' not found", interface.c_str());
      interfaceMissing = true;
    }
  }

  g_dbus_node_info_unref(nodeInfo);
  g_object_unref(connection);

  return !interfaceMissing;
}

std::string PortalProxy::newRequestHandle(std::string token)
{
  std::string uniqueName;
  std::string requestHandle;

  uniqueName = g_dbus_connection_get_unique_name(connection);

  /* https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html#org-freedesktop-portal-request
   * From the documentation: the caller's unique name should have its
   * initial ':' removed, and all '.' replaced by '_'.
   * e.g. :15759.1 -> 15759_1
   */
  uniqueName.erase(0,1);
  std::replace(uniqueName.begin(), uniqueName.end(), '.', '_');

  requestHandle = core::format("/org/freedesktop/portal/desktop/request/%s/%s",
                               uniqueName.c_str(), token.c_str());

  return requestHandle;
}
