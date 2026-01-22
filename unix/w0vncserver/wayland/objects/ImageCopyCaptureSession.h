/* Copyright 2026 Tobias Fahleson for Cendio AB
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

#ifndef __WAYLAND_IMAGE_COPY_CAPTURE_SESSION_H__
#define __WAYLAND_IMAGE_COPY_CAPTURE_SESSION_H__

#include <functional>
#include <vector>

#include <core/Region.h>

#include "Object.h"

struct wl_array;
struct wl_buffer;

struct ext_image_copy_capture_session_v1;
struct ext_image_copy_capture_frame_v1;
struct ext_image_copy_capture_session_v1_listener;
struct ext_image_copy_capture_frame_v1_listener;

namespace wayland {
  class Display;
  class Shm;
  class ShmPool;

  class ImageCopyCaptureSession {
  public:
    ImageCopyCaptureSession(Display* display,
                            ext_image_copy_capture_session_v1* session,
                            std::function<void(uint8_t*, core::Region, uint32_t)>
                              frameReadyCb,
                            std::function<void()> stoppedCb);
    ~ImageCopyCaptureSession();

    uint8_t* getBufferData() const;
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    uint32_t getFormat() const { return format; }

  private:
    void handleBufferSize(uint32_t width, uint32_t height);
    void handleShmFormat(uint32_t format);
    void handleDmabufDevice(wl_array* device);
    void handleDmabufFormat(uint32_t format, wl_array* modifiers);
    void handleDone();
    void handleStopped();
    void handleFrameTransform(uint32_t transform);
    void handleFrameDamage(int32_t x, int32_t y, int32_t width,
                           int32_t height);
    void handleFramePresentationTime(uint32_t tvSecHi, uint32_t tvSecLo,
                                     uint32_t tvNsec);
    void handleFrameReady();
    void handleFrameFailed(uint32_t reason);

    void createFrame();
    void cleanupBuffers();

    uint32_t preferredFormat();

  private:
    Display* display;
    std::function<void(uint8_t*, core::Region, uint32_t)> frameReadyCb;
    ext_image_copy_capture_session_v1* session;
    ext_image_copy_capture_frame_v1* frame;
    Shm* shm;
    ShmPool* pool;
    wl_buffer* buffer;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    std::function<void()> stoppedCb;
    std::vector<uint32_t> formats;
    core::Region damage;
    static const ext_image_copy_capture_session_v1_listener listener;
    static const ext_image_copy_capture_frame_v1_listener frameListener;
  };

};

#endif // __WAYLAND_IMAGE_COPY_CAPTURE_SESSION_H__
