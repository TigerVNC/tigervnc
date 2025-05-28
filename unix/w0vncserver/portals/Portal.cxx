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
  ~PortalCall();
  GDBusSignalCallback callback;
  void* userData;
  char* requestHandle;
  uint32_t signalId;
  GDBusConnection* connection;
};

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
  if (connection_)
    g_object_unref(connection_);
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

void Portal::signalSubscribe(const char* path, GDBusSignalCallback cb,
                             void* userData)
{
  PortalCall* call;

  vlog.debug("signal_subscribe(%s)", path);

  call = new PortalCall();
  call->requestHandle = strdup(path);
  call->callback = cb;
  call->userData = userData;
  call->connection = connection_;

  // FIXME: Handle cancellation

  call->signalId = g_dbus_connection_signal_subscribe(
                      connection_,
                      "org.freedesktop.portal.Desktop",
                      "org.freedesktop.portal.Request",
                      "Response",
                      call->requestHandle,
                      nullptr,
                      G_DBUS_SIGNAL_FLAGS_NONE,
                      onSignalResponse,
                      call,
                      nullptr);

}

void Portal::onSignalResponse(GDBusConnection *connection,
                              const char *senderName,
                              const char *objectPath,
                              const char *interfaceName,
                              const char *signalName,
                              GVariant *parameters,
                              void *userData)
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

void Portal::onCallCb(GDBusProxy *source, GAsyncResult *res,
                      void *userData)
{
  (void) userData;
  GError *error = nullptr;
  GVariant *response;

  vlog.debug("onCallCb()");

  response = g_dbus_proxy_call_finish(source, res, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  g_variant_unref(response);
}
