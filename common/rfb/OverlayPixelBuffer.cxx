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
      _overlayRect(0, 0, 0, 0), _overlayPos(overlayPos),
      _overlayText(overlayText), _overlayFontSize(12), _overlayPadding(10) {
  vlog.debug("Setting overlay position to: %s", overlayPos);
  setOverlayRect(overlayPos);
  syncBuffers(getRect());
}

OverlayPixelBuffer::OverlayPixelBuffer(const PixelBuffer *parentBuf)
    : ManagedPixelBuffer(parentBuf->getPF(), parentBuf->width(),
                         parentBuf->height()),
      parent(parentBuf),
      overlayBuffer(new uint8_t[width() * height() * (format.bpp / 8)]) {
  syncBuffers(getRect());
}

void OverlayPixelBuffer::updateOverlay(const char *overlayPos,
                                       const char *overlayText) {
  vlog.debug("Updating overlay to position %s, text %s", overlayPos,
             overlayText);

  _overlayPos = overlayPos;
  _overlayText = overlayText;

  setOverlayRect(overlayPos);
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
  setOverlayRect(_overlayPos.c_str());
  vlog.debug("Overlay position after resize: %s", _overlayPos.c_str());
  syncBuffers(getRect());
}

OverlayPixelBuffer::~OverlayPixelBuffer() { delete[] overlayBuffer; }

void OverlayPixelBuffer::setOverlayRect(const char *overlayPos) {
  core::Point rectSize = getTextSize();
  int boxWidth = rectSize.x + _overlayPadding;
  int boxHeight = rectSize.y + _overlayPadding;

  if (strcmp(overlayPos, "tl") == 0) {
    // Top-Left corner
    _overlayRect = core::Rect(0, 0, boxWidth, boxHeight);
  } else if (strcmp(overlayPos, "tr") == 0) {
    // Top-Right corner
    _overlayRect = core::Rect(width() - boxWidth, 0, width(), boxHeight);
  } else if (strcmp(overlayPos, "bl") == 0) {
    // Bottom-Left corner
    _overlayRect = core::Rect(0, height() - boxHeight, boxWidth, height());
  } else if (strcmp(overlayPos, "br") == 0) {
    // Bottom-Right corner
    _overlayRect =
        core::Rect(width() - boxWidth, height() - boxHeight, width(), height());
  } else if (strcmp(overlayPos, "c") == 0) {
    // Centered box
    int x1 = (width() / 2) - (boxWidth / 2);
    int y1 = (height() / 2) - (boxHeight / 2);
    int x2 = (width() / 2) + (boxWidth / 2);
    int y2 = (height() / 2) + (boxHeight / 2);
    _overlayRect = core::Rect(x1, y1, x2, y2);
  } else {
    vlog.error("Invalid overlay position specified: %s", overlayPos);
    _overlayRect = core::Rect(0, 0, 0, 0); // Default to no overlay
  }
}

void OverlayPixelBuffer::placeOverlay(const core::Rect &rect) const {
  vlog.debug("Placing overlay at %d,%d with size %dx%d using Pixman", rect.tl.x,
             rect.tl.y, rect.width(), rect.height());

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

  // 2. Define the fill color (Pixman uses 16-bit channels: 0x0000 to 0xffff)
  pixman_color_t backgroundColor;
  backgroundColor.red = 0x0000;
  backgroundColor.green = 0x0000;
  backgroundColor.blue = 0x0000;
  backgroundColor.alpha = 0x7fff; // 50% transparent

  // 3. Define the destination rectangle geometry
  pixman_rectangle16_t pixmanRect;
  pixmanRect.x = static_cast<int16_t>(rect.tl.x);
  pixmanRect.y = static_cast<int16_t>(rect.tl.y);
  pixmanRect.width = static_cast<uint16_t>(rect.width());
  pixmanRect.height = static_cast<uint16_t>(rect.height());

  // 4. Calculate row stride in bytes
  int bytesPerPixel = format.bpp / 8;
  int rowStrideBytes = width() * bytesPerPixel;

  // 5. Wrap the raw overlayBuffer inside a pixman image view
  // Note: const_cast is used because placeOverlay is marked const, but we are
  // writing data to the target buffer
  pixman_image_t *destImage = pixman_image_create_bits(
      pixmanFormat, width(), height(),
      reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(overlayBuffer)),
      rowStrideBytes);

  if (!destImage) {
    vlog.error("Failed to create Pixman image surface wrapper.");
    return;
  }

  // 6. Perform the fill operation (PIXMAN_OP_SRC overwrites the target area
  // completely) Pixman automatically clips the coordinates if they exceed the
  // image bounds.
  pixman_image_fill_rectangles(PIXMAN_OP_OVER, destImage, &backgroundColor, 1,
                               &pixmanRect);

  // 7. Draw the watermark text on top of the box, if any was configured
  if (!_overlayText.empty())
    renderText(rect, destImage);

  // 8. Clean up the Pixman wrapper (this does not free your underlying
  // overlayBuffer)
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
  const std::string fontFile = findFontFile();
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

// Renders the overlay text into the specified rectangle of the destination
// image.
void OverlayPixelBuffer::renderText(const core::Rect &rect,
                                    void *destImagePtr) const {
  pixman_image_t *destImage = static_cast<pixman_image_t *>(destImagePtr);

  FT_Face face = getOverlayFont();
  if (!face)
    return;

  FT_UInt pixelSize = static_cast<FT_UInt>(_overlayFontSize);
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  pixman_color_t whiteColor = {0xffff, 0xffff, 0xffff, 0xffff};
  pixman_image_t *textColor = pixman_image_create_solid_fill(&whiteColor);

  int penX = rect.tl.x + _overlayPadding / 2;
  int baselineY = rect.br.y - _overlayPadding / 2;

  for (size_t i = 0; i < _overlayText.size(); i++) {
    if (FT_Load_Char(face, static_cast<FT_ULong>(_overlayText[i]),
                     FT_LOAD_RENDER) != 0)
      continue;

    FT_GlyphSlot glyph = face->glyph;
    FT_Bitmap &bitmap = glyph->bitmap;

    int glyphX = penX + glyph->bitmap_left;
    int glyphY = baselineY - glyph->bitmap_top;

    // Only render the glyph if it fits within the overlay rectangle
    if ((bitmap.width > 0) && (bitmap.rows > 0) &&
        (glyphX + (int)bitmap.width <= rect.br.x)) {
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
        pixman_image_composite(PIXMAN_OP_OVER, textColor, mask, destImage, 0, 0,
                               0, 0, glyphX, glyphY, bitmap.width, bitmap.rows);
        pixman_image_unref(mask);
      }
    }

    penX += glyph->advance.x >> 6;
    if (penX >= rect.br.x)
      break;
  }

  pixman_image_unref(textColor);
}

// Measures how large _overlayText will be once rendered at the current
// font size, without rasterizing any glyphs.
core::Point OverlayPixelBuffer::getTextSize() const {
  FT_Face face;
  FT_UInt pixelSize;
  int width;

  if (_overlayText.empty())
    return {0, 0};

  face = getOverlayFont();
  if (!face)
    return {0, 0};

  pixelSize = static_cast<FT_UInt>(_overlayFontSize);
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  width = 0;
  for (size_t i = 0; i < _overlayText.size(); i++) {
    if (FT_Load_Char(face, static_cast<FT_ULong>(_overlayText[i]),
                     FT_LOAD_DEFAULT) != 0)
      continue;
    width += face->glyph->advance.x >> 6;
  }

  return {width, static_cast<int>(pixelSize)};
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
    placeOverlay(_overlayRect);
}

const uint8_t *OverlayPixelBuffer::getBuffer(const core::Rect &r,
                                             int *stride_) const {
  int bytesPerPixel = format.bpp / 8;

  *stride_ = width();
  return overlayBuffer + (r.tl.y * width() + r.tl.x) * bytesPerPixel;
}