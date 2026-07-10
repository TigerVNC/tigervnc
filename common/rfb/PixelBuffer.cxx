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

#include "core/Rect.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

#include <core/LogWriter.h>
#include <core/Region.h>
#include <core/string.h>

#include <rfb/PixelBuffer.h>

#include <pixman.h>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace rfb;

static core::LogWriter vlog("PixelBuffer");

// We do a lot of byte offset calculations that assume the result fits
// inside a signed 32 bit integer. Limit the maximum size of pixel
// buffers so that these calculations never overflow.

const int maxPixelBufferWidth = 16384;
const int maxPixelBufferHeight = 16384;
const int maxPixelBufferStride = 16384;


// -=- Generic pixel buffer class

PixelBuffer::PixelBuffer(const PixelFormat& pf, int w, int h)
  : format(pf), width_(0), height_(0)
{
  setSize(w, h);
}

PixelBuffer::PixelBuffer() : width_(0), height_(0)
{
}

PixelBuffer::~PixelBuffer() {}


void
PixelBuffer::getImage(void* imageBuf, const core::Rect& r,
                      int outStride) const
{
  int inStride;
  const uint8_t* data;
  int bytesPerPixel, inBytesPerRow, outBytesPerRow, bytesPerMemCpy;
  uint8_t* imageBufPos;
  const uint8_t* end;

  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  data = getBuffer(r, &inStride);

  bytesPerPixel = format.bpp/8;
  inBytesPerRow = inStride * bytesPerPixel;

  if (!outStride)
    outStride = r.width();
  outBytesPerRow = outStride * bytesPerPixel;
  bytesPerMemCpy = r.width() * bytesPerPixel;

  imageBufPos = (uint8_t*)imageBuf;
  end = data + (inBytesPerRow * r.height());

  while (data < end) {
    memcpy(imageBufPos, data, bytesPerMemCpy);
    imageBufPos += outBytesPerRow;
    data += inBytesPerRow;
  }
}

void PixelBuffer::getImage(const PixelFormat& pf, void* imageBuf,
                           const core::Rect& r, int stride) const
{
  const uint8_t* srcBuffer;
  int srcStride;

  if (format == pf) {
    getImage(imageBuf, r, stride);
    return;
  }

  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  if (stride == 0)
    stride = r.width();

  srcBuffer = getBuffer(r, &srcStride);

  pf.bufferFromBuffer((uint8_t*)imageBuf, format, srcBuffer,
                      r.width(), r.height(), stride, srcStride);
}

void PixelBuffer::setSize(int width, int height)
{
  if ((width < 0) || (width > maxPixelBufferWidth))
    throw std::out_of_range(core::format(
      "Invalid PixelBuffer width of %d pixels requested", width));
  if ((height < 0) || (height > maxPixelBufferHeight))
    throw std::out_of_range(core::format(
      "Invalid PixelBuffer height of %d pixels requested", height));

  width_ = width;
  height_ = height;
}

// -=- Modifiable generic pixel buffer class

ModifiablePixelBuffer::ModifiablePixelBuffer(const PixelFormat& pf,
                                             int w, int h)
  : PixelBuffer(pf, w, h)
{
}

ModifiablePixelBuffer::ModifiablePixelBuffer()
{
}

ModifiablePixelBuffer::~ModifiablePixelBuffer()
{
}

void ModifiablePixelBuffer::fillRect(const core::Rect& r,
                                     const void* pix)
{
  int stride;
  uint8_t *buf;
  int w, h, b;

  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  w = r.width();
  h = r.height();
  b = format.bpp/8;

  if (h == 0)
    return;

  buf = getBufferRW(r, &stride);

  if (b == 1) {
    while (h--) {
      memset(buf, *(const uint8_t*)pix, w);
      buf += stride * b;
    }
  } else {
    uint8_t *start;
    int w1;

    start = buf;

    w1 = w;
    while (w1--) {
      memcpy(buf, pix, b);
      buf += b;
    }
    buf += (stride - w) * b;
    h--;

    while (h--) {
      memcpy(buf, start, w * b);
      buf += stride * b;
    }
  }

  commitBufferRW(r);
}

void ModifiablePixelBuffer::imageRect(const core::Rect& r,
                                      const void* pixels, int srcStride)
{
  uint8_t* dest;
  int destStride;
  int bytesPerPixel, bytesPerDestRow, bytesPerSrcRow, bytesPerFill;
  const uint8_t* src;
  uint8_t* end;

  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  bytesPerPixel = getPF().bpp/8;

  dest = getBufferRW(r, &destStride);

  bytesPerDestRow = bytesPerPixel * destStride;

  if (!srcStride)
    srcStride = r.width();
  bytesPerSrcRow = bytesPerPixel * srcStride;
  bytesPerFill = bytesPerPixel * r.width();

  src = (const uint8_t*)pixels;
  end = dest + (bytesPerDestRow * r.height());

  while (dest < end) {
    memcpy(dest, src, bytesPerFill);
    dest += bytesPerDestRow;
    src += bytesPerSrcRow;
  }

  commitBufferRW(r);
}

void ModifiablePixelBuffer::copyRect(const core::Rect& rect,
                                     const core::Point& move_by_delta)
{
  int srcStride, dstStride;
  int bytesPerPixel;
  const uint8_t* srcData;
  uint8_t* dstData;

  core::Rect drect, srect;

  drect = rect;
  if (!drect.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      drect.width(), drect.height(), drect.tl.x, drect.tl.y,
      width(), height()));

  srect = drect.translate(move_by_delta.negate());
  if (!srect.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Source rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      srect.width(), srect.height(), srect.tl.x, srect.tl.y,
      width(), height()));

  bytesPerPixel = format.bpp/8;

  srcData = getBuffer(srect, &srcStride);
  dstData = getBufferRW(drect, &dstStride);

  if (move_by_delta.y == 0) {
    // Possible overlap. Be careful and use memmove().
    int h = drect.height();
    while (h--) {
      memmove(dstData, srcData, drect.width() * bytesPerPixel);
      dstData += dstStride * bytesPerPixel;
      srcData += srcStride * bytesPerPixel;
    }
  } else if (move_by_delta.y < 0) {
    // The data shifted upwards. Copy from top to bottom.
    int h = drect.height();
    while (h--) {
      memcpy(dstData, srcData, drect.width() * bytesPerPixel);
      dstData += dstStride * bytesPerPixel;
      srcData += srcStride * bytesPerPixel;
    }
  } else {
    // The data shifted downwards. Copy from bottom to top.
    int h = drect.height();
    dstData += (h-1) * dstStride * bytesPerPixel;
    srcData += (h-1) * srcStride * bytesPerPixel;
    while (h--) {
      memcpy(dstData, srcData, drect.width() * bytesPerPixel);
      dstData -= dstStride * bytesPerPixel;
      srcData -= srcStride * bytesPerPixel;
    }
  }

  commitBufferRW(drect);
}

void ModifiablePixelBuffer::fillRect(const PixelFormat& pf,
                                     const core::Rect& dest,
                                     const void* pix)
{
  uint8_t buf[4];
  format.bufferFromBuffer(buf, pf, (const uint8_t*)pix, 1);
  fillRect(dest, buf);
}

void ModifiablePixelBuffer::imageRect(const PixelFormat& pf,
                                      const core::Rect& dest,
                                      const void* pixels, int stride)
{
  uint8_t* dstBuffer;
  int dstStride;

  if (!dest.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d",
      dest.width(), dest.height(), dest.tl.x, dest.tl.y,
      width(), height()));

  if (stride == 0)
    stride = dest.width();

  dstBuffer = getBufferRW(dest, &dstStride);
  format.bufferFromBuffer(dstBuffer, pf, (const uint8_t*)pixels,
                          dest.width(), dest.height(),
                          dstStride, stride);
  commitBufferRW(dest);
}

// -=- Simple pixel buffer with a continuous block of memory

FullFramePixelBuffer::FullFramePixelBuffer(const PixelFormat& pf, int w, int h,
                                           uint8_t* data_, int stride_)
  : ModifiablePixelBuffer(pf, w, h), data(data_), stride(stride_)
{
}

FullFramePixelBuffer::FullFramePixelBuffer() : data(nullptr) {}

FullFramePixelBuffer::~FullFramePixelBuffer() {}

uint8_t* FullFramePixelBuffer::getBufferRW(const core::Rect& r,
                                           int* stride_)
{
  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Pixel buffer request %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  *stride_ = stride;
  return &data[(r.tl.x + (r.tl.y * stride)) * (format.bpp/8)];
}

void FullFramePixelBuffer::commitBufferRW(const core::Rect& /*r*/)
{
}

const uint8_t* FullFramePixelBuffer::getBuffer(const core::Rect& r,
                                               int* stride_) const
{
  if (!r.enclosed_by(getRect()))
    throw std::out_of_range(core::format(
      "Pixel buffer request %dx%d at %d,%d exceeds framebuffer %dx%d",
      r.width(), r.height(), r.tl.x, r.tl.y, width(), height()));

  *stride_ = stride;
  return &data[(r.tl.x + (r.tl.y * stride)) * (format.bpp/8)];
}

void FullFramePixelBuffer::setBuffer(int width, int height,
                                     uint8_t* data_, int stride_)
{
  if ((width < 0) || (width > maxPixelBufferWidth))
    throw std::out_of_range(core::format(
      "Invalid PixelBuffer width of %d pixels requested", width));
  if ((height < 0) || (height > maxPixelBufferHeight))
    throw std::out_of_range(core::format(
      "Invalid PixelBuffer height of %d pixels requested", height));
  if ((stride_ < 0) || (stride_ > maxPixelBufferStride) || (stride_ < width))
    throw std::invalid_argument(core::format(
      "Invalid PixelBuffer stride of %d pixels requested", stride_));
  if ((width != 0) && (height != 0) && (data_ == nullptr))
    throw std::logic_error(core::format(
      "PixelBuffer requested without a valid memory area"));

  ModifiablePixelBuffer::setSize(width, height);
  stride = stride_;
  data = data_;
}

void FullFramePixelBuffer::setSize(int /*w*/, int /*h*/)
{
  // setBuffer() should be used
  throw std::logic_error("Invalid call to FullFramePixelBuffer::setSize()");
}

// -=- Managed pixel buffer class
// Automatically allocates enough space for the specified format & area

ManagedPixelBuffer::ManagedPixelBuffer()
  : data_(nullptr), datasize(0)
{
}

ManagedPixelBuffer::ManagedPixelBuffer(const PixelFormat& pf, int w, int h)
  : FullFramePixelBuffer(pf, 0, 0, nullptr, 0), data_(nullptr), datasize(0)
{
  setSize(w, h);
}

ManagedPixelBuffer::~ManagedPixelBuffer()
{
  if (data_)
    delete [] data_;
}

void ManagedPixelBuffer::setPF(const PixelFormat &pf)
{
  format = pf;
  setSize(width(), height());
}

void ManagedPixelBuffer::setSize(int w, int h)
{
  unsigned long new_datasize = w * h * (format.bpp/8);

  new_datasize = w * h * (format.bpp/8);
  if (datasize < new_datasize) {
    if (data_) {
      delete [] data_;
      data_ = nullptr;
      datasize = 0;
    }
    if (new_datasize) {
      data_ = new uint8_t[new_datasize];
      datasize = new_datasize;
    }
  }

  setBuffer(w, h, data_, w);
}

OverlayPixelBuffer::OverlayPixelBuffer(const PixelBuffer* parentBuf, const char* overlayPos,
                                       const char* overlayText)
  : ManagedPixelBuffer(parentBuf->getPF(), parentBuf->width(), parentBuf->height()),
    parent(parentBuf),
    overlayBuffer(new uint8_t[width() * height() * (format.bpp/8)]),
    _overlayRect(0,0,0,0),
    _overlayPos(overlayPos),
    _overlayText(overlayText),
    _overlayFontSize(12),
    _overlayPadding(10)
{
  vlog.debug("Setting overlay position to: %s", overlayPos);
  setOverlayRect(overlayPos);
  syncBuffers(getRect());
}

OverlayPixelBuffer::OverlayPixelBuffer(const PixelBuffer* parentBuf)
  : ManagedPixelBuffer(parentBuf->getPF(), parentBuf->width(), parentBuf->height()),
    parent(parentBuf),
    overlayBuffer(new uint8_t[width() * height() * (format.bpp/8)])
{
  syncBuffers(getRect());
}

void OverlayPixelBuffer::updateOverlay(const char* overlayPos, const char* overlayText)
{
  vlog.debug("Updating overlay to position %s, text %s", overlayPos, overlayText);

  _overlayPos = overlayPos;
  _overlayText = overlayText;

  setOverlayRect(overlayPos);
  syncBuffers(getRect());
}

void OverlayPixelBuffer::setParent(const PixelBuffer* parentBuf)
{
  parent = parentBuf;

  // If the parent buffer has changed size, resize the overlay buffer to match
  if ((width() != parent->width()) || (height() != parent->height()))
    // syncBuffers() is called later in setSize()
    setSize(parent->width(), parent->height());
  else
    syncBuffers(getRect());
}

void OverlayPixelBuffer::setSize(int w, int h)
{
  ManagedPixelBuffer::setSize(w, h);

  delete [] overlayBuffer;
  overlayBuffer = new uint8_t[width() * height() * (format.bpp/8)];
  
  //Computes a new overlay rectangle position based on the new size
  setOverlayRect(_overlayPos.c_str());
  vlog.debug("Overlay position after resize: %s", _overlayPos.c_str());
  syncBuffers(getRect());
}

OverlayPixelBuffer::~OverlayPixelBuffer()
{
  delete[] overlayBuffer;
}

void OverlayPixelBuffer::setOverlayRect(const char* overlayPos)
{
  core::Point rectSize = getTextSize();
  int boxWidth  = rectSize.x+_overlayPadding;
  int boxHeight = rectSize.y+_overlayPadding;
  

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
      _overlayRect = core::Rect(width() - boxWidth, height() - boxHeight, width(), height());
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

void OverlayPixelBuffer::placeOverlay(const core::Rect& rect) const
{
  vlog.debug("Placing overlay at %d,%d with size %dx%d using Pixman", rect.tl.x, rect.tl.y, rect.width(), rect.height());

  // 1. Map the buffer's bits-per-pixel (bpp) to a corresponding Pixman format
  pixman_format_code_t pixmanFormat;
  switch (format.bpp)
  {
    case 32: pixmanFormat = PIXMAN_a8r8g8b8; break; // Use your system's exact format (e.g., PIXMAN_a8b8g8r8) if colors appear swapped
    case 24: pixmanFormat = PIXMAN_r8g8b8;   break;
    case 16: pixmanFormat = PIXMAN_r5g6b5;   break;
    case 8:  pixmanFormat = PIXMAN_a8;       break;
    default:
      vlog.error("Unsupported bits-per-pixel (%d) for Pixman wrapper", format.bpp);
      return;
  }

  // 2. Define the fill color (Pixman uses 16-bit channels: 0x0000 to 0xffff)
  pixman_color_t blueColor;
  blueColor.red   = 0x0000;
  blueColor.green = 0x0000;
  blueColor.blue  = 0xffff; // Full intensity Blue
  blueColor.alpha = 0x7fff; // Fully opaque

  // 3. Define the destination rectangle geometry
  pixman_rectangle16_t pixmanRect;
  pixmanRect.x      = static_cast<int16_t>(rect.tl.x);
  pixmanRect.y      = static_cast<int16_t>(rect.tl.y);
  pixmanRect.width  = static_cast<uint16_t>(rect.width());
  pixmanRect.height = static_cast<uint16_t>(rect.height());

  // 4. Calculate row stride in bytes
  int bytesPerPixel = format.bpp / 8;
  int rowStrideBytes = width() * bytesPerPixel;

  // 5. Wrap the raw overlayBuffer inside a pixman image view
  // Note: const_cast is used because placeOverlay is marked const, but we are writing data to the target buffer
  pixman_image_t* destImage = pixman_image_create_bits(
    pixmanFormat,
    width(),
    height(),
    reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(overlayBuffer)),
    rowStrideBytes
  );

  if (!destImage)
  {
    vlog.error("Failed to create Pixman image surface wrapper.");
    return;
  }

  // 6. Perform the fill operation (PIXMAN_OP_SRC overwrites the target area completely)
  // Pixman automatically clips the coordinates if they exceed the image bounds.
  pixman_image_fill_rectangles(PIXMAN_OP_OVER, destImage, &blueColor, 1, &pixmanRect);

  // 7. Draw the watermark text on top of the box, if any was configured
  if (!_overlayText.empty())
    renderText(rect, destImage);

  // 8. Clean up the Pixman wrapper (this does not free your underlying overlayBuffer)
  pixman_image_unref(destImage);
}

// Return a path for a usable font file for text overlay
static const char* findFontFile()
{
  static const char* candidates[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
    "/usr/share/fonts/liberation-sans/LiberationSans-Bold.ttf",
    "/usr/share/fonts/liberation/LiberationSans-Bold.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
  };

  for (const char* path : candidates) {
    FILE* f = fopen(path, "rb");
    if (f) {
      fclose(f);
      vlog.debug("Found usable font for overlay text: %s", path);
      return path;
    }
  }
  vlog.debug("No usable font could be found");
  return nullptr;
}

// Initializes FreeType and loads the watermark font once.
static FT_Face getOverlayFont()
{
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

  const char* fontFile = findFontFile();
  if (!fontFile) {
    vlog.error("No usable font found for overlay text");
    return nullptr;
  }

  if (FT_New_Face(library, fontFile, 0, &face) != 0) {
    vlog.error("Failed to load font %s for overlay text", fontFile);
    return nullptr;
  }

  return face;
}

// Renders the overlay text into the specified rectangle of the destination image.
void OverlayPixelBuffer::renderText(const core::Rect& rect, void* destImagePtr) const
{
  pixman_image_t* destImage = static_cast<pixman_image_t*>(destImagePtr);

  FT_Face face = getOverlayFont();
  if (!face)
    return;

  
  FT_UInt pixelSize = static_cast<FT_UInt>(_overlayFontSize);
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  pixman_color_t whiteColor = {0xffff, 0xffff, 0xffff, 0xffff};
  pixman_image_t* textColor = pixman_image_create_solid_fill(&whiteColor);

  int penX = rect.tl.x + _overlayPadding/2;
  int baselineY = rect.br.y - _overlayPadding/2;

  for (size_t i = 0; i < _overlayText.size(); i++) {
    if (FT_Load_Char(face, static_cast<FT_ULong>(_overlayText[i]), FT_LOAD_RENDER) != 0)
      continue;

    FT_GlyphSlot glyph = face->glyph;
    FT_Bitmap& bitmap = glyph->bitmap;

    int glyphX = penX + glyph->bitmap_left;
    int glyphY = baselineY - glyph->bitmap_top;

    // Only render the glyph if it fits within the overlay rectangle
    if ((bitmap.width > 0) && (bitmap.rows > 0) && (glyphX + (int)bitmap.width <= rect.br.x)) {
      // Pixman requires the stride to be a multiple of 4 bytes, which
      // FreeType's tightly-packed 8-bit bitmaps rarely are.
      int alignedStride = (bitmap.width + 3) & ~3;
      std::vector<uint8_t> maskBuffer(alignedStride * bitmap.rows, 0);
      for (unsigned int row = 0; row < bitmap.rows; row++)
        memcpy(&maskBuffer[row * alignedStride],
               bitmap.buffer + row * bitmap.pitch, bitmap.width);

      pixman_image_t* mask = pixman_image_create_bits(
        PIXMAN_a8, bitmap.width, bitmap.rows,
        reinterpret_cast<uint32_t*>(maskBuffer.data()), alignedStride);

      if (mask) {
        pixman_image_composite(PIXMAN_OP_OVER, textColor, mask, destImage,
                                0, 0, 0, 0, glyphX, glyphY,
                                bitmap.width, bitmap.rows);
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
core::Point OverlayPixelBuffer::getTextSize() const
{
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
    if (FT_Load_Char(face, static_cast<FT_ULong>(_overlayText[i]), FT_LOAD_DEFAULT) != 0)
      continue;
    width += face->glyph->advance.x >> 6;
  }

  return {width, static_cast<int>(pixelSize)};
}

void OverlayPixelBuffer::syncBuffers(const core::Region& r)
{
  std::vector<core::Rect> rects;
  int bytesPerPixel = format.bpp / 8;

  r.get_rects(&rects);

  for (const core::Rect& rect : rects) {
    core::Rect clipped = rect.intersect(parent->getRect());
    if (clipped.is_empty())
      continue;

    int parentStride;
    const uint8_t* src = parent->getBuffer(clipped, &parentStride);
    uint8_t* dst = overlayBuffer +
      (clipped.tl.y * width() + clipped.tl.x) * bytesPerPixel;

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

const uint8_t* OverlayPixelBuffer::getBuffer(const core::Rect& r,
                                           int* stride_) const
{
  int bytesPerPixel = format.bpp / 8;

  *stride_ = width();
  return overlayBuffer + (r.tl.y * width() + r.tl.x) * bytesPerPixel;
}