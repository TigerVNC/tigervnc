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

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include <stdexcept>

#include <gio-unix-2.0/gio/gunixinputstream.h>
#include <gio/gio.h>
#include <ext-data-control-v1.h>

#include <core/string.h>
#include <core/LogWriter.h>

#include "../../parameters.h"
#include "Display.h"
#include "Seat.h"
#include "DataControl.h"

using namespace wayland;

static const char* MIME_TEXT_PLAIN = "text/plain";
static const char* MIME_TEXT_PLAIN_UTF8 = "text/plain;charset=utf-8";

static const char* MIME_TYPES[] {
  MIME_TEXT_PLAIN_UTF8,
  MIME_TEXT_PLAIN,
};

/* FIXME: There is currently no way of knowing if it is us or some
*  other application that owns the clipboard. This means we can't
*  distinguish if a clipboard change was initiated by us, or by
*  someone else. A workaround for now is to use a unique mime type to
*  indicate ownership.
*  https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/issues/111
*/
static std::string tigervncMimeType = "application/x-tigervnc-data-control-" + std::to_string(getpid());

struct PendingData {
  int32_t fd;
  std::string mimeType;
};

static core::LogWriter vlog("WaylandDataControl");

const ext_data_control_device_v1_listener DataControl::deviceListener = {
  .data_offer = [](void* data, ext_data_control_device_v1*,
                   ext_data_control_offer_v1* offer) {
    ((DataControl*)data)->handleDataOffer(offer);
  },
  .selection = [](void* data, ext_data_control_device_v1*,
                  ext_data_control_offer_v1* offer) {
    ((DataControl*)data)->handleSelection(offer);

  },
  .finished = [](void* data, ext_data_control_device_v1*) {
    ((DataControl*)data)->handleFinished();
  },
  .primary_selection = [](void* data, ext_data_control_device_v1*,
                          ext_data_control_offer_v1* offer) {
    ((DataControl*)data)->handlePrimarySelection(offer);
  }
};

const ext_data_control_source_v1_listener DataControl::sourceListener = {
  .send = [](void* data, ext_data_control_source_v1*,
             const char* mimeType, int32_t fd) {
    ((DataControl*)data)->handleSend(mimeType, fd);
  },
  .cancelled = [](void* data, ext_data_control_source_v1* source) {
    ((DataControl*)data)->handleCancelled(source);
  }
};

const ext_data_control_offer_v1_listener DataControl::offerListener = {
  .offer = [](void* data, ext_data_control_offer_v1*,
              const char* mimeType) {
    ((DataControl*)data)->handleOffer(mimeType);
  }
};

DataControl::DataControl(Display* display, Seat* seat,
                         std::function<void(bool)> clipboardAnnounceCb_,
                         std::function<void()> clipboardRequestCb_,
                         std::function<void(const char*)> sendClipboardDataCb_)
:  Object(display, "ext_data_control_manager_v1",
          &ext_data_control_manager_v1_interface),
   manager(nullptr), device(nullptr), lastOffer(nullptr),
   selectionSource(nullptr), primarySource(nullptr),
   readInProgress(false), pendingReadMimeType(nullptr),
   readStream(nullptr), readCancellable(nullptr), readBuffer(""),
   clipboardAnnounceCb(clipboardAnnounceCb_),
   clipboardRequestCb(clipboardRequestCb_),
   sendClipboardDataCb(sendClipboardDataCb_)
{
  manager = (ext_data_control_manager_v1*)boundObject;
  device = ext_data_control_manager_v1_get_data_device(manager, seat->getSeat());

  if (!device)
    throw std::runtime_error("Failed to get data device");

  ext_data_control_device_v1_add_listener(device, &deviceListener, this);

  display->roundtrip();
}

DataControl::~DataControl()
{
  clearPendingWrites();

  if (device)
    ext_data_control_device_v1_destroy(device);
  if (manager)
    ext_data_control_manager_v1_destroy(manager);
  if (selectionSource)
    ext_data_control_source_v1_destroy(selectionSource);
  if (primarySource)
    ext_data_control_source_v1_destroy(primarySource);

  if (readInProgress)
    g_cancellable_cancel(readCancellable);

  if (readStream)
    g_object_unref(readStream);
  if (readCancellable)
    g_object_unref(readCancellable);

  if (lastOffer) {
    ext_data_control_offer_v1_destroy(lastOffer);
    lastOffer = nullptr;
  }
}

void DataControl::setSelection()
{
  clearPendingWrites();
  if (readInProgress)
    g_cancellable_cancel(readCancellable);

  if (selectionSource)
    ext_data_control_source_v1_destroy(selectionSource);
  selectionSource = nullptr;

  try {
    selectionSource = createDataSource();
  } catch (const std::exception& e) {
    vlog.error("Could not set selection: %s", e.what());
    return;
  }

  ext_data_control_device_v1_set_selection(device, selectionSource);
}

void DataControl::setPrimarySelection()
{
  clearPendingWrites();
  if (readInProgress)
    g_cancellable_cancel(readCancellable);

  if (primarySource)
    ext_data_control_source_v1_destroy(primarySource);
  primarySource = nullptr;

  try {
    primarySource = createDataSource();
  } catch (const std::exception& e) {
    vlog.error("Could not set primary selection: %s", e.what());
    return;
  }

  ext_data_control_device_v1_set_primary_selection(device, primarySource);
}

void DataControl::clearSelection()
{
  if (selectionSource)
    ext_data_control_source_v1_destroy(selectionSource);
  selectionSource = nullptr;

  ext_data_control_device_v1_set_selection(device, nullptr);

  clearPendingWrites();
  if (readInProgress)
    g_cancellable_cancel(readCancellable);
}

void DataControl::clearPrimarySelection()
{
  if (primarySource)
    ext_data_control_source_v1_destroy(primarySource);
  primarySource = nullptr;

  ext_data_control_device_v1_set_primary_selection(device, nullptr);

  clearPendingWrites();
  if (readInProgress)
    g_cancellable_cancel(readCancellable);
}

void DataControl::receive()
{
  int pipeFd[2];
  const char* mimeType;

  // Nothing to receive
  if (!lastOffer)
    return;

  if (readInProgress)
    return;

  mimeType = nullptr;
  for (const char* m : MIME_TYPES) {
    if (availableMimeTypes.find(m) != availableMimeTypes.end()) {
      mimeType = m;
      break;
    }
  }

  if (!mimeType)
    return;

  if (pipe2(pipeFd, O_NONBLOCK) == -1) {
    vlog.error("Failed to read clipboard data: %s", strerror(errno));
    return;
  }

  readInProgress = true;
  pendingReadMimeType = mimeType;

  ext_data_control_offer_v1_receive(lastOffer, mimeType, pipeFd[1]);
  close(pipeFd[1]);

  readCancellable = g_cancellable_new();
  readStream = g_unix_input_stream_new(pipeFd[0], true);
  readBuffer.clear();

  g_input_stream_read_bytes_async(
    readStream,
    4096,
    G_PRIORITY_DEFAULT,
    readCancellable,
    [](GObject *, GAsyncResult* res, void* userData) {
      ((DataControl*)userData)->handleReadDataCallback(res);
    },
    this);
}

void DataControl::writePending(const char* data)
{
  if (!core::isValidUTF8(data)) {
    vlog.error("Could not write to clipboard: invalid UTF-8");
    clearPendingWrites();
    return;
  }

  while (!pendingWrites.empty()) {
    PendingData pending;
    const char* mimeType;
    uint32_t fd;
    std::string clientData;

    pending = pendingWrites.front();
    pendingWrites.pop();

    mimeType = pending.mimeType.c_str();
    fd = pending.fd;
    if (strcmp(mimeType, MIME_TEXT_PLAIN_UTF8) == 0) {
      clientData = data;
    } else if (strcmp(mimeType, MIME_TEXT_PLAIN) == 0) {
      clientData = core::utf8ToAscii(data, strlen(data));
    } else {
      // FIXME: This shouldn't be possible
      vlog.error("Could not write to clipboard: unsupported mime type %s", mimeType);
      return;
    }

    if (write(fd, clientData.c_str(), clientData.size()) == -1)
      vlog.error("Could not write to clipboard: %s", strerror(errno));

    close(fd);
  }
}

void DataControl::handleDataOffer(ext_data_control_offer_v1* offer)
{
  ext_data_control_offer_v1_add_listener(offer, &offerListener, this);
  lastOffer = offer;
  availableMimeTypes.clear();

  clearPendingWrites();
  if (readInProgress)
    g_cancellable_cancel(readCancellable);
  clipboardAnnounceCb(false);
}

void DataControl::handleSelection(ext_data_control_offer_v1* offer)
{
  bool validMimeType;

  if (!offer) {
    clipboardAnnounceCb(false);
    return;
  }

  if (offer != lastOffer) {
    vlog.error("Got unexpected clipboard offer - ignoring");
    return;
  }

  // Ignore our own requests
  if (availableMimeTypes.find(tigervncMimeType) != availableMimeTypes.end())
    return;

  validMimeType = false;
  for (const char* m : MIME_TYPES) {
    if (availableMimeTypes.find(m) != availableMimeTypes.end()) {
      validMimeType = true;
      break;
    }
  }

  if (validMimeType)
    clipboardAnnounceCb(true);
}

void DataControl::handlePrimarySelection(ext_data_control_offer_v1* offer)
{
  bool validMimeType;

  if (!sendPrimary)
    return;

  if (!offer) {
    clipboardAnnounceCb(false);
    return;
  }

  if (offer != lastOffer) {
    vlog.error("Got unexpected clipboard offer - ignoring");
    return;
  }

  // Ignore our own requests
  if (availableMimeTypes.find(tigervncMimeType) != availableMimeTypes.end())
    return;

  validMimeType = false;
  for (const char* m : MIME_TYPES) {
    if (availableMimeTypes.find(m) != availableMimeTypes.end()) {
      validMimeType = true;
      break;
    }
  }

  if (validMimeType)
    clipboardAnnounceCb(true);
}

void DataControl::handleFinished()
{
  ext_data_control_device_v1_destroy(device);
  device = nullptr;
}

void DataControl::handleSend(const char* mimeType, int32_t fd)
{
  bool validMimeType;

  lastOffer = nullptr;
  validMimeType = false;

  for (const char* m : MIME_TYPES) {
    if (strcmp(mimeType, m) == 0) {
      validMimeType = true;
      break;
    }
  }

  // FIXME: There doesn't seem to be a way to indicate a failure properly
  if (!validMimeType) {
    vlog.error("Could not write to clipboard: unsupported mime type %s", mimeType);
    if (close(fd) == -1)
      vlog.error("Could not write to clipboard: %s", strerror(errno));
    return;
  }

  pendingWrites.push({fd, mimeType});
  clipboardRequestCb();
}

void DataControl::handleCancelled(ext_data_control_source_v1* source)
{
  ext_data_control_source_v1_destroy(source);
  if (source == primarySource)
    primarySource = nullptr;
  if (source == selectionSource)
    selectionSource = nullptr;
}

void DataControl::handleOffer(const char* mimeType)
{
  // FIXME: How do we distinguish between selection and primary?
  availableMimeTypes.insert(mimeType);
}

// FIXME: This is identical to the Portal's
//        Clipboard::handleReadDataCallback(). Keep in sync
void DataControl::handleReadDataCallback(GAsyncResult* res)
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
    std::string utf8string;

    g_object_unref(readStream);
    g_object_unref(readCancellable);
    readStream = nullptr;
    readCancellable = nullptr;
    readInProgress = false;
    g_bytes_unref(bytes);

    if (strcmp(pendingReadMimeType, MIME_TEXT_PLAIN) == 0) {
      if (!core::isValidAscii(readBuffer.c_str(), readBuffer.size())) {
        vlog.error("Invalid ASCII sequence in clipboard - ignoring");
        readBuffer.clear();
        readInProgress = false;
        return;
      }

      utf8string = core::utf8ToAscii(readBuffer.c_str(), readBuffer.size());
      readBuffer = utf8string;
    } else if (strcmp(pendingReadMimeType, MIME_TEXT_PLAIN_UTF8) == 0) {
      if (!core::isValidUTF8(readBuffer.c_str(), readBuffer.size())) {
        vlog.error("Invalid UTF-8 sequence in clipboard - ignoring");
        readBuffer.clear();
        readInProgress = false;
        return;
      }
    }

    sendClipboardDataCb(readBuffer.c_str());
    readBuffer.clear();
    return;
  }

  data = (const char*)g_bytes_get_data(bytes, &dataSize);

  readBuffer.append(data, dataSize);
  g_bytes_unref(bytes);

  // While bytesRead > 0, continue reading until we reach EOF
  g_input_stream_read_bytes_async(
    readStream, 4096, G_PRIORITY_DEFAULT, readCancellable,
    [](GObject*, GAsyncResult* result, void* userData) {
      ((DataControl*)userData)->handleReadDataCallback(result);
    }, this);
}

ext_data_control_source_v1* DataControl::createDataSource()
{
  ext_data_control_source_v1* source;

  assert(manager);

  source = ext_data_control_manager_v1_create_data_source(manager);
  if (!source)
    throw std::runtime_error("Failed to create data source");

  ext_data_control_source_v1_add_listener(source, &sourceListener, this);

  ext_data_control_source_v1_offer(source, tigervncMimeType.c_str());
  for (const char* mimeType : MIME_TYPES)
    ext_data_control_source_v1_offer(source, mimeType);

  return source;
}

void DataControl::clearPendingWrites()
{
  while (!pendingWrites.empty()) {
    close(pendingWrites.front().fd);
    pendingWrites.pop();
  }
}
