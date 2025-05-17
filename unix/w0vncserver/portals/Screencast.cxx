#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <stdexcept>

#include <gio/gio.h>
#include <glib.h>
#include "glib-object.h"

#include <core/LogWriter.h>
#include <core/string.h>

#include "Screencast.h"
#include "RemoteDesktop.h"
#include "../PortalBackend.h"

core::BoolParameter localcursor("localcursor", "Render cursor locally",
                                 false);

static core::LogWriter vlog("Screencast");

Screencast::Screencast(GDBusConnection* connection,
                       PortalBackend* remote)
: proxy_(nullptr),
  parent_(remote)
{
  GError* error = nullptr;
  proxy_ = g_dbus_proxy_new_sync(connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 nullptr,
                                 "org.freedesktop.portal.Desktop",
                                 "/org/freedesktop/portal/desktop",
                                 "org.freedesktop.portal.ScreenCast",
                                 nullptr,
                                 &error);
  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }
}

Screencast::~Screencast()
{
  if (proxy_)
    g_object_unref(proxy_);
}

void Screencast::open_pipewire_remote()
{
  GVariantBuilder options_builder;

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);

  g_dbus_proxy_call_with_unix_fd_list(proxy_,
                                     "OpenPipeWireRemote",
                                      g_variant_new("(oa{sv})",
                                      parent_->session_handle(),
                                      &options_builder),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      3000,
                                      nullptr, /* fd_list */
                                      nullptr, /* cancellable */
                                      (GAsyncReadyCallback)
                                      on_open_pipewire_remote,
                                      parent_);
}

void Screencast::on_open_pipewire_remote(GDBusProxy *proxy,
                                         GAsyncResult *res,
                                         void *user_data)
{
  PortalBackend *parent = (PortalBackend*)user_data;
  GError *error = nullptr;
  GUnixFDList *fd_list = nullptr;
  GVariant *response;
  int32_t fd_index;
  int fd;

  vlog.debug("on_open_pipewire_remote()");

  assert(parent);

  response = g_dbus_proxy_call_with_unix_fd_list_finish(proxy, &fd_list,
                                                        res, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  if (!g_variant_is_of_type(response, G_VARIANT_TYPE("(h)"))) {
    g_free(response);
    throw std::runtime_error(core::format(
      "Screencast: invalid response type: %s, expected (h)",
      g_variant_get_type_string(response)));
  }

  g_variant_get(response, "(h)", &fd_index);

  fd = g_unix_fd_list_get(fd_list, fd_index, &error);
  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    g_free(response);
    throw std::runtime_error(core::format(
      "Screencast: error getting fd list:  %s", error_message.c_str()));
  }

  // FIXME: Handle multiple streams
  rd_stream* stream = parent->streams()[0];
  parent->start_pipewire(fd, stream->pw_node_id);

  // FIXME: Start listening for VNC events here
  g_free(response);
}

void Screencast::select_sources()
{
  GVariant* params;
  GVariantBuilder options_builder;
  char* handle_token;
  char* request_handle;
  int cursor_mode;

  new_request_handle(&request_handle, &handle_token);

  cursor_mode = localcursor ? CUR_METADATA : CUR_EMBEDDED;

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&options_builder, "{sv}", "types",
                        g_variant_new_uint32(SRC_MONITOR));
  g_variant_builder_add(&options_builder, "{sv}", "cursor_mode",
                        g_variant_new_uint32(cursor_mode));
  g_variant_builder_add(&options_builder, "{sv}", "multiple",
                         g_variant_new_boolean(true));

  g_variant_builder_add(&options_builder, "{sv}", "handle_token",
                        g_variant_new_string(handle_token));

  params = g_variant_new("(oa{sv})", parent_->session_handle(), &options_builder);

  if (!g_variant_is_object_path(request_handle)) {
    throw std::runtime_error("Invalid request handle");
  }

  signal_subscribe(request_handle, on_select_sources, parent_);

  g_dbus_proxy_call(proxy_,
                    "SelectSources",
                    params,
                    G_DBUS_CALL_FLAGS_NO_AUTO_START,
                    3000, /* timeout */
                    nullptr, /* cancellable */
                    (GAsyncReadyCallback)(on_call_cb),
                    nullptr);
}

void Screencast::on_select_sources(GDBusConnection *connection,
                                   const char *sender,
                                   const char *object_path,
                                   const char *interface_name,
                                   const char *signal_name,
                                   GVariant *parameters,
                                   void *user_data)
{
  (void)connection;
  (void)sender;
  (void)object_path;
  (void)interface_name;
  (void)signal_name;
  (void)parameters;
  PortalBackend* parent;

  vlog.debug("on_select_sources()");

  parent = (PortalBackend*)user_data;
  assert(parent);

  // FIXME: When Clipboard is implemented, it needs to be set up before
  // start()
  parent->start();
}
