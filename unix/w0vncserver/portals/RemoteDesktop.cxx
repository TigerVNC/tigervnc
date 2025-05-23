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

#include <vector>

#include <glib.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "../w0vncserver.h"
#include "RemoteDesktop.h"
#include "Portal.h"

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
                             startPipewireCb_)
  : oldButtonMask(0), selectedDevices(0), pipewireStarted(false),
    sessionHandle(nullptr), remoteDesktopProxy(nullptr),
    screencastProxy(nullptr), startPipewireCb(startPipewireCb_)
{
  remoteDesktopProxy = new Portal("org.freedesktop.portal.Desktop",
                                  "/org/freedesktop/portal/desktop",
                                  "org.freedesktop.portal.RemoteDesktop");
  screencastProxy = new Portal("org.freedesktop.portal.Desktop",
                               "/org/freedesktop/portal/desktop",
                               "org.freedesktop.portal.ScreenCast");
}

RemoteDesktop::~RemoteDesktop()
{
  for (PipeWireStreamData* s : streams)
    delete s;
  closeSession();
  delete screencastProxy;
  delete remoteDesktopProxy;
}

void RemoteDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  uint32_t state;

  if (!(selectedDevices & DEV_KEYBOARD))
    return;

  // FIXME: ignore keycode for now?
  (void)keycode;

  state = down ? 1 : 0;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", sessionHandle.c_str(),
                         &optionsBuilder, keysym, state);
  remoteDesktopProxy->call("NotifyKeyboardKeysym", params);
}

void RemoteDesktop::pointerEvent(int x, int y, uint16_t buttonMask)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  PipeWireStreamData* stream;
  uint32_t pwNodeId;

  if (!(selectedDevices & DEV_POINTER))
    return;

  // FIXME: Handle multiple screens
  stream = streams[0];
  pwNodeId = stream->pwNodeID;

  std::string str_x = std::to_string(x);
  std::string str_y = std::to_string(y);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}udd)", sessionHandle.c_str(),
                         &optionsBuilder, pwNodeId,
                         (double)x,(double)y);

  remoteDesktopProxy->call("NotifyPointerMotionAbsolute", params);

  if (buttonMask == oldButtonMask)
    return;

  for (int32_t i = 0; i < BUTTONS; i++) {
    if ((buttonMask ^ oldButtonMask) & (1 << i)) {
      if (i > 2 && i < 7)
        handleScrollWheel(i);
      else
        handleButton(i, buttonMask & (1 << i));
    }
  }
  oldButtonMask = buttonMask;
}

void RemoteDesktop::handleButton(int32_t button, bool down)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;

  assert(selectedDevices & DEV_POINTER);

  button = getInputCode(button);

  g_variant_builder_init(&optionsBuilder,
                         G_VARIANT_TYPE("a{sv}"));
  params = g_variant_new("(oa{sv}iu)", sessionHandle.c_str(),
                          &optionsBuilder, button, down);

  remoteDesktopProxy->call("NotifyPointerButton", params);
}

void RemoteDesktop::handleScrollWheel(int32_t button)
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

  remoteDesktopProxy->call("NotifyPointerAxisDiscrete", params);
}

void RemoteDesktop::createSession()
{
  std::string token;
  std::string handleToken;
  std::string sessionHandleToken;
  GVariantBuilder optionsBuilder;
  GVariant* params;

  token = remoteDesktopProxy->newToken();
  handleToken = remoteDesktopProxy->newHandle(token);
  sessionHandleToken = remoteDesktopProxy->newToken();

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handleToken.c_str()));
  g_variant_builder_add(&optionsBuilder, "{sv}", "session_handle_token",
                        g_variant_new_string(sessionHandleToken.c_str()));

  params = g_variant_new("(a{sv})", &optionsBuilder);

  remoteDesktopProxy->call("CreateSession", params, token,
                        std::bind(&RemoteDesktop::handleCreateSession,
                                  this, std::placeholders::_1));
}

void RemoteDesktop::closeSession()
{
  GError* error = nullptr;
  GVariant* result;

  result = screencastProxy->callSync("org.freedesktop.portal.Desktop",
                                     sessionHandle,
                                     "org.freedesktop.portal.Session",
                                     "Close", &error);
  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    fatal_error(msg.c_str());
    return;
  }
  g_variant_unref(result);
}

void RemoteDesktop::selectDevices()
{
  std::string token;
  std::string handleToken;
  GVariantBuilder optionsBuilder;
  GVariant *params;

  token = remoteDesktopProxy->newToken();
  handleToken = remoteDesktopProxy->newHandle(token);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "types",
                        g_variant_new_uint32(DEV_KEYBOARD | DEV_POINTER));
  g_variant_builder_add(&optionsBuilder, "{sv}", "persist_mode",
                        g_variant_new_uint32(0));

  // FIXME: Do we want restore_token?

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handleToken.c_str()));

  params = g_variant_new("(oa{sv})", sessionHandle.c_str(), &optionsBuilder);

  remoteDesktopProxy->call("SelectDevices", params, token,
                           std::bind(&RemoteDesktop::handleSelectDevices,
                                     this, std::placeholders::_1));
}

void RemoteDesktop::selectSources()
{
  GVariant* params;
  GVariantBuilder optionsBuilder;
  std::string token;
  std::string handleToken;

  token = screencastProxy->newToken();
  handleToken = screencastProxy->newHandle(token);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "types",
                        g_variant_new_uint32(SRC_MONITOR));
  g_variant_builder_add(&optionsBuilder, "{sv}", "cursor_mode",
                        g_variant_new_uint32(CUR_EMBEDDED));
  g_variant_builder_add(&optionsBuilder, "{sv}", "multiple",
                         g_variant_new_boolean(false));

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handleToken.c_str()));

  params = g_variant_new("(oa{sv})", sessionHandle.c_str(),
                         &optionsBuilder);

  screencastProxy->call("SelectSources", params, token,
                         std::bind(&RemoteDesktop::handleSelectSources,
                                   this, std::placeholders::_1));
}

void RemoteDesktop::start()
{
  std::string token;
  std::string handletoken;
  GVariantBuilder optionsBuilder;
  GVariant *params;

  token = remoteDesktopProxy->newToken();
  handletoken = remoteDesktopProxy->newHandle(token);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handletoken.c_str()));

  params = g_variant_new("(osa{sv})", sessionHandle.c_str(), "",
                         &optionsBuilder);

  remoteDesktopProxy->call("Start", params, token,
                           std::bind(&RemoteDesktop::handleStart,
                                     this, std::placeholders::_1));
}

void RemoteDesktop::openPipewireRemote()
{
  GVariantBuilder optionsBuilder;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);

  g_dbus_proxy_call_with_unix_fd_list(screencastProxy->getProxy(),
                                     "OpenPipeWireRemote",
                                      g_variant_new("(oa{sv})",
                                      sessionHandle.c_str(),
                                      &optionsBuilder),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      3000,
                                      nullptr,
                                      nullptr,
                                      [](GObject* proxy,
                                         GAsyncResult *res,
                                         void *userData) {
    RemoteDesktop* self = static_cast<RemoteDesktop*>(userData);
    self->handleOpenPipewireRemote(proxy, res);
                                      },
                                      this);
}

void RemoteDesktop::startPipewire(int fd, uint32_t nodeId)
{
  assert(!pipewireStarted);
  startPipewireCb(fd, nodeId);

  pipewireStarted = true;
}

void RemoteDesktop::handleCreateSession(GVariant *parameters)
{
  GVariant* result;
  uint32_t response;
  GVariant* sessionHandleVariant;

  assert(sessionHandle.empty());

  g_variant_get(parameters, "(u@a{sv})", &response, &result);

  if (response == 1)  {
    fatal_error("Error: CreateSession was cancelled");
    return;
  }
  else if (response == 2) {
    fatal_error("Error: CreateSession failed");
    return;
  }

  sessionHandleVariant = g_variant_lookup_value(result, "session_handle",
                                                G_VARIANT_TYPE_STRING);

  sessionHandle = g_variant_get_string(sessionHandleVariant, nullptr);

  g_variant_unref(sessionHandleVariant);

  selectDevices();
}


void RemoteDesktop::handleStart(GVariant *parameters)
{
  uint32_t responseCode;
  GVariant* result;
  GVariant* streams_;
  GVariant* devices;

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    // FIXME: Do we need to free some resources here as well?
    fatal_error(core::format("Unexpected parameters type %s",
                             g_variant_get_type_string(parameters)).c_str());
    return;
  }

  g_variant_get(parameters, "(u@a{sv})", &responseCode, &result);

  if (responseCode == 1) {
    g_variant_unref(result);
    fatal_error("RemoteDesktop.Start cancelled");
    return;
  } else if (responseCode == 2) {
    g_variant_unref(result);
    fatal_error("RemoteDesktop.Start failed");
    return;
  }

  devices = g_variant_lookup_value(result, "devices", G_VARIANT_TYPE_UINT32);
  selectedDevices = g_variant_get_uint32(devices);
  streams_ = g_variant_lookup_value(result, "streams",
                                    G_VARIANT_TYPE_ARRAY);

  if (!parseStreams(streams_)) {
    fatal_error("Failed to parse streams");
    return;
  }

  openPipewireRemote();

  g_variant_unref(streams_);
  g_variant_unref(result);
}

void RemoteDesktop::handleSelectSources(GVariant *parameters)
{
  (void)parameters;
  start();
}

void RemoteDesktop::handleSelectDevices(GVariant* /*parameters */)
{
  selectSources();
}

void RemoteDesktop::handleOpenPipewireRemote(GObject *proxy,
                                             GAsyncResult *res)
{
  GError *error = nullptr;
  GUnixFDList *fdList = nullptr;
  GVariant *response;
  int32_t fdIndex;
  int fd;

  assert(G_IS_DBUS_PROXY(proxy));

  response = g_dbus_proxy_call_with_unix_fd_list_finish(G_DBUS_PROXY(proxy),
                                                        &fdList, res,
                                                        &error);

  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    fatal_error(msg.c_str());
    return;
  }

  if (!g_variant_is_of_type(response, G_VARIANT_TYPE("(h)"))) {
    g_variant_unref(response);
    fatal_error(core::format("Screencast: invalid response type: %s, expected (h)",
                             g_variant_get_type_string(response)).c_str());
    return;
  }

  g_variant_get(response, "(h)", &fdIndex);

  fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    g_variant_unref(response);
    fatal_error(core::format("Screencast: error getting fd list:  %s",
                msg.c_str()).c_str());
    return;
  }

  // FIXME: Handle multiple streams
  PipeWireStreamData* stream = pipewireStreams()[0];
  startPipewire(fd, stream->pwNodeID);

  g_variant_unref(response);
}

bool RemoteDesktop::parseStreams(GVariant *streams_)
{
  std::vector<PipeWireStreamData*> pipewireStreams;
  GVariantIter iter;
  int n_streams;

  g_variant_iter_init(&iter, streams_);
  n_streams = g_variant_iter_n_children(&iter);

  if (n_streams  < 1) {
    fatal_error("RemoteDesktop.Start: no streams to parse");
    return false;
  }

  for (int i = 0; i < n_streams; i++) {
    GVariant* stream;
    PipeWireStreamData* streamData;
    uint32_t pw_id;

    stream = g_variant_get_child_value(streams_, i);

    streamData = parseStream(stream);
    if (!streamData)
      return false;

    g_variant_get_child(stream, 0, "u", &pw_id);
    streamData->pwNodeID = pw_id;

    pipewireStreams.push_back(streamData);
    g_variant_unref(stream);
  }

  assert(!pipewireStreams.empty());

  streams = pipewireStreams;
  return true;
}

PipeWireStreamData* RemoteDesktop::parseStream(GVariant *stream) {

  GVariant* dictionary;
  const uint32_t *key; // unused
  const char *id; // unused
  uint32_t sourceType;
  int32_t x;
  int32_t y;
  int32_t width;
  uint32_t height;
  const char* mappingID;

  PipeWireStreamData* streamData;

  g_variant_get(stream, "(u@a{sv})", &key, &dictionary);

  // None of these are *strictly* necessary when selecting a single
  // *real* monitor

  if (!dictionary || !g_variant_is_of_type(dictionary, G_VARIANT_TYPE("a{sv}")))
  {
    g_variant_unref(dictionary);
    fatal_error("Failed to parse dictionary from stream");
    return nullptr;
  }

  if (!g_variant_lookup(dictionary, "id", "&s", &id)) {
    vlog.debug("Failed to parse 'id'");
    id = nullptr;
  }

  if (!g_variant_lookup(dictionary, "source_type", "u", &sourceType))
    vlog.debug("Failed to parse 'source_type'");

  if (!g_variant_lookup(dictionary, "position", "(ii)", &x, &y)) {
    vlog.debug("Failed to parse 'position'");
    x = -1;
    y = -1;
  }

  if (!g_variant_lookup(dictionary, "size", "(ii)", &width, &height))
    vlog.debug("Failed to parse 'size'");

  if (!g_variant_lookup(dictionary, "mapping_id", "&s", &mappingID)) {
    vlog.debug("Failed to parse 'mapping_id'");
    mappingID = nullptr;
  }

  streamData = new PipeWireStreamData();
  streamData->id = id;
  streamData->sourceType = sourceType;
  streamData->x = x;
  streamData->y = y;
  streamData->width = width;
  streamData->height = height;
  streamData->mappingId = mappingID;

  g_variant_unref(dictionary);

  return streamData;
}
