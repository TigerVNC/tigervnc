/* Copyright 2025-2026 Adam Halim for Cendio AB
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
#include <unistd.h>

#include <stdexcept>

#include <pixman.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

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

static core::LogWriter vlog("WaylandPixelBuffer");

WaylandPixelBuffer::WaylandPixelBuffer(wayland::Display* display,
                                       wayland::Output* output_,
                                       wayland::Seat* seat,
                                       rfb::VNCServer* server_,
                                       std::function<void()> desktopReadyCallback_)
  : firstFrame(true), desktopReadyCallback(desktopReadyCallback_),
    server(server_), output(output_), screencopyManager(nullptr),
    imageCaptureSource(nullptr), imageCopyCaptureManager(nullptr),
    resized(false)
{
  std::function<void(uint8_t*, core::Region, uint32_t, uint32_t)> bufferEventCb =
    std::bind(&WaylandPixelBuffer::bufferEvent, this, std::placeholders::_1,
              std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

  std::function<void(int, int, const core::Point&, uint32_t, uint32_t, const uint8_t*)>
    cursorImageCb = std::bind(&WaylandPixelBuffer::cursorImageEvent, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3, std::placeholders::_4,
                              std::placeholders::_5, std::placeholders::_6);

  std::function<void(const core::Point&)> cursorPosCb =
    std::bind(&WaylandPixelBuffer::cursorPosEvent, this, std::placeholders::_1);

  std::function<void()> stoppedCb = [this]() {
    server->closeClients("The remote session stopped");
  };

  if (display->interfaceAvailable("ext_image_copy_capture_manager_v1") &&
      display->interfaceAvailable("ext_output_image_capture_source_manager_v1")) {
    imageCaptureSource = new wayland::OutputImageCaptureSource(display, output_);

    imageCopyCaptureManager = new wayland::ImageCopyCaptureManager(display,
                                                                   imageCaptureSource,
                                                                   seat,
                                                                   bufferEventCb,
                                                                   cursorImageCb,
                                                                   cursorPosCb,
                                                                   stoppedCb);
    imageCopyCaptureManager->createSession();
  } else {
    screencopyManager = new wayland::ScreencopyManager(display, output_,
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
                                          uint32_t transform,
                                          const uint8_t* src)
{
  const uint8_t* cursorSrc;
  uint8_t* cursorData;
  uint8_t* transformedCursorData;

  cursorData = nullptr;
  transformedCursorData = nullptr;
  cursorSrc = src;
  try {
    if (transform != WL_OUTPUT_TRANSFORM_NORMAL) {
      transformedCursorData = undoTransformation(src, width, height,
                                                 width * 4, shmFormat,
                                                 transform);
      cursorSrc = transformedCursorData;
    }
    cursorData = convertCursorBuffer(cursorSrc, shmFormat, width, height);
    server->setCursor(width, height, hotspot, cursorData);
  } catch (std::exception& e) {
    vlog.error("Failed to set cursor: %s", e.what());
  }

  delete [] cursorData;
  delete [] transformedCursorData;
}

void WaylandPixelBuffer::cursorPosEvent(const core::Point& pos)
{
  server->setCursorPos(pos, true);
}

void WaylandPixelBuffer::bufferEvent(uint8_t* buffer, core::Region damage,
                                     uint32_t shmFormat, uint32_t transform)
{
  uint32_t bufferWidth;
  uint32_t bufferHeight;
  bool transformed;
  bool rotated;

  bufferWidth = output->getWidth();
  bufferHeight = output->getHeight();
  transformed = false;
  rotated = false;

  transformed = transform != WL_OUTPUT_TRANSFORM_NORMAL;

  rotated = transform == WL_OUTPUT_TRANSFORM_90 ||
            transform == WL_OUTPUT_TRANSFORM_270 ||
            transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
            transform == WL_OUTPUT_TRANSFORM_FLIPPED_270;

  if (rotated) {
    bufferWidth = output->getHeight();
    bufferHeight = output->getWidth();
  }

  if (bufferWidth != (uint32_t)width() || bufferHeight != (uint32_t)height()) {
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
      format = shmToRfbFormat(shmFormat);
    } catch (std::exception& e) {
      vlog.error("Failed to convert pixelformat: %s", e.what());
      server->closeClients("Failed to start remote session");
      return;
    }

    if (rotated)
      setSize(output->getHeight(), output->getWidth());
    else
      setSize(output->getWidth(), output->getHeight());

    desktopReadyCallback();
  }

  firstFrame = false;

  if (transformed) {
    try {
      syncBuffersTransformed(buffer, damage, shmFormat, transform);
    } catch (std::exception& e) {
      vlog.error("Failed to sync transformed buffers: %s", e.what());
      server->closeClients("Error copying buffers");
      return;
    }
  } else {
    syncBuffers(buffer, damage);
  }
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

void WaylandPixelBuffer::syncBuffersTransformed(uint8_t* buffer,
                                                core::Region damage,
                                                uint32_t shmFormat,
                                                uint32_t transform)
{
  int srcWidth;
  int srcHeight;
  int dstWidth;
  int dstHeight;
  bool wasResized;
  core::Region region;
  std::vector<core::Rect> rects;
  std::vector<core::Rect> transformedRects;
  core::Region transformedDamage;
  bool rotated;
  pixman_format_code_t pixmanFormat;
  pixman_image_t* srcImg;

  assert(transform != WL_OUTPUT_TRANSFORM_NORMAL);

  rotated = transform == WL_OUTPUT_TRANSFORM_90 ||
            transform == WL_OUTPUT_TRANSFORM_270 ||
            transform == WL_OUTPUT_TRANSFORM_FLIPPED_90 ||
            transform == WL_OUTPUT_TRANSFORM_FLIPPED_270;

  wasResized = resized;
  if (resized) {
    if (rotated)
      setSize(output->getHeight(), output->getWidth());
    else
      setSize(output->getWidth(), output->getHeight());
    server->setPixelBuffer(this);
    resized = false;
  }

  region = damage;

  if (region.is_empty())
    region = getRect();

  dstWidth = width();
  dstHeight = height();
  if (rotated) {
    srcWidth = dstHeight;
    srcHeight = dstWidth;
  } else {
    srcWidth = dstWidth;
    srcHeight = dstHeight;
  }

  region.get_rects(&rects);

  if (!wasResized) {
    // Transform the damage rects
    for (core::Rect &rect : rects) {
      int x;
      int y;
      int w;
      int h;
      int dx;
      int dy;
      int dw;
      int dh;

      x = rect.tl.x;
      y = rect.tl.y;
      w = rect.width();
      h = rect.height();

      switch (transform) {
      case WL_OUTPUT_TRANSFORM_NORMAL:
        dx = x;
        dy = y;
        break;
      case WL_OUTPUT_TRANSFORM_90:
        dx = srcHeight - y - h;
        dy = x;
        break;
      case WL_OUTPUT_TRANSFORM_180:
        dx = srcWidth - x - w;
        dy = srcHeight - y - h;
        break;
      case WL_OUTPUT_TRANSFORM_270:
        dx = y;
        dy = srcWidth - x - w;
        break;
      case WL_OUTPUT_TRANSFORM_FLIPPED:
        dx = srcWidth - x - w;
        dy = y;
        break;
      case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        dx = y;
        dy = x;
        break;
      case WL_OUTPUT_TRANSFORM_FLIPPED_180:
        dx = x;
        dy = srcHeight - y - h;
        break;
      case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        dx = srcHeight - y - h;
        dy = srcWidth - x - w;
        break;
      default:
        throw std::runtime_error(core::format("Unknown transform: %d", transform));
      }

      if (rotated) {
        dw = h;
        dh = w;
      } else {
        dw = w;
        dh = h;
      }

      transformedDamage.assign_union({{dx, dy, dx + dw, dy + dh}});
    }
  } else {
    // Mark entire screen as dirty when we resize
    transformedDamage = getRect();
  }

  pixmanFormat = shmToPixmanFormat(shmFormat);

  srcImg = pixman_image_create_bits(pixmanFormat, srcWidth, srcHeight,
                                   (uint32_t*)(buffer), srcWidth * 4);

  transformedDamage.get_rects(&transformedRects);
  for (core::Rect &rect : transformedRects) {
    uint8_t* dstBuffer;
    int dstStride;
    pixman_image_t* dstImg;
    pixman_transform_t inverseTransform;

    dstBuffer = getBufferRW(getRect(), &dstStride);

    dstImg = pixman_image_create_bits(pixmanFormat, dstWidth, dstHeight,
                                     (uint32_t*)(dstBuffer), dstStride * 4);

    inverseTransform = wlToPixmanInverseTransform(transform, srcWidth, srcHeight);
    pixman_image_set_transform(srcImg, &inverseTransform);

    pixman_image_composite32(PIXMAN_OP_SRC, srcImg, nullptr, dstImg,
                             rect.tl.x, rect.tl.y, 0, 0, rect.tl.x,
                             rect.tl.y, rect.width(), rect.height());

    pixman_image_unref(dstImg);
    commitBufferRW(rect);
  }

  pixman_image_unref(srcImg);
  server->add_changed(transformedDamage);
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

rfb::PixelFormat WaylandPixelBuffer::shmToRfbFormat(uint32_t shmFormat)
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

pixman_format_code_t WaylandPixelBuffer::shmToPixmanFormat(uint32_t shmFormat)
{
  switch (shmFormat) {
    case WL_SHM_FORMAT_XRGB8888:
      return PIXMAN_x8r8g8b8;
    case WL_SHM_FORMAT_ARGB8888:
      return PIXMAN_a8r8g8b8;
    case WL_SHM_FORMAT_RGBX8888:
      return PIXMAN_r8g8b8x8;
    case WL_SHM_FORMAT_RGBA8888:
      return PIXMAN_r8g8b8a8;
    case WL_SHM_FORMAT_XBGR8888:
      return PIXMAN_x8b8g8r8;
    case WL_SHM_FORMAT_ABGR8888:
      return PIXMAN_a8b8g8r8;
   default:
    throw std::runtime_error(core::format("format %d not supported", shmFormat));
  }
}

uint8_t* WaylandPixelBuffer::undoTransformation(const uint8_t* srcBuffer,
                                                int srcWidth,
                                                int srcHeight,
                                                int srcStride,
                                                uint32_t shmFormat,
                                                uint32_t transform)
{
  pixman_image_t* srcImg;
  pixman_image_t* dstImg;
  pixman_format_code_t pixmanFormat;
  pixman_transform_t pxTransform;
  uint8_t* dstBuffer;
  int dstWidth;
  int dstHeight;
  int dstStride;

  if (transform == WL_OUTPUT_TRANSFORM_NORMAL ||
      transform == WL_OUTPUT_TRANSFORM_FLIPPED) {
    dstWidth = srcWidth;
    dstHeight = srcHeight;
  } else {
    dstWidth = srcHeight;
    dstHeight = srcWidth;
  }

  dstStride = dstWidth * 4;
  dstBuffer = new uint8_t[dstHeight * dstStride];

  pixmanFormat = shmToPixmanFormat(shmFormat);
  srcImg = pixman_image_create_bits(pixmanFormat, srcWidth, srcHeight,
                                   (uint32_t*)(srcBuffer), srcStride);
  dstImg = pixman_image_create_bits(pixmanFormat, dstWidth, dstHeight,
                                   (uint32_t*)(dstBuffer), dstStride);

  pxTransform = wlToPixmanInverseTransform(transform, srcWidth, srcHeight);
  pixman_image_set_transform(srcImg, &pxTransform);

  pixman_image_composite32(PIXMAN_OP_SRC, srcImg, nullptr, dstImg,
                           0, 0, 0, 0, 0, 0, dstWidth, dstHeight);

  pixman_image_unref(srcImg);
  pixman_image_unref(dstImg);

  return dstBuffer;
}

pixman_transform_t WaylandPixelBuffer::wlToPixmanInverseTransform(uint32_t transform,
                                                                  int srcWidth, int srcHeight)
{
  pixman_fixed_t w;
  pixman_fixed_t h;

  w = srcWidth * pixman_fixed_1;
  h = srcHeight * pixman_fixed_1;

  switch (transform) {
  case WL_OUTPUT_TRANSFORM_NORMAL:
    return {{{pixman_fixed_1, 0, 0},
             {0, pixman_fixed_1, 0},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_90:
    return {{{0, pixman_fixed_1, 0},
             {-pixman_fixed_1, 0, h},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_180:
    return {{{-pixman_fixed_1, 0, w},
             {0, -pixman_fixed_1, h},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_270:
    return {{{0, -pixman_fixed_1, w},
             {pixman_fixed_1, 0, 0},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_FLIPPED:
    return {{{-pixman_fixed_1, 0, w},
             {0, pixman_fixed_1, 0},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_FLIPPED_90:
    return {{{0, pixman_fixed_1, 0},
             {pixman_fixed_1, 0, 0},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_FLIPPED_180:
    return {{{pixman_fixed_1, 0, 0},
             {0, -pixman_fixed_1, h},
             {0, 0, pixman_fixed_1}}};
  case WL_OUTPUT_TRANSFORM_FLIPPED_270:
    return {{{0, -pixman_fixed_1, w},
             {-pixman_fixed_1, 0, h},
             {0, 0, pixman_fixed_1}}};
  default:
    assert(false);
  }
}
