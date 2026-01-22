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
  : firstFrame(true), desktopReadyCallback(desktopReadyCallback_),
    server(server_), output(output_), resized(false)
{
  std::function<void(uint8_t*, core::Region, rfb::PixelFormat)> bufferEventCb =
    std::bind(&WaylandPixelBuffer::bufferEvent, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3);

  screencopyManager = new wayland::ScreencopyManager(display, output_,
                                                     bufferEventCb);
}

WaylandPixelBuffer::~WaylandPixelBuffer()
{
  delete screencopyManager;
}

void WaylandPixelBuffer::bufferEvent(uint8_t* buffer, core::Region damage, rfb::PixelFormat pf)
{
  if (output->getWidth() != (uint32_t)width() ||
      output->getHeight() != (uint32_t)height()) {
    if (!firstFrame && !resized) {
      vlog.debug("Detected resize, calling resize()");
      screencopyManager->resize();
      resized = true;
      return;
    }
  }

  // We need to capture our first frame before we know which format
  // the display is using.
  // FIXME: Can we query the compositor instead of doing this?
  if (firstFrame) {
    format = pf;
    setSize(output->getWidth(), output->getHeight());
    desktopReadyCallback();
  }

  firstFrame = false;

  syncBuffers(buffer, damage);
}

void WaylandPixelBuffer::syncBuffers(uint8_t* buffer, core::Region damage)
{
  int srcStride;
  int dstStride;
  uint8_t* srcBuffer;
  uint8_t* dstBuffer;
  pixman_bool_t ret;
  core::Region region;
  std::vector<core::Rect> rects;

  if (resized) {
    setSize(output->getWidth(), output->getHeight());
    server->setPixelBuffer(this);
    damage = getRect();
    resized = false;
  }

  region = damage;

  if (region.is_empty())
    region = getRect();

  srcBuffer = buffer;
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

  server->add_changed(damage);
}
