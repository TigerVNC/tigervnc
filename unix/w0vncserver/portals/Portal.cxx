#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <uuid/uuid.h>

#include <stdexcept>

#include <gio/gio.h>
#include <glib.h>

#include <core/LogWriter.h>
#include <core/string.h>


#include "Portal.h"


static core::LogWriter vlog("Portal");

struct PortalCall {
  PortalCall();
  ~PortalCall();
  GDBusSignalCallback callback;
  void* userData;
  char* requestHandle;
  const char* method;
  uint32_t signalId;
  GDBusConnection* connection;
};

PortalCall::PortalCall()
  : callback(nullptr), userData(nullptr), requestHandle(nullptr),
    method(nullptr), signalId(0), connection(nullptr)
{
}

PortalCall::~PortalCall()
{
  if (signalId > 0)
    g_dbus_connection_signal_unsubscribe(connection, signalId);

  free(requestHandle);
}

Portal::Portal() : sessionHandle_(nullptr)
{
  GError* error = nullptr;
  const char* uniqueName;
  char *cleanUniqueName;

  connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  uniqueName = g_dbus_connection_get_unique_name(connection_);
  cleanUniqueName = (char*)malloc(strlen(uniqueName));

  /* https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Request.html#org-freedesktop-portal-request
   * From the documentation: the caller's unique name should have its
   * initial ':' removed, and all '.' replaced by '_'.
   * e.g. :15759.1 -> 15759_1
   */
  for (uint i = 1; i < strlen(uniqueName); i++) {
    if (uniqueName[i] == '.')
      cleanUniqueName[i-1] = '_';
    else
      cleanUniqueName[i-1] = uniqueName[i];
  }

  cleanUniqueName[strlen(uniqueName)-1] = '\0';

  uniqueName_ = cleanUniqueName;
}

Portal::~Portal() {
  if (connection_) {
    closeSession();
    g_dbus_connection_close_sync(connection_, nullptr, nullptr);
    g_object_unref(connection_);
  }
  free(uniqueName_);
}

void Portal::newRequestHandle(const char** requestHandle,
                              const char** handleToken)
{
  uuid_t uuid;
  char uuidStr[37];

  uuid_generate(uuid);
  uuid_unparse(uuid, uuidStr);

  // Valid DBus object paths may only contain
  // the ASCII characters "[A-Z][a-z][0-9]_"
  for (char *p = uuidStr; *p; ++p) {
    if (*p == '-')
      *p = '_';
  }

  *requestHandle = strdup(core::format("/org/freedesktop/portal/desktop/request/%s/w0vncserver_%s",
                          uniqueName_, uuidStr).c_str());

  *handleToken = strdup(core::format("w0vncserver_%s", uuidStr).c_str());

  vlog.debug("new request handle: %s", *requestHandle);
  vlog.debug("new handle token: %s", *handleToken);
}

const char* Portal::newSessionHandle()
{
  const char* sessionHandle;
  uuid_t uuid;
  char uuidStr[37];

  uuid_generate(uuid);
  uuid_unparse(uuid, uuidStr);

  // Valid DBus object paths may only contain
  // the ASCII characters "[A-Z][a-z][0-9]_"
  for (char *p = uuidStr; *p; ++p) {
    if (*p == '-')
      *p = '_';
  }

  sessionHandle = strdup(core::format("w0vncserver_%s", uuidStr).c_str());

  vlog.debug("new session handle: %s", sessionHandle);

  return sessionHandle;
}

void Portal::call(GDBusProxy* proxy, const char* method,
                  GVariant* parameters, GDBusCallFlags flags,
                  GDBusSignalCallback signalCallback,
                  const char* requestHandle, void* userData)
{
  PortalCall* call;

  call = new PortalCall();
  call->callback = signalCallback;
  call->userData = userData;
  call->connection = connection_;
  call->method = method;

  if (requestHandle)
    call->requestHandle = strdup(requestHandle);

  if (signalCallback) {
    call->signalId = g_dbus_connection_signal_subscribe(
                      connection_,
                      "org.freedesktop.portal.Desktop",
                      "org.freedesktop.portal.Request",
                      "Response",
                      requestHandle,
                      nullptr,
                      G_DBUS_SIGNAL_FLAGS_NONE,
                      [](GDBusConnection *connection,
                         const char *sender_name,
                         const char *object_path,
                         const char *interface_name,
                         const char *signal_name,
                         GVariant *parameters_,
                         void* user_data) {
                           Portal* self = (Portal*)user_data;
                           self->handleSignalResponse(connection,
                                                      sender_name,
                                                      object_path,
                                                      interface_name,
                                                      signal_name,
                                                      parameters_,
                                                      user_data);
                        },
                      call,
                      nullptr);
  }

  g_dbus_proxy_call(proxy, method, parameters, flags, 3000, nullptr,
                    [](GObject *source_object, GAsyncResult *res,
				               void* data) {
                      Portal* self = (Portal*)data;
                      self->handleCallCb(source_object, res, data);
                    }, userData);
}

void Portal::closeSession()
{
  GError* error = nullptr;
  g_dbus_connection_call_sync(connection_,
                              "org.freedesktop.portal.Desktop",
                              sessionHandle_,
                              "org.freedesktop.portal.Session",
                              "Close",
                              nullptr,
                              nullptr,
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              nullptr,
                              &error);
  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }
}

void Portal::handleSignalResponse(GDBusConnection *connection,
                                  const char *senderName,
                                  const char *objectPath,
                                  const char *interfaceName,
                                  const char *signalName,
                                  GVariant *parameters, void *userData)
{
  PortalCall* call;

  call = (PortalCall*)userData;
  assert(call);

  vlog.debug("onSignalResponse()");

  if(call->callback) {
    call->callback(connection, senderName, objectPath, interfaceName,
                   signalName, parameters, call->userData);
  }

  delete call;
}

void Portal::handleCallCb(GObject *source, GAsyncResult *res,
                          void *userData)
{
  (void) userData;
  GError *error = nullptr;
  GVariant *response;

  assert(G_IS_DBUS_PROXY(source));

  response = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  g_variant_unref(response);
}
