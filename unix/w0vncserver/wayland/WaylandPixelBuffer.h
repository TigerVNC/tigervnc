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

#ifndef __WAYLAND_PIXELBUFFER_H__
#define __WAYLAND_PIXELBUFFER_H__

#include <functional>

#include <rfb/PixelBuffer.h>

#include "objects/ScreencopyManager.h"

namespace rfb { class VNCServer; class PixelFormat; }

namespace wayland {
  class Output;
  class Display;
};

class WaylandPixelBuffer : public rfb::ManagedPixelBuffer {
public:
  WaylandPixelBuffer(wayland::Display* display, wayland::Output* output,
                     rfb::VNCServer* server,
                     std::function<void()> desktopReadyCallback);
  ~WaylandPixelBuffer();

protected:
  // Called when there is pixel data available to read
  void bufferEvent(uint8_t* buffer, core::Region damage, uint32_t format);

private:
  // Sync the shadow framebuffer to the actual framebuffer
  void syncBuffers(uint8_t* buffer, core::Region damage);

  // Convert from wl_shm_format to rfb::PixelFormat
  rfb::PixelFormat convertPixelformat(uint32_t format);

private:
  bool firstFrame;
  std::function<void()> desktopReadyCallback;
  rfb::VNCServer* server;
  wayland::Output* output;
  wayland::ScreencopyManager* screencopyManager;
  bool resized;
};

#endif // __WAYLAND_PIXELBUFFER_H__
