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

#include <assert.h>

#include <pixman.h>
#include <stdexcept>
#include <vector>

#include <core/LogWriter.h>
#include <core/Region.h>
#include <core/i18n.h>

#include "FadingPixelBuffer.h"

using namespace rfb;

static const PixelFormat pfRGBX(32, 24, false, true, 255, 255, 255, 0, 8, 16);
static const PixelFormat pfBGRX(32, 24, false, true, 255, 255, 255, 16, 8, 0);
static const PixelFormat pfXRGB(32, 24, false, true, 255, 255, 255, 8, 16, 24);
static const PixelFormat pfXBGR(32, 24, false, true, 255, 255, 255, 24, 16, 8);

static core::LogWriter vlog("FadingPixelBuffer");

FadingPixelBuffer::FadingPixelBuffer(const PixelBuffer* pb)
  : PixelBuffer(pb->getPF(), pb->width(), pb->height()),
    parent(pb), fadeLevel(1.0f),
    fadedBuffer(new uint8_t[width() * height() * (format.bpp/8)])
{
}

FadingPixelBuffer::~FadingPixelBuffer()
{
  delete[] fadedBuffer;
}

void FadingPixelBuffer::setFadeLevel(float level)
{
  fadeLevel = level < 0.0f ? 0.0f : (level > 1.0f ? 1.0f : level);
}

const uint8_t* FadingPixelBuffer::getBuffer(const core::Rect& r,
                                            int* stride_) const
{
  *stride_ = width();
  return fadedBuffer + (r.tl.y * width() + r.tl.x) * format.bpp / 8;
}

void FadingPixelBuffer::syncBuffers(const core::Region& r)
{
  const uint8_t* buffer;
  int stride_;
  int bpp;
  pixman_format_code_t pixmanFormat;
  pixman_image_t* srcImg;
  pixman_image_t* dstImg;
  uint16_t alpha;
  pixman_color_t black;
  pixman_image_t* overlay;
  std::vector<core::Rect> rects;

  assert(fadeLevel <= 1.0f && fadeLevel >= 0.0f);

  if (!r.get_rects(&rects))
    return;

  if (parent->getRect().width() != width() || parent->getRect().height() != height())
    throw std::runtime_error(_("Parent PixelBuffer size mismatch"));

  if (format != parent->getPF())
    throw std::runtime_error(_("Parent PixelBuffer format mismatch"));

  try {
    pixmanFormat = rfbToPixmanFormat(format);
  } catch (std::runtime_error&) {
    syncBuffersFallback(r);
    return;
  }

  buffer = parent->getBuffer(parent->getRect(), &stride_);
  bpp = format.bpp / 8;

  alpha = (1 - fadeLevel) * 0xffff;
  black = {0, 0, 0, alpha};
  overlay = pixman_image_create_solid_fill(&black);

  srcImg = pixman_image_create_bits(pixmanFormat, width(), height(),
                                    (uint32_t *)buffer, stride_ * bpp);
  dstImg = pixman_image_create_bits(pixmanFormat, width(), height(),
                                    (uint32_t *)fadedBuffer, width() * bpp);
  for (core::Rect &rect : rects) {
    pixman_image_composite32(PIXMAN_OP_SRC, srcImg, nullptr, dstImg,
                             rect.tl.x, rect.tl.y, 0, 0, rect.tl.x,
                             rect.tl.y, rect.width(), rect.height());

    pixman_image_composite32(PIXMAN_OP_OVER, overlay, nullptr, dstImg,
                             rect.tl.x, rect.tl.y, 0, 0, rect.tl.x,
                             rect.tl.y, rect.width(), rect.height());
  }

  pixman_image_unref(overlay);
  pixman_image_unref(srcImg);
  pixman_image_unref(dstImg);
}

void FadingPixelBuffer::syncBuffersFallback(const core::Region& r)
{
  const uint8_t* buffer;
  int stride_;
  int bpp;
  std::vector<core::Rect> rects;

  assert(fadeLevel <= 1.0f && fadeLevel >= 0.0f);

  if (!r.get_rects(&rects))
    return;

  if (parent->getRect().width() != width() || parent->getRect().height() != height())
    throw std::runtime_error(_("Parent PixelBuffer size mismatch"));

  if (format != parent->getPF())
    throw std::runtime_error(_("Parent PixelBuffer format mismatch"));

  buffer = parent->getBuffer(parent->getRect(), &stride_);
  bpp = format.bpp / 8;

  for (core::Rect &rect : rects) {
    std::vector<uint8_t> rgb(rect.width() * 3);

    for (int y = 0; y < rect.height(); y++) {
      const uint8_t* srcRow;
      uint8_t* dstRow;

      srcRow = buffer + ((rect.tl.y + y) * stride_ + rect.tl.x) * bpp;
      dstRow = fadedBuffer + ((rect.tl.y + y) * width() + rect.tl.x) * bpp;

      format.rgbFromBuffer(rgb.data(), srcRow, rect.width());

      for (int x = 0; x < rect.width(); x++) {
        rgb[x*3 + 0] = (uint8_t)(rgb[x*3 + 0] * fadeLevel);
        rgb[x*3 + 1] = (uint8_t)(rgb[x*3 + 1] * fadeLevel);
        rgb[x*3 + 2] = (uint8_t)(rgb[x*3 + 2] * fadeLevel);
      }

      format.bufferFromRGB(dstRow, rgb.data(), rect.width());
    }
  }
}

pixman_format_code_t FadingPixelBuffer::rfbToPixmanFormat(PixelFormat pf)
{
  if (pf == pfBGRX)
    return PIXMAN_x8r8g8b8;
  if (pf == pfRGBX)
    return PIXMAN_x8b8g8r8;
  if (pf == pfXBGR)
    return PIXMAN_r8g8b8x8;
  if (pf == pfXRGB)
    return PIXMAN_b8g8r8x8;

  // FIXME: Support more pixman formats
  throw std::runtime_error(_("Unsupported pixel format"));
}
