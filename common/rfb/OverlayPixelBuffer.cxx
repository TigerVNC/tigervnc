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

#include <string>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/LogWriter.h>
#include <core/Region.h>

#include <rfb/OverlayPixelBuffer.h>

#include <pixman.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fontconfig/fontconfig.h>

using namespace rfb;

static core::LogWriter vlog("OverlayPixelBuffer");

OverlayPixelBuffer::OverlayPixelBuffer(const PixelBuffer *parentBuf,
                                       const char *overlayPos,
                                       const char *overlayText)
    : ManagedPixelBuffer(parentBuf->getPF(), parentBuf->width(),
                         parentBuf->height()),
      parent(parentBuf),
      overlayBuffer(new uint8_t[width() * height() * (format.bpp / 8)]),
      _textBuffer(nullptr),
      _overlayRect(0, 0, 0, 0), _overlayPos(overlayPos),
      _overlayText(overlayText), _overlayFontSize(12), _overlayPadding(10), _overlayAlpha(0.5) {
  vlog.debug("Setting overlay position to: %s", overlayPos);
  renderOverlay();
  syncBuffers(getRect());
}

void OverlayPixelBuffer::updateOverlay(const char *overlayPos,
                                       const char *overlayText) {
  vlog.debug("Updating overlay to position %s, text %s", overlayPos,
             overlayText);

  _overlayPos = overlayPos;
  _overlayText = overlayText;

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
  delete[] _textBuffer;
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

void OverlayPixelBuffer::renderOverlay() {
  delete[] _textBuffer;
  _textBuffer = nullptr;
  _textWidth = _textHeight = 0;
  _overlayRect = core::Rect(0, 0, 0, 0);

  if (_overlayText.empty())
    return;

  _textBuffer = generateTextOverlayBuffer(_overlayText, _overlayFontSize,
                                 &_textWidth, &_textHeight);
  if (!_textBuffer)
    return;

  _textPos = calcOverlayPosition(_overlayPos.c_str(), _textWidth, _textHeight);
  _overlayRect = core::Rect(_textPos.x, _textPos.y, _textPos.x + _textWidth,
                            _textPos.y + _textHeight);
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

// Return a path for a usable font file for text overlay
static std::string findFontFile(const std::string &fontPattern = "") {
  if (!FcInit()) {
    return "";
  }

  FcPattern *pattern =
      FcNameParse(reinterpret_cast<const FcChar8 *>(fontPattern.c_str()));
  if (!pattern) {
    return "";
  }

  FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);

  FcResult result;
  FcPattern *font = FcFontMatch(nullptr, pattern, &result);

  std::string fontPath;
  if (font) {
    FcChar8 *file = nullptr;

    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      fontPath = reinterpret_cast<char *>(file);
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pattern);

  return fontPath;
}

// Initializes FreeType and loads the watermark font once.
static FT_Face getOverlayFont() {
  static FT_Library library = nullptr;
  static FT_Face face = nullptr;
  static bool initialized = false;

  if (initialized)
    return face;
  initialized = true;

  if (FT_Init_FreeType(&library) != 0) {
    vlog.error("Failed to initialize FreeType");
    return nullptr;
  }

  // TODO: Send in parameter for font
  const std::string fontFile = findFontFile("cursive");
  if (fontFile.empty()) {
    vlog.error("No usable font found for overlay text");
    return nullptr;
  }

  if (FT_New_Face(library, fontFile.c_str(), 0, &face) != 0) {
    vlog.error("Failed to load font %s for overlay text", fontFile.c_str());
    return nullptr;
  }

  return face;
}

// Renders text at the given pixel font size into a freshly allocated
// ARGB32 buffer sized exactly to fit the rendered glyphs. Returns nullptr
// if text is empty or no usable font is found.
uint8_t *OverlayPixelBuffer::generateTextOverlayBuffer(const std::string &text,
                                              int size,
                                              int *outWidth, int *outHeight) {
  *outWidth = *outHeight = 0;

  if (text.empty())
    return nullptr;

  FT_Face face = getOverlayFont();
  if (!face)
    return nullptr;

  FT_UInt pixelSize = static_cast<FT_UInt>(size);
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  // First pass: measure the extents so we can allocate a tightly fitting
  // buffer before rendering any glyphs.
  int textWidth = 0;
  for (size_t i = 0; i < text.size(); i++) {
    if (FT_Load_Char(face, static_cast<FT_ULong>(text[i]), FT_LOAD_DEFAULT) !=
        0)
      continue;
    textWidth += face->glyph->advance.x >> 6;
  }
  int ascent = face->size->metrics.ascender >> 6;
  int descent = -(face->size->metrics.descender >> 6);
  int textHeight = ascent + descent;

  if ((textWidth <= 0) || (textHeight <= 0))
    return nullptr;

  int stride = textWidth * 4;
  uint8_t *buf = new uint8_t[stride * textHeight]();

  pixman_image_t *destImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, textWidth, textHeight,
      reinterpret_cast<uint32_t *>(buf), stride);
  if (!destImage) {
    vlog.error("Failed to create Pixman image surface wrapper for text.");
    delete[] buf;
    return nullptr;
  }

  pixman_color_t whiteColor = {0xffff, 0xffff, 0xffff, 0xffff};
  pixman_image_t *textColor = pixman_image_create_solid_fill(&whiteColor);

  int penX = 0;
  int baselineY = ascent;

  for (size_t i = 0; i < text.size(); i++) {
    if (FT_Load_Char(face, static_cast<FT_ULong>(text[i]), FT_LOAD_RENDER) !=
        0)
      continue;

    FT_GlyphSlot glyph = face->glyph;
    FT_Bitmap &bitmap = glyph->bitmap;

    int glyphX = penX + glyph->bitmap_left;
    int glyphY = baselineY - glyph->bitmap_top;

    if ((bitmap.width > 0) && (bitmap.rows > 0)) {
      // Pixman requires the stride to be a multiple of 4 bytes, which
      // FreeType's tightly-packed 8-bit bitmaps rarely are.
      int alignedStride = (bitmap.width + 3) & ~3;
      std::vector<uint8_t> maskBuffer(alignedStride * bitmap.rows, 0);
      for (unsigned int row = 0; row < bitmap.rows; row++)
        memcpy(&maskBuffer[row * alignedStride],
               bitmap.buffer + row * bitmap.pitch, bitmap.width);

      pixman_image_t *mask = pixman_image_create_bits(
          PIXMAN_a8, bitmap.width, bitmap.rows,
          reinterpret_cast<uint32_t *>(maskBuffer.data()), alignedStride);

      if (mask) {
        pixman_image_composite(PIXMAN_OP_OVER, textColor, mask, destImage, 0,
                               0, 0, 0, glyphX, glyphY, bitmap.width,
                               bitmap.rows);
        pixman_image_unref(mask);
      }
    }

    penX += glyph->advance.x >> 6;
  }

  pixman_image_unref(textColor);
  pixman_image_unref(destImage);

  *outWidth = textWidth;
  *outHeight = textHeight;
  return buf;
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

  // Only redraw the overlay if the damage is within the overlay rectangle
  if (!r.intersect(_overlayRect).is_empty())
    blendBuffer(_textBuffer, _textWidth, _textHeight, _textPos,
               _overlayAlpha);
}

const uint8_t *OverlayPixelBuffer::getBuffer(const core::Rect &r,
                                             int *stride_) const {
  int bytesPerPixel = format.bpp / 8;

  *stride_ = width();
  return overlayBuffer + (r.tl.y * width() + r.tl.x) * bytesPerPixel;
}