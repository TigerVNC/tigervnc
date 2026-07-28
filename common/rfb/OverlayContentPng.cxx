// -=- OverlayContentPng.cxx
//
// Loads a PNG image into a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include <core/LogWriter.h>

#include <rfb/OverlayContentPng.h>

#include <png.h>

using namespace rfb;

static core::LogWriter vlog("OverlayContentPng");

OverlayContentPng::OverlayContentPng(const std::string &filePath)
    : _buffer(nullptr) {
  _width = _height = 0;
  _buffer = loadPngBuffer(filePath, &_width, &_height);
}

OverlayContentPng::~OverlayContentPng() { delete[] _buffer; }

// Decodes the PNG file at filePath into a new pixelbuffer
uint8_t *OverlayContentPng::loadPngBuffer(const std::string &filePath,
                                          int *outWidth, int *outHeight) {
  *outWidth = *outHeight = 0;

  if (filePath.empty())
    return nullptr;

  png_image image;
  memset(&image, 0, sizeof(image));
  image.version = PNG_IMAGE_VERSION;

  if (!png_image_begin_read_from_file(&image, filePath.c_str())) {
    vlog.error("Failed to read PNG file %s: %s", filePath.c_str(),
               image.message);
    return nullptr;
  }

  //Pixman format
  image.format = PNG_FORMAT_BGRA;

  uint8_t *buffer = new uint8_t[PNG_IMAGE_SIZE(image)];

  if (!png_image_finish_read(&image, nullptr, buffer, 0, nullptr)) {
    vlog.error("Failed to decode PNG file %s: %s", filePath.c_str(),
               image.message);
    delete[] buffer;
    png_image_free(&image);
    return nullptr;
  }

  size_t pixelCount = static_cast<size_t>(image.width) * image.height;
  for (size_t i = 0; i < pixelCount; i++) {
    uint8_t *p = buffer + i * 4;
    uint8_t alpha = p[3];
    p[0] = static_cast<uint8_t>((p[0] * alpha) / 255);
    p[1] = static_cast<uint8_t>((p[1] * alpha) / 255);
    p[2] = static_cast<uint8_t>((p[2] * alpha) / 255);
  }

  *outWidth = static_cast<int>(image.width);
  *outHeight = static_cast<int>(image.height);

  png_image_free(&image);
  return buffer;
}
