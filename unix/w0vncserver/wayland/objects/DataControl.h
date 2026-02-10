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

#ifndef __WAYLAND_DATA_CONTROL_H__
#define __WAYLAND_DATA_CONTROL_H__

struct ext_data_control_manager_v1;
struct ext_data_control_device_v1;
struct ext_data_control_source_v1;
struct ext_data_control_offer_v1;
struct ext_data_control_device_v1_listener;
struct ext_data_control_source_v1_listener;
struct ext_data_control_offer_v1_listener;

#include <functional>
#include <string>
#include <queue>
#include <set>

#include <gio/gio.h>

#include "Object.h"

struct PendingData;

namespace wayland {
  class Seat;

  class DataControl : public Object {
  public:
    DataControl(Display* display, Seat* seat,
                std::function<void(bool available)> clipboardAnnounceCb,
                std::function<void()> clipboardRequestCb,
                std::function<void(const char* data)> sendClipboardDataCb);
    virtual ~DataControl();

    // ext_data_control_device_v1 functions
    void setSelection();
    void setPrimarySelection();

    void clearSelection();
    void clearPrimarySelection();

    // ext_data_control_offer_v1 functions
    void receive();

    // Write to all pending clipboard requests
    void writePending(const char* data);

  private:
    // ext_data_control_device_v1 handlers
    void handleDataOffer(ext_data_control_offer_v1* offer);
    void handleSelection(ext_data_control_offer_v1* offer);
    void handlePrimarySelection(ext_data_control_offer_v1* offer);
    void handleFinished();

    // ext_data_control_source_v1 handlers
    void handleSend(const char* mimeType, int32_t fd);
    void handleCancelled(ext_data_control_source_v1* source);

    // ext_data_control_offer_v1 handlers
    void handleOffer(const char* mimeType);

    void handleReadDataCallback(GAsyncResult* res);

    ext_data_control_source_v1* createDataSource();

    void clearPendingWrites();

  private:
    ext_data_control_manager_v1* manager;
    ext_data_control_device_v1* device;

    // Keep track of the most recent offer
    ext_data_control_offer_v1* lastOffer;

    ext_data_control_source_v1* selectionSource;
    ext_data_control_source_v1* primarySource;

    bool readInProgress;
    const char* pendingReadMimeType;
    GInputStream* readStream;
    GCancellable* readCancellable;
    std::string readBuffer;


    std::set<std::string> availableMimeTypes;
    std::queue<PendingData> pendingWrites;

    std::function<void(bool available)> clipboardAnnounceCb;
    std::function<void()> clipboardRequestCb;
    std::function<void(const char* data)> sendClipboardDataCb;

    static const ext_data_control_device_v1_listener deviceListener;
    static const ext_data_control_source_v1_listener sourceListener;
    static const ext_data_control_offer_v1_listener offerListener;
  };
};

#endif // __WAYLAND_DATA_CONTROL_H__
