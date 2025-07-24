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

#ifndef __WAYLAND_SCREENCOPY_MANAGER_H__
#define __WAYLAND_SCREENCOPY_MANAGER_H__

#include <stddef.h>

#include <core/Region.h>

#include "Object.h"

namespace rfb { class PixelFormat; }

struct wl_buffer;
struct zwlr_screencopy_manager_v1;
struct zwlr_screencopy_frame_v1;
struct zwlr_screencopy_frame_v1_listener;
struct BufferInfo;

namespace wayland {
  class Display;
  class Shm;
  class ShmPool;
  class Output;

  class ScreencopyManager : public Object {
  public:
    ScreencopyManager(Display* display, Output* output);
    virtual ~ScreencopyManager();

    uint8_t* getBufferData();

    // Capture the next frame. This function is asynchronous.
    // Framebuffer data is available after handleScreencopyReady() has
    // been called, in which case captureFrameDone() will be called.
    virtual void captureFrame() = 0;

  protected:
    // Called when the buffer is safe to read from, the frame is ready.
    virtual void captureFrameDone() = 0;

    // Called when the remote output is resized
    virtual void resize() = 0;

    rfb::PixelFormat getPixelFormat();

    core::Region getDamage() { return accumulatedDamage; }

  private:
    void initBuffers(size_t size);
    rfb::PixelFormat convertPixelformat(uint32_t format);

    // zwlr_screencopy_frame_v1_listener handlers
    void handleScreencopyBuffer(uint32_t format, uint32_t width, uint32_t height,
                                uint32_t stride);
    void handleScreencopyFlags(uint32_t flags);
    void handleScreencopyReady();
    void handleScreencopyFailed();
    void handleScreencopyDamage(uint32_t x, uint32_t y, uint32_t width,
                                uint32_t height);
    void handleScreencopyLinuxDmabuf(uint32_t format, uint32_t width,
                                    uint32_t height);
    void handleScreencopyBufferDone();

  protected:
    Output* output;

  private:
    zwlr_screencopy_manager_v1* screencopyManager;
    zwlr_screencopy_frame_v1* frame;
    BufferInfo* info;
    Shm* shm;
    ShmPool* pool;
    wl_buffer* buffer;
    core::Region accumulatedDamage;
    static const zwlr_screencopy_frame_v1_listener listener;
  };
};

#endif // __WAYLAND_SCREENCOPY_MANAGER_H__
