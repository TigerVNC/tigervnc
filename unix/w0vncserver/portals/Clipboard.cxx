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

#include <assert.h>
#include <sys/stat.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <gio-unix-2.0/gio/gunixinputstream.h>
#include <glib-object.h>
#include <glib.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "../w0vncserver.h"
#include "PortalProxy.h"
#include "Clipboard.h"

static core::LogWriter vlog("Clipboard");

const char* MIME_TEXT_PLAIN = "text/plain";
const char* MIME_TEXT_PLAIN_UTF8 = "text/plain;charset=utf-8";

const char* MIME_TYPES[] {
  MIME_TEXT_PLAIN_UTF8,
  MIME_TEXT_PLAIN,
};

Clipboard::Clipboard(std::string sessionHandle_,
                     std::function<void(const char* data)>
                       sendClipboardDataCb_,
                      std::function<void(bool available)>
                        clipboardAnnounceCb_)
  : clipboard(nullptr), sessionHandle(sessionHandle_),
    selectionOwner(true), readInProgress(false),
    pendingReadMimeType(nullptr),readStream(nullptr),
    readCancellable(nullptr),
    sendClipboardDataCb(sendClipboardDataCb_),
    clipboardAnnounceCb(clipboardAnnounceCb_)
{
  assert(available());

  clipboard = new PortalProxy("org.freedesktop.portal.Desktop",
                              "/org/freedesktop/portal/desktop",
                              "org.freedesktop.portal.Clipboard");
}

Clipboard::~Clipboard()
{
  delete clipboard;
  if (readStream)
    g_object_unref(readStream);
  // FIXME: Do we cancel a pending read here?
  if (readCancellable)
    g_object_unref(readCancellable);
}

bool Clipboard::available()
{
  return PortalProxy::interfacesAvailable({"org.freedesktop.portal.Clipboard"});
}

void Clipboard::subscribe()
{
  clipboard->subscribe("SelectionTransfer",
                       std::bind(&Clipboard::handleSelectionTransfer,
                                   this, std::placeholders::_1));
  clipboard->subscribe("SelectionOwnerChanged",
                       std::bind(&Clipboard::handleSelectionOwnerChanged,
                                   this, std::placeholders::_1));
}

void Clipboard::requestClipboard()
{
  GVariant* params;
  GVariantBuilder optionsBuilder;

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  params = g_variant_new("(oa{sv})", sessionHandle.c_str(), &optionsBuilder);

  clipboard->call("RequestClipboard", params);
}

void Clipboard::setSelection(const char* data)
{
  GVariantBuilder optionsBuilder;
  GVariant* params;
  GVariant* mimeTypes;

  clientData = data;
  mimeTypes = g_variant_new_strv(&MIME_TEXT_PLAIN_UTF8, 1);

  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add(&optionsBuilder, "{sv}", "mime_types", mimeTypes);
  params = g_variant_new("(oa{sv})", sessionHandle.c_str(),
                         &optionsBuilder);

  clipboard->call("SetSelection", params);
}

void Clipboard::selectionWrite(uint32_t serial)
{
  GError* error = nullptr;
  GVariant* params;

  params = g_variant_new("(ou)", sessionHandle.c_str(), serial);

  pendingSerials.push(serial);

  g_dbus_proxy_call_with_unix_fd_list(
    clipboard->getProxy(),
    "SelectionWrite", params, G_DBUS_CALL_FLAGS_NONE, 3000,
    nullptr, nullptr,
    [](GObject* proxy, GAsyncResult* res, void* userData) {
      ((Clipboard*)(userData))->handleSelectionWrite(proxy, res);
     }, this);

  if (error) {
    vlog.error("Clipboard.SelectionWrite failed: %s", error->message);
    g_error_free(error);
    return;
  }
}

void Clipboard::selectionWriteDone(uint32_t serial, bool success)
{
  GError* error = nullptr;
  GVariant* params;
  GVariant* result;

  params = g_variant_new("(oub)", sessionHandle.c_str(),
                         serial, success);

  result = g_dbus_proxy_call_sync(clipboard->getProxy(),
                                  "SelectionWriteDone", params,
                                  G_DBUS_CALL_FLAGS_NONE, 3000,
                                  nullptr, &error);
  if (error) {
    vlog.error("Clipboard.SelectionWriteDone failed: %s", error->message);
    g_error_free(error);
    return;
  }

  g_variant_unref(result);
}

void Clipboard::selectionRead()
{
  GVariant* params;
  const char* mimeType;

  if (selectionOwner)
    return;

  if (readInProgress) {
    // FIXME: Do we cancel our current read and read again?
    return;
  }

  mimeType = "";
  for (uint i = 0; i < sizeof(MIME_TYPES) / sizeof(MIME_TYPES[0]); i++) {
    if (availableMimeTypes.find(MIME_TYPES[i]) != availableMimeTypes.end()) {
      mimeType = MIME_TYPES[i];
      break;
    }
  }

  if (!mimeType) {
    vlog.error("Could not find a supported available mime type, skipping selectionRead()");
    return;
  }

  assert(!selectionOwner);
  assert(!readInProgress);
  readInProgress = true;
  pendingReadMimeType = mimeType;

  params = g_variant_new("(os)", sessionHandle.c_str(), mimeType);

  g_dbus_proxy_call_with_unix_fd_list(
    clipboard->getProxy(),
    "SelectionRead",
    params,
    G_DBUS_CALL_FLAGS_NONE,
    3000,
    nullptr,
    nullptr,
    [](GObject* proxy, GAsyncResult* res, void* userData) {
      Clipboard* self = (Clipboard*)(userData);
      self->handleSelectionRead(proxy, res);
    },
    this);
}

void Clipboard::handleSelectionRead(GObject* proxy, GAsyncResult* res)
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
    vlog.error("%s", msg.c_str());
    readInProgress = false;
    return;
  }

  if (!g_variant_is_of_type(response, G_VARIANT_TYPE("(h)"))) {
    g_variant_unref(response);
    g_object_unref(fdList);
    vlog.error("Clipboard::handleSelectionRead: invalid response type: %s, expected (h)",
                g_variant_get_type_string(response));
    readInProgress = false;
    return;
  }

  g_variant_get(response, "(h)", &fdIndex);
  g_variant_unref(response);

  fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  g_object_unref(fdList);

  if (error || fd == -1) {
    std::string msg(error->message);
    g_error_free(error);
    readInProgress = false;
    vlog.error("Error getting fd:  %s", msg.c_str());
    return;
  }

  readCancellable = g_cancellable_new();
  readStream = g_unix_input_stream_new(fd, true);
  readBuffer.clear();

  g_input_stream_read_bytes_async(
    readStream,
    4096,
    G_PRIORITY_DEFAULT,
    readCancellable,
    [](GObject*, GAsyncResult* result, void* userData) {
      ((Clipboard*)userData)->handleReadDataCallback(result);
    },
    this);
}

void Clipboard::handleSelectionTransfer(GVariant* parameters)
{
  uint32_t serial;
  char* sessionHandle_; // ignored
  char* mimeType;

  g_variant_get(parameters, "(osu)", &sessionHandle_, &mimeType, &serial);

  // Always write UTF-8
  if (strcmp(mimeType, MIME_TEXT_PLAIN_UTF8) != 0) {
    vlog.error("Unsupported mime type %s", mimeType);
    return;
  }

  free(sessionHandle_);
  free(mimeType);

  selectionWrite(serial);
}

void Clipboard::handleSelectionWrite(GObject* proxy,
                                     GAsyncResult* res)
{
  GError* error = nullptr;
  GUnixFDList* fdList = nullptr;
  GVariant* response;
  int fd;
  size_t remaining;
  ssize_t written;
  const char* data;
  uint32_t serial;

  assert(G_IS_DBUS_PROXY(proxy));

  response = g_dbus_proxy_call_with_unix_fd_list_finish(G_DBUS_PROXY(proxy),
                                                        &fdList, res, &error);
  g_variant_unref(response);

  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    vlog.error("%s", msg.c_str());
    return;
  }

  // FIXME: Can we always assume index 0?
  fd = g_unix_fd_list_get(fdList, 0, &error);
  if (error) {
    std::string msg(error->message);
    g_error_free(error);
    g_object_unref(fdList);
    vlog.error("%s", msg.c_str());
    return;
  }

  serial = pendingSerials.front();
  pendingSerials.pop();
  data = clientData.c_str();

  remaining = clientData.length() + 1;
  written = 0;
  // FIXME: Should this write be async as well?
  while (remaining > 0) {
    written = write(fd, data, remaining);

    if (written < 0) {
      vlog.error("handleSelectionWrite: write() failed: %s", strerror(errno));
      selectionWriteDone(serial, false);
      g_object_unref(fdList);
      close(fd);
      return;
    }

    remaining -= written;
    data += written;
  }

  if (close(fd) != 0) {
    vlog.error("Failed to close fd: %s", strerror(errno));
    selectionWriteDone(serial, false);
  } else {
    selectionWriteDone(serial, true);
  }

  g_object_unref(fdList);
}

void Clipboard::handleSelectionOwnerChanged(GVariant* parameters)
{
  GVariant* sessionHandle_; // ignored
  GVariant* options;
  GVariant* mimeTypes;
  GVariant* sessionIsOwner;

  g_variant_get(parameters, "(o@a{sv})", &sessionHandle_, &options);

  g_variant_unref(sessionHandle_);

  mimeTypes = g_variant_lookup_value(options, "mime_types",
                                     G_VARIANT_TYPE("(as)"));
  sessionIsOwner = g_variant_lookup_value(options, "session_is_owner",
                                          G_VARIANT_TYPE_BOOLEAN);
  g_variant_unref(options);

  if (!sessionIsOwner) {
    readInProgress = false;
    availableMimeTypes.clear();

    if (mimeTypes)
      g_variant_unref(mimeTypes);

    return;
  }

  selectionOwner = g_variant_get_boolean(sessionIsOwner);
  g_variant_unref(sessionIsOwner);

  if (selectionOwner) {
    availableMimeTypes.clear();

    if (mimeTypes)
      g_variant_unref(mimeTypes);

    return;
  }

  // Clear any pending writes
  while (!pendingSerials.empty())
    pendingSerials.pop();

  if (mimeTypes) {
    char** mimeTypeArray;
    g_variant_get(mimeTypes, "(^as)", &mimeTypeArray);

    for (int i = 0; mimeTypeArray[i] != nullptr; i++) {
      for (uint j = 0; j < sizeof(MIME_TYPES) / sizeof(MIME_TYPES[0]); j++) {
        if (strcmp(mimeTypeArray[i], MIME_TYPES[j]) == 0) {
          availableMimeTypes.insert(mimeTypeArray[i]);
        }
      }
    }

    clipboardAnnounceCb(true);
    g_strfreev(mimeTypeArray);
    g_variant_unref(mimeTypes);
  }
}

void Clipboard::handleReadDataCallback(GAsyncResult* res)
{
  GError* error = nullptr;
  GBytes* bytes;
  size_t bytesRead;
  size_t dataSize;
  const char* data;

  bytes = g_input_stream_read_bytes_finish(readStream, res, &error);

  if (error) {
    if (error->code == G_IO_ERROR_CANCELLED)
      vlog.error("Cancelled reading clipboard data");
    else
      vlog.error("Failed to read clipboard data: %s", error->message);

    g_error_free(error);
    g_object_unref(readStream);
    g_object_unref(readCancellable);
    readStream = nullptr;
    readCancellable = nullptr;
    readInProgress = false;
    return;
  }

  bytesRead = g_bytes_get_size(bytes);

  // Zero is returned on EOF, we are finished reading
  if (bytesRead == 0) {
    sendClipboardDataCb(readBuffer.c_str());

    g_object_unref(readStream);
    g_object_unref(readCancellable);
    readStream = nullptr;
    readCancellable = nullptr;
    readInProgress = false;
    readBuffer.clear();

    g_bytes_unref(bytes);
    return;
  }

  data = (const char*)g_bytes_get_data(bytes, &dataSize);

  if (strcmp(pendingReadMimeType, MIME_TEXT_PLAIN) == 0) {
    if (!core::isValidAscii(data, dataSize)) {
      vlog.error("Invalid ASCII sequence in clipboard - ignoring");
      g_cancellable_cancel(readCancellable);
      return;
    }
  } else if (strcmp(pendingReadMimeType, MIME_TEXT_PLAIN_UTF8) == 0) {
    if (!core::isValidUTF8(data, dataSize)) {
      vlog.error("Invalid UTF-8 sequence in clipboard - ignoring");
      g_cancellable_cancel(readCancellable);
      return;
    }
  }

  readBuffer.append(data, dataSize);
  g_bytes_unref(bytes);

  // While bytesRead > 0, continue reading until we reach EOF
  g_input_stream_read_bytes_async(
    readStream, 4096, G_PRIORITY_DEFAULT, readCancellable,
    [](GObject*, GAsyncResult* result, void* userData) {
      ((Clipboard*)userData)->handleReadDataCallback(result);
    }, this);
}
