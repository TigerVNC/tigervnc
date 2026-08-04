// -=- OverlayContentPng.cxx
//
// Loads a PNG image into a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstring>

#include <core/LogWriter.h>

#include <rfb/OverlayContentPng.h>
#include <rfb/OverlayContentText.h>

#include <pixman.h>
#include <png.h>

using namespace rfb;

static core::LogWriter vlog("OverlayContentPng");

int ERROR_MSG_SIZE =
    20; // Size of the error message buffer for PNG load failures
std::string ERROR_MSG =
    "PNG file could not be found/opened"; // Error message for PNG load failures

// Scales a tightly packed ARGB32 buffer with a given height, keeps the original
// aspect ratio
static uint8_t *scaleArgbBuffer(const uint8_t *srcBuffer, int srcWidth,
                                int srcHeight, int dstWidth, int dstHeight) {
  uint8_t *dstBuffer = new uint8_t[dstWidth * dstHeight * 4];

  pixman_image_t *srcImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, srcWidth, srcHeight,
      reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(srcBuffer)),
      srcWidth * 4);
  pixman_image_t *dstImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, dstWidth, dstHeight,
      reinterpret_cast<uint32_t *>(dstBuffer), dstWidth * 4);

  pixman_transform_t transform;
  pixman_transform_init_scale(
      &transform,
      pixman_double_to_fixed(static_cast<double>(srcWidth) / dstWidth),
      pixman_double_to_fixed(static_cast<double>(srcHeight) / dstHeight));
  pixman_image_set_transform(srcImage, &transform);
  pixman_image_set_filter(srcImage, PIXMAN_FILTER_BILINEAR, nullptr, 0);

  pixman_image_composite(PIXMAN_OP_SRC, srcImage, nullptr, dstImage, 0, 0, 0, 0,
                         0, 0, static_cast<uint16_t>(dstWidth),
                         static_cast<uint16_t>(dstHeight));

  pixman_image_unref(srcImage);
  pixman_image_unref(dstImage);

  return dstBuffer;
}

OverlayContentPng::OverlayContentPng(const std::string &filePath, int height)
    : _buffer(nullptr) {
  _width = _height = 0;
  _buffer = loadPngBuffer(filePath, height, &_width, &_height);
}

OverlayContentPng::~OverlayContentPng() { delete[] _buffer; }

// Decodes the PNG file at filePath into a new pixelbuffer, scaled to the
// given height while preserving the image's own aspect ratio.
uint8_t *OverlayContentPng::loadPngBuffer(const std::string &filePath,
                                          int height, int *outWidth,
                                          int *outHeight) {
  *outWidth = *outHeight = 0;

  if (filePath.empty())
    return nullptr;

  png_image image;
  memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;

  if (!png_image_begin_read_from_file(&image, filePath.c_str())) {
    vlog.error("Failed to read PNG file %s: %s", filePath.c_str(),
               image.message);
    return OverlayContentText::generateTextBuffer(ERROR_MSG, ERROR_MSG_SIZE,
                                                  outWidth, outHeight);
  }

  // Pixman format
  image.format = PNG_FORMAT_BGRA;

  uint8_t *buffer = new uint8_t[PNG_IMAGE_SIZE(image)];

  if (!png_image_finish_read(&image, nullptr, buffer, 0, nullptr)) {
    vlog.error("Failed to decode PNG file %s: %s", filePath.c_str(),
               image.message);
    delete[] buffer;
    png_image_free(&image);
    return OverlayContentText::generateTextBuffer(ERROR_MSG, ERROR_MSG_SIZE,
                                                  outWidth, outHeight);
  }

  size_t pixelCount = static_cast<size_t>(image.width) * image.height;
  for (size_t i = 0; i < pixelCount; i++) {
    uint8_t *p = buffer + i * 4;
    uint8_t alpha = p[3];
    p[0] = static_cast<uint8_t>((p[0] * alpha) / 255);
    p[1] = static_cast<uint8_t>((p[1] * alpha) / 255);
    p[2] = static_cast<uint8_t>((p[2] * alpha) / 255);
  }

  int width = static_cast<int>(image.width);
  int origHeight = static_cast<int>(image.height);

  png_image_free(&image);

  if (height > 0 && height != origHeight) {
    int scaledWidth =
        std::max(1, static_cast<int>(std::lround(static_cast<double>(width) *
                                                 height / origHeight)));
    uint8_t *scaled =
        scaleArgbBuffer(buffer, width, origHeight, scaledWidth, height);
    delete[] buffer;
    buffer = scaled;
    width = scaledWidth;
    origHeight = height;
  }

  *outWidth = width;
  *outHeight = origHeight;

  return buffer;
}
