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

namespace rfb { class VNCServer; }

namespace wayland {
  class Output;
  class Display;
};

class WaylandPixelBuffer : public rfb::ManagedPixelBuffer,
                           public wayland::ScreencopyManager {
public:
  WaylandPixelBuffer(wayland::Display* display, wayland::Output* output,
                     rfb::VNCServer* server,
                     std::function<void()> desktopReadyCallback);
  ~WaylandPixelBuffer();

protected:
  virtual void captureFrame() override;
  virtual void captureFrameDone() override;
  virtual void resize() override;

private:
  // Sync the shadow framebuffer to the actual framebuffer
  void syncBuffers();

private:
  bool firstFrame;
  uint8_t* shadowFramebuffer;
  std::function<void()> desktopReadyCallback;
  rfb::VNCServer* server;
};

#endif // __WAYLAND_PIXELBUFFER_H__
