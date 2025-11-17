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
#include <stdio.h>
#include <assert.h>
#include <sys/select.h>
#include <uuid/uuid.h>
#include <linux/input-event-codes.h>

#include <string>

#include <glib.h>
#include <gio/gunixfdlist.h>

#include <core/Configuration.h>
#include <core/LogWriter.h>
#include <core/string.h>
#include <core/xdgdirs.h>

#include <rfb/KeysymStr.h>
#include "../w0vncserver.h"
#include "portalConstants.h"
#include "RemoteDesktop.h"
#include "Clipboard.h"
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
core::BoolParameter
  localCursor("experimentalLocalCursor",
              "[EXPERIMENTAL] Render cursor locally",
              false);

core::EnumParameter
  rememberDisplayChoice("RememberDisplayChoice",
                        "Remember display choice when user connects (Always, Never, Session)",
                        {"Always", "Never", "Session"}, "Session");

// Sync with w0vncserver-forget
static const char* RESTORE_TOKEN_FILENAME =  "restoretoken";

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

RemoteDesktop::RemoteDesktop(std::string restoreToken_,
                             std::function<void(int fd, uint32_t nodeId)>
                               startPipewireCb_,
                             std::function<void(const char*)>
                               cancelStartCb_,
                             std::function<void()> initClipboardCb_,
                             std::function<void()> clipboardSubscribeCb_)
  : sessionStarted(false), oldButtonMask(0), selectedDevices(0),
    clipboardEnabled(false), sessionHandle(""), remoteDesktop(nullptr),
    screenCast(nullptr), session(nullptr), restoreToken(restoreToken_),
    startPipewireCb(startPipewireCb_), cancelStartCb(cancelStartCb_),
    initClipboardCb(initClipboardCb_),
    clipboardSubscribeCb(clipboardSubscribeCb_)
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
  try {
    remoteDesktop->call("NotifyKeyboardKeysym", params);
  } catch (const std::exception& e) {
    vlog.error("Could not handle keysym XK_%s (0x%04x): %s",
               KeySymName(keysym), keysym, e.what());
  }
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
  try {
    remoteDesktop->call("NotifyKeyboardKeycode", params);
  } catch (const std::exception& e) {
    vlog.error("Could not handle key %d: %s", keycode, e.what());
  }
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

  try {
    remoteDesktop->call("NotifyPointerMotionAbsolute", params);
  } catch (const std::exception& e) {
    vlog.error("Could not move pointer: %s", e.what());
  }

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

  try {
    remoteDesktop->call("NotifyPointerButton", params);
  } catch (const std::exception& e) {
    vlog.error("Could not handle mouse button: %s", e.what());
  }
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

  try {
    remoteDesktop->call("NotifyPointerAxisDiscrete", params);
  } catch (const std::exception& e) {
    vlog.error("Could not handle mouse scroll: %s", e.what());
  }
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

  try {
    remoteDesktop->call("CreateSession", params,
                        requestHandleToken.c_str(),
                        std::bind(&RemoteDesktop::handleCreateSession,
                                  this, std::placeholders::_1));
  } catch (const std::exception& e) {
    vlog.error("Could not create session: %s", e.what());
    cancelStartCb("Failed to start remote desktop session");
  }
}

void RemoteDesktop::closeSession()
{
  if (session && sessionStarted) {
    try {
      session->call("Close", nullptr, nullptr, nullptr);
    } catch (const std::exception& e) {
      // This is not necessarily unexpected, as the session can be
      // closed by the compositor.
      vlog.info("Could not close session: %s", e.what());
    }
  }
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

  if (rememberDisplayChoice != "Never") {
    g_variant_builder_add(&optionsBuilder, "{sv}", "persist_mode",
                          g_variant_new_uint32(PERSIST_UNTIL_REVOKED));
  }

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(requestHandleToken.c_str()));

  if (loadRestoreToken()) {
    g_variant_builder_add(&optionsBuilder, "{sv}", "restore_token",
                          g_variant_new_string(restoreToken.c_str()));
  }

  params = g_variant_new("(oa{sv})", sessionHandle.c_str(), &optionsBuilder);

  try {
    remoteDesktop->call("SelectDevices", params, requestHandleToken.c_str(),
                        std::bind(&RemoteDesktop::handleSelectDevices,
                        this, std::placeholders::_1));
  } catch (const std::exception& e) {
    vlog.error("Could not select devices: %s", e.what());
    cancelStartCb("Failed to start remote desktop session");
  }
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

  try {
    screenCast->call("SelectSources", params, requestHandleToken.c_str(),
                     std::bind(&RemoteDesktop::handleSelectSources,
                     this, std::placeholders::_1));
  } catch (const std::exception& e) {
    vlog.error("Could not select sources: %s", e.what());
    cancelStartCb("Failed to start remote desktop session");
  }
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
  try {
    remoteDesktop->call("Start", params, requestHandleToken.c_str(),
                        std::bind(&RemoteDesktop::handleStart,
                        this, std::placeholders::_1));
  } catch (const std::exception& e) {
    vlog.error("Could not start session: %s", e.what());
    cancelStartCb("Failed to start remote desktop session");
  }
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
    vlog.error("Could not create session: unexpected parameters: %s",
               g_variant_print(parameters, true));
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  g_variant_get(parameters, "(u@a{sv})", &response, &result);

  if (response == 1)  {
    vlog.error("Session was cancelled");
    cancelStartCb("Failed to start remote desktop session");
    return;
  }
  else if (response == 2) {
    vlog.error("Failed to create session");
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  sessionHandleVariant = g_variant_lookup_value(result, "session_handle",
                                                G_VARIANT_TYPE_STRING);
  g_variant_unref(result);

  if (!sessionHandleVariant) {
    vlog.error("Failed to create session: no session handle");
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

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
  GVariant* streams;
  GVariant* devices;
  GVariant* newRestoreToken;
  GVariant* clipboardEnabled_;

  assert(!sessionStarted);

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    vlog.error("Could not start remote desktop: unexpected parameters %s",
               g_variant_get_type_string(parameters));
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  g_variant_get(parameters, "(u@a{sv})", &responseCode, &result);

  if (responseCode == 1) {
    g_variant_unref(result);
    vlog.error("Could not start remote desktop - local user denied the connection.");
    cancelStartCb("Failed to start remote desktop session");
    return;
  } else if (responseCode == 2) {
    g_variant_unref(result);
    vlog.error("Failed to start remote desktop session");
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  sessionStarted = true;

  devices = g_variant_lookup_value(result, "devices", G_VARIANT_TYPE_UINT32);

  if (!devices) {
    g_variant_unref(result);
    vlog.error("Failed to get devices");
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  selectedDevices = g_variant_get_uint32(devices);
  g_variant_unref(devices);

  streams = g_variant_lookup_value(result, "streams",
                                    G_VARIANT_TYPE_ARRAY);
  g_variant_unref(result);

  if (!streams) {
    vlog.error("Failed to start remote desktop session");
    cancelStartCb("Failed to get streams");
    return;
  }

  clipboardEnabled_ = g_variant_lookup_value(result,"clipboard_enabled",
                                             G_VARIANT_TYPE_BOOLEAN);
  if (clipboardEnabled_) {
    clipboardEnabled = g_variant_get_boolean(clipboardEnabled_);
    g_variant_unref(clipboardEnabled_);
  }

  if (clipboardEnabled)
    clipboardSubscribeCb();

  if (!parseStreams(streams)) {
    vlog.error("Failed to parse streams");
    cancelStartCb("Failed to start remote desktop session");
    g_variant_unref(streams);
    return;
  }
  g_variant_unref(streams);

  newRestoreToken = g_variant_lookup_value(result, "restore_token",
                                         G_VARIANT_TYPE_STRING);

  if (newRestoreToken) {
    storeRestoreToken(g_variant_get_string(newRestoreToken, nullptr));
    g_variant_unref(newRestoreToken);
  } else {
    vlog.info("Could not get restore token - display choice will not be saved");
  }

  openPipewireRemote();
}

void RemoteDesktop::handleSelectSources(GVariant* /* parameters */)
{
  if (Clipboard::available())
    initClipboardCb();

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
    vlog.error("Could not start PipeWire remote: %s", error->message);
    g_error_free(error);
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  if (!g_variant_is_of_type(response, G_VARIANT_TYPE("(h)"))) {
    g_variant_unref(response);
    vlog.error("Could not start PipeWire: invalid response type: %s",
               g_variant_get_type_string(response));
    cancelStartCb("Failed to start remote desktop session");
    return;
  }

  g_variant_get(response, "(h)", &fdIndex);
  g_variant_unref(response);

  fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  g_object_unref(fdList);

  if (error) {
    vlog.error("Could not start PipeWire remote: error getting fd list:  %s",
               error->message);
    g_error_free(error);
    cancelStartCb("Failed to start remote desktop session");

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
    vlog.error("Could not find streams to parse");
    return false;
  }

  if (n_streams != 1) {
    vlog.error("Only one stream supported, got %d",
               n_streams);
  }

  stream = g_variant_get_child_value(streams, 0);
  g_variant_get_child(stream, 0, "u", &pipewireNodeId);

  g_variant_unref(stream);

  return true;
}

bool RemoteDesktop::loadRestoreToken()
{
  char filepath[PATH_MAX];
  FILE* f;
  const char* stateDir;
  char restoreToken_[37];

  if (rememberDisplayChoice == "Never")
    return false;

  // restoreToken will be empty the first time, we want to prompt the user
  if (rememberDisplayChoice == "Session")
    return !restoreToken.empty();

  assert(rememberDisplayChoice == "Always");

  // Only load from disk the first time
  if (!restoreToken.empty())
    return true;

  stateDir = core::getvncstatedir();
  if (!stateDir) {
    vlog.error("Could not get state directory");
    return false;
  }

  snprintf(filepath, sizeof(filepath), "%s/%s", stateDir, RESTORE_TOKEN_FILENAME);
  f = fopen(filepath, "r");
  if (!f) {
    if (errno != ENOENT)
      vlog.error("Could not open \"%s\": %s", filepath, strerror(errno));

    return false;
  }

  if (fgets(restoreToken_, sizeof(restoreToken_), f)) {
    uuid_t uuid;

    fclose(f);
    if (uuid_parse(restoreToken_, uuid) < 0) {
      vlog.error("Invalid restore token, not a valid UUID string \"%s\"", restoreToken_);
      return false;
    }

    restoreToken = restoreToken_;
    return true;
  }
  fclose(f);

  vlog.error("Could not read restore token from \"%s\"", filepath);
  return false;
}

bool RemoteDesktop::storeRestoreToken(const char* newToken)
{
  char filepath[PATH_MAX];
  FILE* f;
  const char* stateDir;

  if (!newToken)
    return false;

  // Don't store anything
  if (rememberDisplayChoice == "Never")
    return true;

  // Store token in memory
  restoreToken = newToken;

  if (rememberDisplayChoice == "Session")
    return true;

  assert(rememberDisplayChoice == "Always");

  // Store token to file so it can be re-used on next startup
  stateDir = core::getvncstatedir();
  if (!stateDir) {
    vlog.error("Could not determine VNC state directory path, cannot store restore token");
    return false;
  }

  if (core::mkdir_p(stateDir, 0755) == -1) {
    if (errno != EEXIST) {
      vlog.error("Could not create VNC config directory \"%s\": %s",
                  stateDir, strerror(errno));
      return false;
    }
  }

  snprintf(filepath, sizeof(filepath), "%s/%s", stateDir,
           RESTORE_TOKEN_FILENAME);

  f = fopen(filepath, "w");
  if (!f) {
    vlog.error("Could not store token to \"%s\": %s", filepath, strerror(errno));
    return false;
  }

  fprintf(f, "%s", newToken);
  fclose(f);

  return true;
}
