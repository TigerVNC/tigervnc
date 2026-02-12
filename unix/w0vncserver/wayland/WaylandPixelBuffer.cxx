/* Copyright 2026 Adam Halim for Cendio AB
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

#include <pixman.h>
#include <wayland-client-protocol.h>

#include <core/Region.h>
#include <core/string.h>
#include <core/LogWriter.h>
#include <core/Rect.h>
#include <rfb/VNCServerST.h>

#include "../w0vncserver.h"
#include "objects/Output.h"
#include "objects/Display.h"
#include "objects/ScreencopyManager.h"
#include "objects/ImageCaptureSource.h"
#include "objects/ImageCopyCaptureManager.h"
#include "WaylandPixelBuffer.h"

using namespace wayland;

static core::LogWriter vlog("WaylandPixelBuffer");

WaylandPixelBuffer::WaylandPixelBuffer(Display* display_,
                                       Output* output_,
                                       Seat* seat_,
                                       rfb::VNCServer* server_,
                                       std::function<void()> desktopReadyCallback_)
  : firstFrame(true), desktopReadyCallback(desktopReadyCallback_),
    server(server_), output(output_), screencopyManager(nullptr),
    imageCaptureSource(nullptr), imageCopyCaptureManager(nullptr),
    resized(false)
{
  std::function<void(uint8_t*, core::Region, uint32_t)> bufferEventCb =
    std::bind(&WaylandPixelBuffer::bufferEvent, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3);

  std::function<void(int, int, const core::Point&, uint32_t, const uint8_t*)>
    cursorImageCb = std::bind(&WaylandPixelBuffer::cursorImageEvent, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4,
                              std::placeholders::_5);

  std::function<void(const core::Point&)> cursorPosCb =
    std::bind(&WaylandPixelBuffer::cursorPosEvent, this, std::placeholders::_1);

  std::function<void()> stoppedCb = [this]() {
    server->closeClients("The remote session stopped");
  };

  if (display_->interfaceAvailable("ext_image_copy_capture_manager_v1") &&
      display_->interfaceAvailable("ext_output_image_capture_source_manager_v1")) {
    imageCaptureSource = new OutputImageCaptureSource(display_, output_);

    imageCopyCaptureManager = new ImageCopyCaptureManager(display_,
                                                          imageCaptureSource,
                                                          seat_,
                                                          bufferEventCb,
                                                          cursorImageCb,
                                                          cursorPosCb,
                                                          stoppedCb);
    imageCopyCaptureManager->createSession();
    imageCopyCaptureManager->createPointerCursorSession();
  } else {
    screencopyManager = new ScreencopyManager(display_, output_,
                                              bufferEventCb, stoppedCb);
  }
}

WaylandPixelBuffer::~WaylandPixelBuffer()
{
  delete screencopyManager;
  delete imageCopyCaptureManager;
  delete imageCaptureSource;
}

void WaylandPixelBuffer::cursorImageEvent(int width, int height,
                                          const core::Point& hotspot,
                                          uint32_t shmFormat,
                                          const uint8_t* src)
{
  uint8_t* cursorData;

  cursorData = nullptr;
  try {
    cursorData = convertCursorBuffer(src, shmFormat, width, height);
    server->setCursor(width, height, hotspot, cursorData);
  } catch (std::exception& e) {
    vlog.error("Failed to set cursor: %s", e.what());
  }

  delete cursorData;
}

void WaylandPixelBuffer::cursorPosEvent(const core::Point& pos)
{
  server->setCursorPos(pos, true);
}

void WaylandPixelBuffer::bufferEvent(uint8_t* buffer, core::Region damage,
                                     uint32_t shmFormat)
{
  if (output->getWidth() != (uint32_t)width() ||
      output->getHeight() != (uint32_t)height()) {
    if (!firstFrame && !resized) {
      resized = true;
      if (screencopyManager)
        screencopyManager->resize();
      return;
    }
  }

  // We need to capture our first frame before we know which format
  // the display is using.
  // FIXME: Can we query the compositor instead of doing this?
  if (firstFrame) {
    try {
      format = convertPixelformat(shmFormat);
    } catch (std::exception& e) {
      vlog.error("Failed to convert pixelformat: %s", e.what());
      server->closeClients("Failed to start remote session");
      return;
    }
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

uint8_t* WaylandPixelBuffer::convertCursorBuffer(const uint8_t* src,
                                                 uint32_t shmFormat,
                                                 uint32_t width,
                                                 uint32_t height)
{
  const unsigned char* in;
  uint8_t* out;
  uint8_t* cursorData;
  bool hasAlpha;

  cursorData = new uint8_t[width * height * 4];

  in = src;
  out = cursorData;

  hasAlpha = (shmFormat == WL_SHM_FORMAT_ARGB8888 ||
              shmFormat == WL_SHM_FORMAT_RGBA8888 ||
              shmFormat == WL_SHM_FORMAT_ABGR8888);

  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      uint8_t r, g, b, a;

      switch (shmFormat) {
        case WL_SHM_FORMAT_ARGB8888:
          b = in[0];
          g = in[1];
          r = in[2];
          a = in[3];
          break;
        case WL_SHM_FORMAT_XRGB8888:
          b = in[0];
          g = in[1];
          r = in[2];
          a = 0xff;
          break;
        case WL_SHM_FORMAT_RGBA8888:
          r = in[0];
          g = in[1];
          b = in[2];
          a = in[3];
          break;
        case WL_SHM_FORMAT_RGBX8888:
          r = in[0];
          g = in[1];
          b = in[2];
          a = 0xff;
          break;
        case WL_SHM_FORMAT_ABGR8888:
          a = in[0];
          b = in[1];
          g = in[2];
          r = in[3];
          break;
        case WL_SHM_FORMAT_XBGR8888:
          b = in[1];
          g = in[2];
          r = in[3];
          a = 0xff;
          break;
        default:
          throw std::runtime_error("Unsupported cursor format");
      }

      if (hasAlpha) {
        if (a == 0) {
          r = g = b = 0;
        } else {
          r = (uint8_t)((uint16_t)r * 255 / a);
          g = (uint8_t)((uint16_t)g * 255 / a);
          b = (uint8_t)((uint16_t)b * 255 / a);
        }
      }

      *out++ = r;
      *out++ = g;
      *out++ = b;
      *out++ = a;

      in += 4;
    }
  }

  return cursorData;
}

rfb::PixelFormat WaylandPixelBuffer::convertPixelformat(uint32_t shmFormat)
{
  switch (shmFormat) {
    case WL_SHM_FORMAT_XRGB8888:
    case WL_SHM_FORMAT_ARGB8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              16, 8, 0);
    case WL_SHM_FORMAT_RGBX8888:
    case WL_SHM_FORMAT_RGBA8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              24, 16, 8);
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              0, 8, 16);
    default:
      throw std::runtime_error(core::format("format %d not supported",
                                            shmFormat));
  }
}
