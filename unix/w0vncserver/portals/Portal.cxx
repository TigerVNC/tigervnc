#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Portal.h"
#include "gio/gio.h"
#include "glib.h"

#include <core/LogWriter.h>

#include <assert.h>
#include <stdexcept>
#include <string.h>
#include <uuid/uuid.h>

static core::LogWriter vlog("Portal");

struct PortalCall {
  ~PortalCall();
  GDBusSignalCallback callback;
  void* user_data;
  char* request_handle;
  uint32_t signal_id;
  GDBusConnection* connection;
};

PortalCall::~PortalCall()
{
  if (signal_id > 0)
    g_dbus_connection_signal_unsubscribe(connection, signal_id);

  free(request_handle);
}

Portal::Portal()
{
  GError* error = nullptr;
  const char* unique_name;
  char *clean_unque_name;

  connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  unique_name = g_dbus_connection_get_unique_name(connection_);
  clean_unque_name = (char*)malloc(strlen(unique_name));

  // Replace '.' with '_'
  for (uint i = 1; i < strlen(unique_name); i++) {
    if (unique_name[i] == '.')
      clean_unque_name[i-1] = '_';
    else
      clean_unque_name[i-1] = unique_name[i];
  }

  clean_unque_name[strlen(unique_name)-1] = '\0';

  unique_name_ = clean_unque_name;
}

Portal::~Portal() {
  if (connection_)
    g_object_unref(connection_);
  free(unique_name_);
}

void Portal::new_request_handle(char** request_handle, char** handle_token) {
  char* request_handle_;
  char* handle_token_;
  size_t len;
  uuid_t uuid;
  char uuid_str[37];

  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_str);

  // replace all "-" with "_"
  for (char *p = uuid_str; *p; ++p) {
    if (*p == '-')
      *p = '_';
  }

  // Two slashes + null terminator, hence the + 3
  len = strlen("/org/freedesktop/portal/desktop/request") +
        strlen(unique_name_) + strlen("w0vncserver_") + strlen(uuid_str) + 3;
  request_handle_ = (char*)malloc(len);

  snprintf(request_handle_, len, "%s/%s/w0vncserver_%s",
           "/org/freedesktop/portal/desktop/request", unique_name_,
           uuid_str);

  *request_handle = request_handle_;

  len = strlen("w0vncserver_") + strlen(uuid_str) + 1;
  handle_token_ = (char*)malloc(len);
  snprintf(handle_token_, len, "w0vncserver_%s", uuid_str);

  *handle_token = handle_token_;
  vlog.debug("new request handle: %s", request_handle_);
  vlog.debug("new handle token: %s", handle_token_);
}

char* Portal::new_session_handle()
{
  char* session_handle;
  size_t len;
  uuid_t uuid;
  char uuid_str[37];

  uuid_generate(uuid);
  uuid_unparse(uuid, uuid_str);

  // replace all "-" with "_"
  for (char *p = uuid_str; *p; ++p) {
    if (*p == '-')
      *p = '_';
  }

  len = strlen("w0vncserver_") + strlen(uuid_str) + 1;
  session_handle = (char*)malloc(len);
  snprintf(session_handle, len + 1, "w0vncserver_%s", uuid_str);

  vlog.debug("new session handle: %s", session_handle);

  return session_handle;
}

void Portal::signal_subscribe(const char* path, GDBusSignalCallback cb,
                              void* user_data)
{
  PortalCall* call;

  vlog.debug("signal_subscribe(%s)", path);

  call = new PortalCall();
  call->request_handle = strdup(path);
  call->callback = cb;
  call->user_data = user_data;
  call->connection = connection_;

  // FIXME: Handle cancellation

  call->signal_id = g_dbus_connection_signal_subscribe(
                      connection_,
                      "org.freedesktop.portal.Desktop",
                      "org.freedesktop.portal.Request",
                      "Response",
                      call->request_handle,
                      nullptr,
                      G_DBUS_SIGNAL_FLAGS_NONE,
                      on_signal_response,
                      call,
                      nullptr);

}

void Portal::on_signal_response(GDBusConnection *connection,
                                const char *sender_name,
                                const char *object_path,
                                const char *interface_name,
                                const char *signal_name,
                                GVariant *parameters,
                                void *user_data)
{
  PortalCall* call;

  call = (PortalCall*)user_data;
  assert(call);

  vlog.debug("on_response()");

  if(call->callback) {
    call->callback(connection, sender_name, object_path, interface_name,
                   signal_name, parameters, call->user_data);
  }

  delete call;
}

void Portal::on_call_cb(GDBusProxy *source, GAsyncResult *res,
                        void *user_data)
{
  (void) user_data;
  GError *error = nullptr;
  GVariant *response;

  vlog.debug("on_call_cb()");

  response = g_dbus_proxy_call_finish(source, res, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  g_variant_unref(response);
}