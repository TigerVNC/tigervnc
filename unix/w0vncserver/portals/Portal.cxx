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
#include "Portal.h"

static core::LogWriter vlog("Portal");

static uint32_t portalsCounter = 0;

Portal::Portal(std::string name, std::string objectPath,
               std::string interfaceName)
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
                                nullptr, name.c_str(),
                                objectPath.c_str(),
                                interfaceName.c_str(), nullptr,
                                &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  portalsCounter++;
}

Portal::~Portal() {
  if (proxy)
    g_object_unref(proxy);

  if (connection) {
    for (uint32_t pendingCall : pendingCalls)
      g_dbus_connection_signal_unsubscribe(connection, pendingCall);

    if (--portalsCounter == 0)
      g_dbus_connection_close_sync(connection, nullptr, nullptr);
    g_object_unref(connection);
  }
}

void Portal::call(std::string method, GVariant* parameters,
                  std::string token,
                  std::function<void(GVariant* parameters)> signalCallback,
                  GDBusCallFlags flags)
{
  struct PortalCall {
    std::function<void(GVariant* parameters)> callback;
    uint32_t signalId;
    Portal* parent;
  };

  std::string request;

  request = token.empty() ? "" : newRequestHandle(token);

  if (signalCallback) {
    PortalCall* call;
    call = new PortalCall{signalCallback, 0, this};

    call->signalId = g_dbus_connection_signal_subscribe(
                       connection,
                       "org.freedesktop.portal.Desktop",
                       "org.freedesktop.portal.Request",
                       "Response",
                       request.c_str(),
                       nullptr,
                       G_DBUS_SIGNAL_FLAGS_NONE,
                       [](GDBusConnection*, const char*, const char*,
                          const char*, const char*,
                          GVariant *parameters_, void* userData) {
                            PortalCall* call_ = (PortalCall*)userData;
                            Portal* self = call_->parent;

                            if (call_->signalId > 0) {
                              g_dbus_connection_signal_unsubscribe(self->connection,
                                                                   call_->signalId);
                              self->pendingCalls.remove(call_->signalId);
                            }

                            if(call_->callback)
                              call_->callback(parameters_);
                       },
                       call, [](void* userData) {
                         delete (PortalCall*)userData;
                       });

    pendingCalls.push_back(call->signalId);
  }

  g_dbus_proxy_call(proxy, method.c_str(), parameters, flags, 3000,
                    nullptr,
                    [](GObject *source, GAsyncResult* res, void*) {
                        GError *error = nullptr;
                        GVariant *response;

                        assert(G_IS_DBUS_PROXY(source));

                        response = g_dbus_proxy_call_finish(G_DBUS_PROXY(source),
                                                            res, &error);
                        if (!error)
                          return g_variant_unref(response);

                        std::string msg(error->message);
                        g_error_free(error);
                        fatal_error(msg.c_str());
                    },
                    nullptr);
}

GVariant* Portal::callSync(std::string busName, std::string objectPath,
                           std::string interface, std::string method,
                           GError** error, GVariant* parameters,
                           const GVariantType* replyType,
                           GDBusCallFlags flags,
                           int timeout, GCancellable* cancellable)
{
  return  g_dbus_connection_call_sync(connection, busName.c_str(),
                                      objectPath.c_str(),
                                      interface.c_str(),
                                      method.c_str(), parameters,
                                      replyType, flags, timeout,
                                      cancellable, error);
}

std::string Portal::newToken()
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

std::string Portal::newHandle(std::string token)
{
  std::string handle = core::format("w0vncserver_%s", token.c_str()).c_str();
  return handle;
}

bool Portal::interfacesAvailable(std::vector<std::string> interfaces)
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

  g_dbus_connection_close_sync(connection, nullptr, nullptr);
  g_object_unref(connection);

  return !interfaceMissing;
}

std::string Portal::newRequestHandle(std::string token)
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

  requestHandle = core::format("/org/freedesktop/portal/desktop/request/%s/w0vncserver_%s",
                               uniqueName.c_str(), token.c_str());

  return requestHandle;
}
