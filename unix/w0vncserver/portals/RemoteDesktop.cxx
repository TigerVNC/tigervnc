#include "RemoteDesktop.h"
#include "Portal.h"
#include "glib.h"
#include "../WDesktop.h"
#include "../PortalBackend.h"

#include <core/LogWriter.h>
#include <core/string.h>

#include <assert.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <sys/select.h>
#include <vector>

static core::LogWriter vlog("RemoteDesktop");

RemoteDesktop::RemoteDesktop(PortalBackend* parent)
  : Portal(), clipboardEnabled(false), oldButtonMask(0), parent_(parent)
{
  GError* error = nullptr;

  proxy_ = g_dbus_proxy_new_sync(connection_,
                                 G_DBUS_PROXY_FLAGS_NONE,
                                 nullptr,
                                 "org.freedesktop.portal.Desktop",
                                 "/org/freedesktop/portal/desktop",
                                 "org.freedesktop.portal.RemoteDesktop",
                                 nullptr,
                                 &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }
}

RemoteDesktop::~RemoteDesktop()
{
  if (proxy_)
    g_object_unref(proxy_);

  for (rd_stream* s : streams_)
    delete s;

}

void RemoteDesktop::init()
{
  create_session();
}

void RemoteDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  GVariantBuilder options_builder;
  GVariant* params;
  uint32_t state;


  // FIXME: ignore keycode for now?
  (void)keycode;

  state = down ? 1 : 0;

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", parent_->session_handle(),
                         &options_builder, keysym, state);
  g_dbus_proxy_call(proxy_,
                    "NotifyKeyboardKeysym",
                    params,
                    G_DBUS_CALL_FLAGS_NONE,
                    3000, /* timeout */
                    nullptr, /* cancellable */
                    (GAsyncReadyCallback)(on_call_cb),
                    nullptr);
}

static int get_input_code(uint32_t button)
{
  switch (button) {
  case 0x0:
    return BTN_LEFT;
  case 0x1:
    return BTN_MIDDLE;
  case 0x2:
    return BTN_RIGHT;
  case 0x7:
    return BTN_SIDE;
  case 0x8:
    return BTN_EXTRA;
  default:
    throw std::runtime_error("Unknown mouse button");
  }
}

void RemoteDesktop::pointerEvent(int x, int y, uint16_t buttonMask)
{
  GVariantBuilder options_builder;
  GVariant* params;
  rd_stream* stream;
  uint32_t pw_node_id;

  // FIXME: Handle multiple screens
  stream = streams_[0];
  pw_node_id = stream->pw_node_id;

  std::string str_x = std::to_string(x);
  std::string str_y = std::to_string(y);

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}udd)", parent_->session_handle(),
                         &options_builder, pw_node_id,
                         (double)x,(double)y);

  g_dbus_proxy_call(proxy_,
                    "NotifyPointerMotionAbsolute",
                    params,
                    G_DBUS_CALL_FLAGS_NONE,
                    3000, /* timeout */
                    nullptr, /* cancellable */
                    (GAsyncReadyCallback)(on_call_cb),
                    nullptr);

  if (buttonMask == oldButtonMask)
    return;

  handle_pointerEvent(buttonMask);
}

void RemoteDesktop::handle_pointerEvent(uint16_t buttonMask)
{
  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        handle_scroll_wheel(i);
      else
        handle_button_press(buttonMask, i);
    }
  }
  oldButtonMask = buttonMask;
}

void RemoteDesktop::handle_button_press(uint16_t buttonMask,
                                        int32_t button)
{
  GVariantBuilder options_builder;
  GVariant* params;
  uint32_t down;

  down = buttonMask & (1 << button);
  button = get_input_code(button);

  g_variant_builder_init(&options_builder,
                         G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", parent_->session_handle(),
                          &options_builder, button, (uint32_t)down);

  g_dbus_proxy_call(proxy_,
                    "NotifyPointerButton",
                    params,
                    G_DBUS_CALL_FLAGS_NONE,
                    3000, /* timeout */
                    nullptr, /* cancellable */
                    (GAsyncReadyCallback)(on_call_cb),
                    nullptr);
}

void RemoteDesktop::handle_scroll_wheel(int32_t button)
{
  assert(button > 2 && button < 7);

  GVariantBuilder options_builder;
  GVariant* params;

  int32_t axis;
  int steps;

  switch (button) {
  case WHEEL_VERTICAL_DOWN:
    axis = 0;
    steps = -1;
    break;
  case WHEEL_VERTICAL_UP:
    axis = 0;
    steps = 1;
    break;
  case WHEEL_HORIZONTAL_LEFT:
    axis = 1;
    steps = -1;
    break;
  case WHEEL_HORIZONTAL_RIGHT:
    axis = 1;
    steps = 1;
    break;
  default:
    assert(false);
  }

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}ui)", parent_->session_handle(),
                         &options_builder, axis, steps);

  g_dbus_proxy_call(proxy_,
                    "NotifyPointerAxisDiscrete",
                    params,
                    G_DBUS_CALL_FLAGS_NONE,
                    3000, /* timeout */
                    nullptr, /* cancellable */
                    (GAsyncReadyCallback)(on_call_cb),
                    nullptr);
}

void RemoteDesktop::create_session()
{
  assert(proxy_);

  char* handle_token;
  char* session_handle_token;
  GVariantBuilder options_builder;
  char* request_handle;

  new_request_handle(&request_handle, &handle_token);
  session_handle_token = new_session_handle();

  vlog.debug("handle_token: %s", handle_token);
  vlog.debug("session_handle_token: %s", session_handle_token);

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&options_builder, "{sv}", "handle_token",
                        g_variant_new_string(handle_token));
  g_variant_builder_add(&options_builder, "{sv}", "session_handle_token",
                        g_variant_new_string(session_handle_token));

  vlog.debug("request_handle: %s", request_handle);

  signal_subscribe(request_handle, on_create_session, parent_);
  g_dbus_proxy_call(proxy_,
                    "CreateSession",
                    g_variant_new("(a{sv})", &options_builder),
                    G_DBUS_CALL_FLAGS_NONE,
                    3000,
                    nullptr,
                    (GAsyncReadyCallback)on_call_cb,
                    nullptr);

}

void RemoteDesktop::on_create_session(GDBusConnection *connection,
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
  PortalBackend* parent;
  GVariant* results;
  uint32_t response;
  GVariant* session_handle;

  parent = (PortalBackend*)user_data;
  assert(parent);

  vlog.debug("on_create_session()");

  g_variant_get(parameters, "(u@a{sv})", &response, &results);

  if (response == 1) {
    throw std::runtime_error("Error: CreateSession was cancelled");
  } else if (response == 2) {
    throw std::runtime_error("Error: CreateSession failed");
  }

  session_handle = g_variant_lookup_value(results,
                                          "session_handle",
                                          G_VARIANT_TYPE_STRING);

  parent->set_session_handle(
    strdup(g_variant_get_string(session_handle, nullptr)));

  vlog.debug("session_handle: %s", parent->session_handle());


  parent->select_devices();
}

void RemoteDesktop::select_devices()
{
  assert(proxy_);

  char* handle_token;
  char* request_handle;
  GVariantBuilder options_builder;
  GVariant *params;


  new_request_handle(&request_handle, &handle_token);

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&options_builder, "{sv}", "types",
                        g_variant_new_uint32(DEV_KEYBOARD | DEV_POINTER));
  g_variant_builder_add(&options_builder, "{sv}", "persist_mode",
                        g_variant_new_uint32(0));

  // FIXME: Do we want restore_token?

  g_variant_builder_add(&options_builder, "{sv}", "handle_token",
                        g_variant_new_string(handle_token));

  params = g_variant_new("(oa{sv})", parent_->session_handle(), &options_builder);

  signal_subscribe(request_handle, on_select_devices, parent_);

  g_dbus_proxy_call(proxy_,
    "SelectDevices",
    params,
    G_DBUS_CALL_FLAGS_NONE,
    3000, /* timeout */
    nullptr, /* cancellable */
    (GAsyncReadyCallback)(on_call_cb),
    nullptr);

}

void RemoteDesktop::on_select_devices(GDBusConnection *connection,
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
  vlog.debug("on_select_devices()");

  parent = (PortalBackend*)user_data;
  assert(parent);

  parent->select_sources();
}

void RemoteDesktop::start()
{
  char* handle_token;
  char* request_handle;
  GVariantBuilder options_builder;
  GVariant *params;

  assert(proxy_);

  new_request_handle(&request_handle, &handle_token);

  g_variant_builder_init(&options_builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&options_builder, "{sv}", "handle_token",
                        g_variant_new_string(handle_token));

  params = g_variant_new("(osa{sv})", parent_->session_handle(), "",
                         &options_builder);

  signal_subscribe(request_handle, on_start, this);

  g_dbus_proxy_call(proxy_,
    "Start",
    params,
    G_DBUS_CALL_FLAGS_NO_AUTO_START,
    3000, /* timeout */
    nullptr, /* cancellable */
    (GAsyncReadyCallback)(on_call_cb),
    nullptr);
}

void RemoteDesktop::on_start(GDBusConnection *connection,
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
  RemoteDesktop* self;
  uint32_t response_code;
  GVariant* result;
  GVariant* devices;
  GVariant* streams;
  std::vector<uint32_t> pipewire_ids;

  self = (RemoteDesktop*)user_data;
  assert(self);

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    // FIXME: Do we need to free some resources here as well?
    throw std::runtime_error(
      core::format("Unexpected parameters type %s",
                    g_variant_get_type_string(parameters)));
  }


  g_variant_get(parameters, "(u@a{sv})", &response_code, &result);

  if (response_code == 1) {
    throw std::runtime_error("RemoteDesktop.Start cancelled");
  } if (response_code == 2) {
    throw std::runtime_error("RemoteDesktop.Start failed");
  }

  streams = g_variant_lookup_value(result, "streams",
            G_VARIANT_TYPE_ARRAY);

  vlog.debug("streams: %s", g_variant_print(streams, true));

  devices = g_variant_lookup_value(result, "devices", G_VARIANT_TYPE_UINT32);

  vlog.debug("devices: %s", g_variant_print(devices, true));

  self->parse_streams(streams);

  g_variant_unref(result);

}

void RemoteDesktop::parse_streams(GVariant *streams)
{
  std::vector<rd_stream*> rdp_streams;
  GVariantIter iter;
  int n_streams;

  g_variant_iter_init(&iter, streams);
  n_streams = g_variant_iter_n_children(&iter);

  assert(n_streams > 0);

  for (int i = 0; i < n_streams; i++) {
    GVariant* stream;
    rd_stream* s;
    uint32_t pw_id;

    stream = g_variant_get_child_value(streams, i);

    s = parse_stream(stream);

    g_variant_get_child(stream, 0, "u", &pw_id);
    s->pw_node_id = pw_id;

    rdp_streams.push_back(s);
  }

  assert(!rdp_streams.empty());

  streams_ = rdp_streams;

  parent_->open_pipewire_remote();
}

rd_stream* RemoteDesktop::parse_stream(GVariant *stream) {

  GVariant* dictionary;
  const uint32_t *key; // unused? What is even this
  const char *id; // unused
  uint32_t source_type;
  int32_t x;
  int32_t y;
  int32_t width;
  uint32_t height;
  const char* mapping_id;

  rd_stream* s;

  g_variant_get(stream, "(u@a{sv})", &key, &dictionary);

  // None of these are *strictly* necessary when selecting a single
  // *real* monitor

  if (!dictionary || !g_variant_is_of_type(dictionary, G_VARIANT_TYPE("a{sv}")))
    throw std::runtime_error("Failed to parse dictionary from stream");

  if (!g_variant_lookup(dictionary, "id", "&s", &id)) {
    vlog.debug("Failed to parse 'id'");
    id = nullptr;
  }

  if (!g_variant_lookup(dictionary, "source_type", "u", &source_type))
    vlog.debug("Failed to parse 'source_type'");

  if (!g_variant_lookup(dictionary, "position", "(ii)", &x, &y)) {
    vlog.debug("Failed to parse 'position'");
    x = -1;
    y = -1;
  }

  if (!g_variant_lookup(dictionary, "size", "(ii)", &width, &height))
    vlog.debug("Failed to parse 'size'");

  if (!g_variant_lookup(dictionary, "mapping_id", "&s", &mapping_id)) {
    vlog.debug("Failed to parse 'mapping_id'");
    mapping_id = nullptr;
  }

  s = new rd_stream();
  s->id = id;
  s->source_type = source_type;
  s->x = x;
  s->y = y;
  s->width = width;
  s->height = height;
  s->mapping_id = mapping_id;

  g_variant_unref(dictionary);

  return s;
}
