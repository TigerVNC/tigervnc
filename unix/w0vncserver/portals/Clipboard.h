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

#ifndef __PORTAL_CLIPBOARD_H__
#define __PORTAL_CLIPBOARD_H__

#include <stdint.h>

#include <string>
#include <set>
#include <queue>
#include <functional>

#include <gio/gio.h>

struct PendingData;
class PortalProxy;

class Clipboard {
public:
  Clipboard(std::string sessionHandle,
            std::function<void(const char* data)> sendClipboardDataCb,
            std::function<void(bool available)> clipboardAnnounceCb,
            std::function<void()> clipboardRequestCb);
  ~Clipboard();

  static bool available();

  // Subscribe to the SelctionOwnerChanged and SelectionTransfer signals.
  // Must be called once for setSelection() and selectionRead() to work.
  void subscribe();

  // Portal methods
  void requestClipboard();
  void setSelection();
  void selectionRead();
  // Called from rfb::SDesktop, writes to all pending requests
  void selectionWrite(const char* data);

  // Resets the clipboard when data is no longer avaialble.
  void clearSelection();

private:
  // Portal methods
  void selectionWrite(PendingData pendingData, const char* data);
  void selectionWriteDone(uint32_t serial, bool success);

  // Portal method callbacks
  void handleRequestClipboard(GVariant *parameters);
  void handleSelectionWrite(GObject* proxy, GAsyncResult* res);
  void handleSelectionRead(GObject* proxy, GAsyncResult* res);

  // Portal signals
  void handleSelectionTransfer(GVariant* parameters);
  void handleSelectionOwnerChanged(GVariant* parameters);

  // Asynchronously reads the clipboard data. This will send the
  // clipboard data to the client when it has finished reading.
  void handleReadDataCallback(GAsyncResult* res);

  void clearPendingSerials();

private:
  PortalProxy* clipboard;
  std::string sessionHandle;
  gboolean selectionOwner;
  bool readInProgress;

  const char* pendingReadMimeType;
  GInputStream* readStream;
  GCancellable* readCancellable;
  std::string readBuffer;

  std::set<std::string> availableMimeTypes;
  std::queue<PendingData> pendingSerials;
  std::function<void(const char* data)> sendClipboardDataCb;
  std::function<void(bool available)> clipboardAnnounceCb;
  std::function<void()> clipboardRequestCb;
};

#endif // __PORTAL_CLIPBOARD_H__
