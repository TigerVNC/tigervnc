#include <X11/extensions/XI.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <assert.h>
#include <sys/select.h>
#include <string.h>

#include <stdexcept>
#include <vector>

#include <glib.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "RemoteDesktop.h"
#include "Portal.h"

static core::LogWriter vlog("RemoteDesktop");

core::BoolParameter localCursor(
  "experimentalLocalCursor", "[EXPERIMENTAL] Render cursor locally", false);

RemoteDesktop::RemoteDesktop(rfb::VNCServer* server,
                             std::function<void(int fd, uint32_t nodeId)> startPipewireCb)
  : Portal(), pipewireStarted(false), server_(server),
    screencastProxy_(nullptr), startPipewireCb_(startPipewireCb)
{
  GError* error = nullptr;

  screencastProxy_ = g_dbus_proxy_new_sync(connection_,
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

  createSession();
}

RemoteDesktop::~RemoteDesktop()
{
  for (PipeWireStreamData* s : streams_)
    delete s;
  g_object_unref(screencastProxy_);
}

void RemoteDesktop::createSession()
{
  assert(screencastProxy_);

  const char* handleToken;
  const char* sessionHandleToken;
  GVariantBuilder optionsBuilder;
  const char* requestHandle;
  GVariant* params;

  newRequestHandle(&requestHandle, &handleToken);
  sessionHandleToken = newSessionHandle();

  vlog.debug("handle_token: %s", handleToken);
  vlog.debug("session_handle_token: %s", sessionHandleToken);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handleToken));
  g_variant_builder_add(&optionsBuilder, "{sv}", "session_handle_token",
                        g_variant_new_string(sessionHandleToken));

  vlog.debug("request_handle: %s", requestHandle);

  params = g_variant_new("(a{sv})", &optionsBuilder);

  call(screencastProxy_, "CreateSession", params,
       G_DBUS_CALL_FLAGS_NONE, [](GDBusConnection*, const char*,
                                  const char*, const char*,
                                  const char*, GVariant *parameters,
                                  void *userData) {
    RemoteDesktop* self = static_cast<RemoteDesktop*>(userData);
    self->handleCreateSession(parameters);
                                  }, requestHandle, this);
}

void RemoteDesktop::handleCreateSession(GVariant *parameters)
{
  GVariant* results;
  uint32_t response;
  GVariant* sessionHandle;


  vlog.debug("onCreateSession()");

  g_variant_get(parameters, "(u@a{sv})", &response, &results);

  if (response == 1) {
    throw std::runtime_error("Error: CreateSession was cancelled");
  } else if (response == 2) {
    throw std::runtime_error("Error: CreateSession failed");
  }

  sessionHandle = g_variant_lookup_value(results,
                                         "session_handle",
                                         G_VARIANT_TYPE_STRING);

  setSessionHandle(strdup(g_variant_get_string(sessionHandle, nullptr)));

  vlog.debug("session_handle: %s", sessionHandle_);

  selectSources();
}

void RemoteDesktop::selectSources()
{
  GVariant* params;
  GVariantBuilder optionsBuilder;
  const char* handleToken;
  const char* requestHandle;

  vlog.debug("selectSources()");

  newRequestHandle(&requestHandle, &handleToken);

  if (!g_variant_is_object_path(requestHandle)) {
    throw std::runtime_error("Invalid request handle");
  }

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "types",
                        g_variant_new_uint32(SRC_MONITOR));
  g_variant_builder_add(&optionsBuilder, "{sv}", "cursor_mode",
                        g_variant_new_uint32(CUR_EMBEDDED));
  g_variant_builder_add(&optionsBuilder, "{sv}", "multiple",
                         g_variant_new_boolean(false));

  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handleToken));

  params = g_variant_new("(oa{sv})", sessionHandle_,
                         &optionsBuilder);

  call(screencastProxy_, "SelectSources", params,
       G_DBUS_CALL_FLAGS_NO_AUTO_START, [](GDBusConnection*,
                                           const char*, const char*,
                                           const char*, const char*,
                                           GVariant *parameters,
                                           void *userData) {
    RemoteDesktop* self = static_cast<RemoteDesktop*>(userData);
    self->handleSelectSources(parameters);
                                           },
                                           requestHandle,
                                           this);
}


void RemoteDesktop::start()
{
  const char* handletoken;
  const char* requestHandle;
  GVariantBuilder optionsBuilder;
  GVariant *params;

  assert(screencastProxy_);

  vlog.debug("start()");

  newRequestHandle(&requestHandle, &handletoken);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "handle_token",
                        g_variant_new_string(handletoken));

  params = g_variant_new("(osa{sv})", sessionHandle_, "",
                         &optionsBuilder);

  call(screencastProxy_, "Start", params,
       G_DBUS_CALL_FLAGS_NO_AUTO_START, [](GDBusConnection*,
                                           const char*, const char*,
                                           const char*, const char*,
                                           GVariant *parameters,
                                           void *userData) {
    RemoteDesktop* self = static_cast<RemoteDesktop*>(userData);
    self->handleStart(parameters);
                                           },
                                           requestHandle, this);
}

void RemoteDesktop::handleStart(GVariant *parameters)
{
  uint32_t responseCode;
  GVariant* result;
  GVariant* streams;


  vlog.debug("onStart()");

  if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ua{sv})"))) {
    // FIXME: Do we need to free some resources here as well?
    throw std::runtime_error(
      core::format("Unexpected parameters type %s",
                    g_variant_get_type_string(parameters)));
  }

  g_variant_get(parameters, "(u@a{sv})", &responseCode, &result);

  if (responseCode == 1) {
    throw std::runtime_error("ScreenCast.Start cancelled");
  } if (responseCode == 2) {
    throw std::runtime_error("ScreenCast.Start failed");
  }

  streams = g_variant_lookup_value(result, "streams",
            G_VARIANT_TYPE_ARRAY);

  vlog.debug("streams: %s", g_variant_print(streams, true));

  parseStreams(streams);
  openPipewireRemote();

  g_variant_unref(result);
}

void RemoteDesktop::openPipewireRemote()
{
  GVariantBuilder optionsBuilder;

  vlog.debug("openPipewireRemote()");

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);

  g_dbus_proxy_call_with_unix_fd_list(screencastProxy_,
                                     "OpenPipeWireRemote",
                                      g_variant_new("(oa{sv})",
                                      sessionHandle_,
                                      &optionsBuilder),
                                      G_DBUS_CALL_FLAGS_NONE,
                                      3000,
                                      nullptr, /* fd_list */
                                      nullptr, /* cancellable */
                                      [](GObject* proxy,
                                         GAsyncResult *res,
                                         void *userData) {
    RemoteDesktop* self = static_cast<RemoteDesktop*>(userData);
    self->handleOpenPipewireRemote(proxy, res);
                                      },
                                      this);
}

void RemoteDesktop::handleSelectSources(GVariant *parameters)
{
  (void)parameters;
  start();
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

  vlog.debug("onOpenPipewireRemote()");

  response = g_dbus_proxy_call_with_unix_fd_list_finish(G_DBUS_PROXY(proxy),
                                                        &fdList, res,
                                                        &error);

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

  g_variant_get(response, "(h)", &fdIndex);

  fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    g_free(response);
    throw std::runtime_error(core::format(
      "Screencast: error getting fd list:  %s", error_message.c_str()));
  }

  // FIXME: Handle multiple streams
  PipeWireStreamData* stream = pipewireStreams()[0];
  startPipewire(fd, stream->pwNodeID);

  g_free(response);
}

void RemoteDesktop::startPipewire(int fd, uint32_t nodeId)
{
  assert(!pipewireStarted);
  startPipewireCb_(fd, nodeId);

  pipewireStarted = true;
}

void RemoteDesktop::parseStreams(GVariant *streams)
{
  std::vector<PipeWireStreamData*> rdp_streams;
  GVariantIter iter;
  int n_streams;

  g_variant_iter_init(&iter, streams);
  n_streams = g_variant_iter_n_children(&iter);

  assert(n_streams > 0);

  for (int i = 0; i < n_streams; i++) {
    GVariant* stream;
    PipeWireStreamData* s;
    uint32_t pw_id;

    stream = g_variant_get_child_value(streams, i);

    s = parseStream(stream);

    g_variant_get_child(stream, 0, "u", &pw_id);
    s->pwNodeID = pw_id;

    rdp_streams.push_back(s);
  }

  assert(!rdp_streams.empty());

  streams_ = rdp_streams;
}

void RemoteDesktop::setSessionHandle(char* handle)
{
  assert(sessionHandle_ == nullptr);

  sessionHandle_ = strdup(handle);
}

PipeWireStreamData* RemoteDesktop::parseStream(GVariant *stream) {

  GVariant* dictionary;
  const uint32_t *key; // unused? What is even this
  const char *id; // unused
  uint32_t sourceType;
  int32_t x;
  int32_t y;
  int32_t width;
  uint32_t height;
  const char* mappingID;

  PipeWireStreamData* s;

  g_variant_get(stream, "(u@a{sv})", &key, &dictionary);

  // None of these are *strictly* necessary when selecting a single
  // *real* monitor

  if (!dictionary || !g_variant_is_of_type(dictionary, G_VARIANT_TYPE("a{sv}")))
    throw std::runtime_error("Failed to parse dictionary from stream");

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

  s = new PipeWireStreamData();
  s->id = id;
  s->sourceType = sourceType;
  s->x = x;
  s->y = y;
  s->width = width;
  s->height = height;
  s->mappingId = mappingID;

  g_variant_unref(dictionary);

  return s;
}
