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

#include <stdint.h>
#include <assert.h>
#include <sys/select.h>
#include <linux/input-event-codes.h>

#include <glib.h>
#include <gio/gunixfdlist.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "../w0vncserver.h"
#include "portalConstants.h"
#include "RemoteDesktop.h"
#include "PortalProxy.h"

// Maximum number of buttons
#define BUTTONS 9

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

// This feature is disabled by default as it doesn't work properly on
// GNOME. https://gitlab.gnome.org/GNOME/mutter/-/issues/4135
core::BoolParameter localCursor("experimentalPortalLocalCursor",
                                "[EXPERIMENTAL] Render cursor locally",
                                false);

static core::LogWriter vlog("RemoteDesktop");

static int getInputCode(uint32_t button)
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
    assert(false);
  }
}

RemoteDesktop::RemoteDesktop(std::function<void(int fd, uint32_t nodeId)>
                               startPipewireCb_,
                             std::function<void(const char*)>
                               cancelStartCb_)
  : sessionStarted(false), oldButtonMask(0), selectedDevices(0),
    sessionHandle(""), remoteDesktop(nullptr), screenCast(nullptr),
    session(nullptr), startPipewireCb(startPipewireCb_),
    cancelStartCb(cancelStartCb_)
{
  remoteDesktop = new PortalProxy("org.freedesktop.portal.Desktop",
                                  "/org/freedesktop/portal/desktop",
                                  "org.freedesktop.portal.RemoteDesktop");
  screenCast = new PortalProxy("org.freedesktop.portal.Desktop",
                               "/org/freedesktop/portal/desktop",
                               "org.freedesktop.portal.ScreenCast");
}

RemoteDesktop::~RemoteDesktop()
{
  closeSession();
  delete session;
  delete screenCast;
  delete remoteDesktop;
}

void RemoteDesktop::notifyKeyboardKeysym(uint32_t keysym, bool down)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  uint32_t state;

  if (!(selectedDevices & DEV_KEYBOARD))
    return;

  state = down ? 1 : 0;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", sessionHandle.c_str(),
                         &optionsBuilder, keysym, state);
  remoteDesktop->call("NotifyKeyboardKeysym", params);
}

void RemoteDesktop::notifyKeyboardKeycode(uint32_t keycode, bool down)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  uint32_t state;

  if (!(selectedDevices & DEV_KEYBOARD))
    return;

  state = down ? 1 : 0;

  // FIXME: We should probably verify that xkv_v1 is used, but it seems
  // like most compositors (mutter/kwin etc.) use xkb_v1.
  // The Wayland documentation states that:
  //   "clients must add 8 to the key event keycode"
  // when format xkb_v1 is used. Subtract 8 to get the correct keycode.
  keycode -= 8;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", sessionHandle.c_str(),
                         &optionsBuilder, keycode, state);
  remoteDesktop->call("NotifyKeyboardKeycode", params);
}

void RemoteDesktop::notifyPointerMotionAbsolute(int x, int y,
                                                uint16_t buttonMask)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;

  if (!(selectedDevices & DEV_POINTER))
    return;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}udd)", sessionHandle.c_str(),
                         &optionsBuilder, pipewireNodeId,
                         (double)x,(double)y);

  remoteDesktop->call("NotifyPointerMotionAbsolute", params);

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        notifyPointerAxisDiscrete(i);
      else
        notifyPointerButton(i, buttonMask & (1 << i));
    }
  }
  oldButtonMask = buttonMask;
}

void RemoteDesktop::notifyPointerButton(int32_t button, bool down)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;

  assert(selectedDevices & DEV_POINTER);

  button = getInputCode(button);

  g_variant_builder_init(&optionsBuilder,
                         G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", sessionHandle.c_str(),
                          &optionsBuilder, button, down);

  remoteDesktop->call("NotifyPointerButton", params);
}

void RemoteDesktop::notifyPointerAxisDiscrete(int32_t button)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  int32_t axis;
  int steps;

  assert(button > 2 && button < 7);
  assert(selectedDevices & DEV_POINTER);

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

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}ui)", sessionHandle.c_str(),
                         &optionsBuilder, axis, steps);

  remoteDesktop->call("NotifyPointerAxisDiscrete", params);
}

void RemoteDesktop::createSession()
{
  std::string requestHandleToken;
  std::string sessionHandleToken;
  GVariantBuilder optionsBuilder;
  GVariant* params;

  requestHandleToken = remoteDesktop->newHandle();
  sessionHandleToken = remoteDesktop->newHandle();

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(requestHandleToken.c_str()));
  g_variant_builder_add(&optionsBuilder, "{sv}", "session_handle_token",
                        g_variant_new_string(sessionHandleToken.c_str()));

  params = g_variant_new("(a{sv})", &optionsBuilder);

  remoteDesktop->call("CreateSession", params,
                      requestHandleToken.c_str(),
                      std::bind(&RemoteDesktop::handleCreateSession,
                                this, std::placeholders::_1));
}

void RemoteDesktop::closeSession()
{
  if (session && sessionStarted)
    session->call("Close", nullptr, nullptr, nullptr);
}

void RemoteDesktop::selectDevices()
{
  std::string requestHandleToken;
  GVariantBuilder optionsBuilder;
  GVariant *params;

  requestHandleToken = remoteDesktop->newHandle();

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "types",
                        g_variant_new_uint32(DEV_KEYBOARD | DEV_POINTER));
  g_variant_builder_add(&optionsBuilder, "{sv}", "persist_mode",
                        g_variant_new_uint32(0));

  // FIXME: Do we want restore_token?

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(requestHandleToken.c_str()));

  params = g_variant_new("(oa{sv})", sessionHandle.c_str(), &optionsBuilder);

  remoteDesktop->call("SelectDevices", params, requestHandleToken.c_str(),
                      std::bind(&RemoteDesktop::handleSelectDevices,
                                this, std::placeholders::_1));
}

void RemoteDesktop::selectSources()
{
  GVariant* params;
  GVariantBuilder optionsBuilder;
  std::string requestHandleToken;
  int cursorMode;

  requestHandleToken = screenCast->newHandle();

  cursorMode = localCursor ? CUR_METADATA : CUR_EMBEDDED;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "types",
                        g_variant_new_uint32(SRC_MONITOR));
  g_variant_builder_add(&optionsBuilder, "{sv}", "cursor_mode",
                        g_variant_new_uint32(cursorMode));
  g_variant_builder_add(&optionsBuilder, "{sv}", "multiple",
                        g_variant_new_boolean(false));

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(requestHandleToken.c_str()));

  params = g_variant_new("(oa{sv})", sessionHandle.c_str(),
                         &optionsBuilder);

  screenCast->call("SelectSources", params, requestHandleToken.c_str(),
                   std::bind(&RemoteDesktop::handleSelectSources,
                             this, std::placeholders::_1));
}

void RemoteDesktop::start()
{
  std::string requestHandleToken;
  GVariantBuilder optionsBuilder;
  GVariant *params;

  requestHandleToken = remoteDesktop->newHandle();

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(requestHandleToken.c_str()));

  params = g_variant_new("(osa{sv})", sessionHandle.c_str(), "",
                         &optionsBuilder);

  remoteDesktop->call("Start", params, requestHandleToken.c_str(),
                      std::bind(&RemoteDesktop::handleStart,
                                this, std::placeholders::_1));
}

void RemoteDesktop::openPipewireRemote()
{
  GVariantBuilder optionsBuilder;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);

  g_dbus_proxy_call_with_unix_fd_list(
    screenCast->getProxy(),
    "OpenPipeWireRemote",
    g_variant_new("(oa{sv})",
    sessionHandle.c_str(),
    &optionsBuilder),
    G_DBUS_CALL_FLAGS_NONE,
    3000,
    nullptr,
    nullptr,
    [](GObject* proxy, GAsyncResult *res, void *userData) {
      RemoteDesktop* self = (RemoteDesktop*)(userData);
      self->handleOpenPipewireRemote(proxy, res);
    },
    this);
}

void RemoteDesktop::handleCreateSession(GVariant *parameters)
{
  GVariant* result;
  uint32_t response;
  GVariant* sessionHandleVariant;

  assert(sessionHandle.empty());

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    fatal_error("%s", core::format("RemoteDesktop::handleCreateSession: Unexpected parameters: %s",
                g_variant_print(parameters, true)).c_str());
    return;
  }

  g_variant_get(parameters, "(u@a{sv})", &response, &result);

  if (response == 1)  {
    cancelStartCb("RemoteDesktop::handleCreateSession(): RemoteDesktop.CreateSession was cancelled");
    return;
  }
  else if (response == 2) {
    cancelStartCb("RemoteDesktop::handleCreateSession(): RemoteDesktop.CreateSession failed");
    return;
  }

  sessionHandleVariant = g_variant_lookup_value(result, "session_handle",
                                                G_VARIANT_TYPE_STRING);
  g_variant_unref(result);

  sessionHandle = g_variant_get_string(sessionHandleVariant, nullptr);
  g_variant_unref(sessionHandleVariant);

  // This proxy is used to to close the session.
  session = new PortalProxy("org.freedesktop.portal.Desktop",
                            sessionHandle.c_str(),
                            "org.freedesktop.portal.Session");

  selectDevices();
}

void RemoteDesktop::handleStart(GVariant* parameters)
{
  uint32_t responseCode;
  GVariant* result;
  GVariant* streams_;
  GVariant* devices;

  assert(!sessionStarted);

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    fatal_error("%s", core::format("RemoteDesktop::handleStart: Unexpected parameters %s",
                                   g_variant_get_type_string(parameters)).c_str());
    return;
  }

  g_variant_get(parameters, "(u@a{sv})", &responseCode, &result);

  if (responseCode == 1) {
    g_variant_unref(result);
    cancelStartCb("RemoteDesktop::handleStart(): Local user denied the connection.");
    return;
  } else if (responseCode == 2) {
    g_variant_unref(result);
    cancelStartCb("RemoteDesktop::handleStart(): RemoteDesktop.Start failed");
    return;
  }

  sessionStarted = true;

  devices = g_variant_lookup_value(result, "devices", G_VARIANT_TYPE_UINT32);
  selectedDevices = g_variant_get_uint32(devices);
  g_variant_unref(devices);

  streams_ = g_variant_lookup_value(result, "streams",
                                    G_VARIANT_TYPE_ARRAY);
  g_variant_unref(result);

  if (!parseStreams(streams_)) {
    g_variant_unref(streams_);
    fatal_error("Failed to parse streams");
    return;
  }

  openPipewireRemote();
  g_variant_unref(streams_);
}

void RemoteDesktop::handleSelectSources(GVariant* /* parameters */)
{
  start();
}

void RemoteDesktop::handleSelectDevices(GVariant* /* parameters */)
{
  selectSources();
}

void RemoteDesktop::handleOpenPipewireRemote(GObject *proxy,
                                             GAsyncResult *res)
{
  GError* error = nullptr;
  GUnixFDList* fdList = nullptr;
  GVariant* response;
  int32_t fdIndex;
  int fd;

  assert(G_IS_DBUS_PROXY(proxy));

  response = g_dbus_proxy_call_with_unix_fd_list_finish(G_DBUS_PROXY(proxy),
                                                        &fdList, res,
                                                        &error);

  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    fatal_error("%s", msg.c_str());
    return;
  }

  if (!g_variant_is_of_type(response, G_VARIANT_TYPE("(h)"))) {
    g_variant_unref(response);
    fatal_error("%s", core::format("RemoteDesktop.handleOpenPipewireRemote: invalid response type: %s, expected (h)",
                                   g_variant_get_type_string(response)).c_str());
    return;
  }

  g_variant_get(response, "(h)", &fdIndex);
  g_variant_unref(response);

  fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  g_object_unref(fdList);

  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    fatal_error("%s", core::format("ScreenCast: error getting fd list:  %s",
                                   msg.c_str()).c_str());
    return;
  }

  // FIXME: Handle multiple streams
  startPipewireCb(fd, pipewireNodeId);
}

bool RemoteDesktop::parseStreams(GVariant* streams)
{
  GVariantIter iter;
  GVariant* stream;
  int n_streams;

  g_variant_iter_init(&iter, streams);
  n_streams = g_variant_iter_n_children(&iter);

  if (n_streams  < 1) {
    fatal_error("RemoteDesktop.Start: could not find streams to parse");
    return false;
  }

  if (n_streams != 1) {
    vlog.error("RemoteDesktop.Start: only one stream supported, got %d",
               n_streams);
  }

  stream = g_variant_get_child_value(streams, 0);
  g_variant_get_child(stream, 0, "u", &pipewireNodeId);

  g_variant_unref(stream);

  return true;
}
