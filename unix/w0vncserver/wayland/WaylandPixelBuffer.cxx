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

#include <assert.h>
#include <stdexcept>
#include <unistd.h>

#include <pixman.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include <core/Region.h>
#include <core/LogWriter.h>
#include <rfb/VNCServerST.h>

#include "../w0vncserver.h"
#include "objects/Output.h"
#include "objects/Display.h"
#include "objects/ScreencopyManager.h"
#include "WaylandPixelBuffer.h"

static core::LogWriter vlog("WaylandPixelBuffer");

WaylandPixelBuffer::WaylandPixelBuffer(wayland::Display* display,
                                       wayland::Output* output_,
                                       rfb::VNCServer* server_,
                                       std::function<void()> desktopReadyCallback_)
  : wayland::ScreencopyManager(display, output_), firstFrame(true),
    shadowFramebuffer(nullptr),
    desktopReadyCallback(desktopReadyCallback_), server(server_)
{
  captureFrame();
}

WaylandPixelBuffer::~WaylandPixelBuffer()
{
  delete [] shadowFramebuffer;
}

void WaylandPixelBuffer::captureFrame()
{
  if (output->getWidth() != (uint32_t)width() ||
      output->getHeight() != (uint32_t)height()) {
    if (!firstFrame) {
      vlog.debug("Detected resize, calling resize()");
      resize();
    }
  }

  ScreencopyManager::captureFrame();
}

void WaylandPixelBuffer::captureFrameDone()
{
  // We need to capture our first frame before we know which format
  // the display is using.
  // FIXME: Can we query the compositor instead of doing this?
  if (firstFrame) {
    try {
      format = getPixelFormat();
    } catch (std::runtime_error& e) {
      fatal_error("Failed to get pixel format: %s", e.what());
      return;
    }

    // Utilize a shadow framebuffer, as we have no way of accessing
    // the screen contents directly.
    shadowFramebuffer = new uint8_t[output->getWidth() *
                                    output->getHeight() * (format.bpp / 8)];

    setSize(output->getWidth(), output->getHeight());

    desktopReadyCallback();
  }

  firstFrame = false;

  syncBuffers();

  ScreencopyManager::captureFrameDone();

  server->add_changed(getDamage());

  captureFrame();
}

void WaylandPixelBuffer::resize()
{
  ScreencopyManager::resize();

  delete [] shadowFramebuffer;

  shadowFramebuffer = new uint8_t[output->getWidth() *
                                  output->getHeight() * (format.bpp / 8)];

  setSize(output->getWidth(), output->getHeight());

  syncBuffers();

  server->setPixelBuffer(this);
}

void WaylandPixelBuffer::syncBuffers()
{
  int srcStride;
  int dstStride;
  uint8_t* srcBuffer;
  uint8_t* dstBuffer;
  pixman_bool_t ret;
  core::Region region;
  std::vector<core::Rect> rects;

  region = getDamage();

  if (region.is_empty())
    region = getRect();

  srcBuffer = getBufferData();
  srcStride = width();

  region.get_rects(&rects);
  for (core::Rect &rect : rects) {
    dstBuffer = getBufferRW(getRect(), &dstStride);
    ret = pixman_blt((uint32_t*)srcBuffer, (uint32_t*)dstBuffer,
                     srcStride, dstStride, format.bpp, format.bpp,
                     rect.tl.x, rect.tl.y, rect.tl.x, rect.tl.y,
                     rect.width(), rect.height());
    commitBufferRW(rect);

    if (!ret) {
      uint8_t* damagedBuffer;

      damagedBuffer = &srcBuffer[(format.bpp / 8) *
                                 (rect.tl.y * srcStride + rect.tl.x)];
      imageRect(rect, damagedBuffer, srcStride);
    }
  }
}
