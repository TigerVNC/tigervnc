/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

// -=- PixelBuffer.cxx
//
// The PixelBuffer class encapsulates the PixelFormat and dimensions
// of a block of pixel data.

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/LogWriter.h>
#include <core/Region.h>
#include <core/string.h>

#include <rfb/OverlayPixelBuffer.h>
#include "rfb/OverlayContentPng.h"
#include "rfb/OverlayContentQr.h"
#include "rfb/OverlayContentText.h"

#include <pixman.h>

using namespace rfb;

static core::LogWriter vlog("OverlayPixelBuffer");

OverlayPixelBuffer::OverlayPixelBuffer(const PixelBuffer *parentBuf,
                                       const char *overlayType,
                                       const char *overlayPos,
                                       const char *overlayInput,
                                       const int overlayAlpha,
                                       int overlaySize)
    : ManagedPixelBuffer(parentBuf->getPF(), parentBuf->width(),
                         parentBuf->height()),
      parent(parentBuf),
      overlayBuffer(new uint8_t[width() * height() * (format.bpp / 8)]),
      _content(nullptr),
      _overlayType(overlayType),
      _overlayPos(overlayPos),
      _overlayInput(overlayInput), _overlaySize(overlaySize),
      _overlayPadding(10), _overlayAlpha(overlayAlpha) {
  vlog.debug("Setting overlay position to: %s", overlayPos);
  renderOverlay();
  syncBuffers(getRect());
}

void OverlayPixelBuffer::updateOverlay(const char *overlayType,
                                       const char *overlayPos,
                                       const char *overlayInput,
                                       const int overlayAlpha,
                                       int overlaySize) {
  vlog.debug("Updating overlay to position %s, text %s, font size %d",
             overlayPos, overlayInput, overlaySize);

  _overlayType = overlayType;
  _overlayPos = overlayPos;
  _overlayInput = overlayInput;
  _overlayAlpha = overlayAlpha;
  _overlaySize = overlaySize;

  renderOverlay();
  syncBuffers(getRect());
}

void OverlayPixelBuffer::setParent(const PixelBuffer *parentBuf) {
  parent = parentBuf;

  // If the parent buffer has changed size, resize the overlay buffer to match
  if ((width() != parent->width()) || (height() != parent->height()))
    // syncBuffers() is called later in setSize()
    setSize(parent->width(), parent->height());
  else
    syncBuffers(getRect());
}

void OverlayPixelBuffer::setSize(int w, int h) {
  ManagedPixelBuffer::setSize(w, h);

  delete[] overlayBuffer;
  overlayBuffer = new uint8_t[width() * height() * (format.bpp / 8)];

  // Computes a new overlay rectangle position based on the new size
  renderOverlay();
  vlog.debug("Overlay position after resize: %s", _overlayPos.c_str());
  syncBuffers(getRect());
}

OverlayPixelBuffer::~OverlayPixelBuffer() {
  delete[] overlayBuffer;
  delete _content;
}

core::Point OverlayPixelBuffer::calcOverlayPosition(const char *overlayPos,
                                             int contentWidth,
                                             int contentHeight) const {
  int x, y;

  if (strcmp(overlayPos, "tl") == 0) {
    // Top-Left corner
    x = _overlayPadding;
    y = _overlayPadding;
  } else if (strcmp(overlayPos, "tr") == 0) {
    // Top-Right corner
    x = width() - contentWidth - _overlayPadding;
    y = _overlayPadding;
  } else if (strcmp(overlayPos, "bl") == 0) {
    // Bottom-Left corner
    x = _overlayPadding;
    y = height() - contentHeight - _overlayPadding;
  } else if (strcmp(overlayPos, "br") == 0) {
    // Bottom-Right corner
    x = width() - contentWidth - _overlayPadding;
    y = height() - contentHeight - _overlayPadding;
  } else if (strcmp(overlayPos, "c") == 0) {
    // Centered
    x = (width() - contentWidth) / 2;
    y = (height() - contentHeight) / 2;
  } else {
    vlog.error("Invalid overlay position specified: %s", overlayPos);
    return core::Point(0, 0);
  }

  return core::Point(x, y);
}

std::vector<core::Rect>
OverlayPixelBuffer::calcOverlayPositions(const char *overlayPos,
                                         int contentWidth,
                                         int contentHeight) const {
  std::vector<core::Rect> rects;

  for (const std::string &pos : core::split(overlayPos, ',')) {
    core::Point singleOverlayPos = calcOverlayPosition(pos.c_str(), contentWidth,
                                         contentHeight);
    rects.push_back(core::Rect(singleOverlayPos.x, singleOverlayPos.y, singleOverlayPos.x + contentWidth,
                               singleOverlayPos.y + contentHeight));
  }

  return rects;
}

void OverlayPixelBuffer::renderOverlay() {
  delete _content;
  _content = nullptr;
  _overlayRects.clear();

  if (_overlayInput.empty())
    return;

  // calculate the pixel-height of the overlay
  int targetHeight = std::max(
      1, static_cast<int>(std::lround(_overlaySize / 100.0 * height())));

  if (_overlayType == "png")
    _content = new OverlayContentPng(_overlayInput, targetHeight);
  else if (_overlayType == "qr")
    _content = new OverlayContentQr(_overlayInput, targetHeight);
  else
    _content = new OverlayContentText(_overlayInput, targetHeight);

  if (!_content->getContentPixelBuffer()) {
    delete _content;
    _content = nullptr;
    return;
  }

  _overlayRects = calcOverlayPositions(_overlayPos.c_str(),
                                       _content->getWidth(),
                                       _content->getHeight());
}

void OverlayPixelBuffer::blendBuffer(const uint8_t *buf, int bufWidth,
                                     int bufHeight, const core::Point &pos,
                                     double alpha) const {
  if (!buf)
    return;

  vlog.debug("Blending %dx%d overlay buffer at %d,%d with alpha %.2f",
             bufWidth, bufHeight, pos.x, pos.y, alpha);

  // 1. Map the buffer's bits-per-pixel (bpp) to a corresponding Pixman format
  pixman_format_code_t pixmanFormat;
  switch (format.bpp) {
  case 32:
    pixmanFormat = PIXMAN_a8r8g8b8;
    break; // Use your system's exact format (e.g., PIXMAN_a8b8g8r8) if colors
           // appear swapped
  case 24:
    pixmanFormat = PIXMAN_r8g8b8;
    break;
  case 16:
    pixmanFormat = PIXMAN_r5g6b5;
    break;
  case 8:
    pixmanFormat = PIXMAN_a8;
    break;
  default:
    vlog.error("Unsupported bits-per-pixel (%d) for Pixman wrapper",
               format.bpp);
    return;
  }

  // 2. Calculate row stride in bytes
  int bytesPerPixel = format.bpp / 8;
  int rowStrideBytes = width() * bytesPerPixel;

  // 3. Wrap the raw overlayBuffer inside a pixman image view
  // Note: const_cast is used because blendBuffer is marked const, but we are
  // writing data to the target buffer
  pixman_image_t *destImage = pixman_image_create_bits(
      pixmanFormat, width(), height(),
      reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(overlayBuffer)),
      rowStrideBytes);
  if (!destImage) {
    vlog.error("Failed to create Pixman image surface wrapper.");
    return;
  }

  // 4. Wrap the source (text) buffer, which is always tightly packed ARGB32
  pixman_image_t *srcImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, bufWidth, bufHeight,
      reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(buf)),
      bufWidth * 4);
  if (!srcImage) {
    vlog.error("Failed to create Pixman image surface wrapper for overlay.");
    pixman_image_unref(destImage);
    return;
  }

  // 5. A solid alpha mask that multiplies the source's own per-pixel alpha
  // by the requested overall alpha, giving the watermark its translucency.
  // alpha arrives as a 0-100 percentage (OverlayAlpha), so convert it to a
  // 0.0-1.0 fraction before clamping.
  alpha /= 100.0;
  if (alpha < 0.0)
    alpha = 0.0;
  else if (alpha > 1.0)
    alpha = 1.0;

  pixman_color_t alphaColor;
  alphaColor.red = alphaColor.green = alphaColor.blue = 0;
  alphaColor.alpha = static_cast<uint16_t>(alpha * 0xffff);
  pixman_image_t *alphaMask = pixman_image_create_solid_fill(&alphaColor);

  // 6. Composite the source onto the destination at the given position
  pixman_image_composite(PIXMAN_OP_OVER, srcImage, alphaMask, destImage, 0, 0,
                         0, 0, static_cast<int16_t>(pos.x),
                         static_cast<int16_t>(pos.y),
                         static_cast<uint16_t>(bufWidth),
                         static_cast<uint16_t>(bufHeight));

  // 7. Clean up the Pixman wrappers (this does not free the underlying
  // buffers)
  pixman_image_unref(alphaMask);
  pixman_image_unref(srcImage);
  pixman_image_unref(destImage);
}

void OverlayPixelBuffer::syncBuffers(const core::Region &r) {
  std::vector<core::Rect> rects;
  int bytesPerPixel = format.bpp / 8;

  r.get_rects(&rects);

  for (const core::Rect &rect : rects) {
    core::Rect clipped = rect.intersect(parent->getRect());
    if (clipped.is_empty())
      continue;

    int parentStride;
    const uint8_t *src = parent->getBuffer(clipped, &parentStride);
    uint8_t *dst =
        overlayBuffer + (clipped.tl.y * width() + clipped.tl.x) * bytesPerPixel;

    int rowBytes = clipped.width() * bytesPerPixel;
    int h = clipped.height();
    while (h--) {
      memcpy(dst, src, rowBytes);
      dst += width() * bytesPerPixel;
      src += parentStride * bytesPerPixel;
    }
  }

  // Only redraw a position if the damage is within its overlay rectangle
  if (!_content)
    return;

  for (const core::Rect &overlayRect : _overlayRects) {
    if (!r.intersect(overlayRect).is_empty())
      blendBuffer(_content->getContentPixelBuffer(), _content->getWidth(),
                 _content->getHeight(), overlayRect.tl, _overlayAlpha);
  }
}

const uint8_t *OverlayPixelBuffer::getBuffer(const core::Rect &r,
                                             int *stride_) const {
  int bytesPerPixel = format.bpp / 8;

  *stride_ = width();
  return overlayBuffer + (r.tl.y * width() + r.tl.x) * bytesPerPixel;
}