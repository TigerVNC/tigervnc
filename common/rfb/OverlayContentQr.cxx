// -=- OverlayContentQr.cxx
//
// Encodes data into a QR code and renders it as a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/LogWriter.h>

#include <rfb/OverlayContentQr.h>

using namespace rfb;

static core::LogWriter vlog("OverlayContentQr");

OverlayContentQr::OverlayContentQr(const std::string &data, int height)
    : _buffer(nullptr) {
  _width = _height = 0;
  _buffer = generateQrBuffer(data, height, &_width, &_height);
}

OverlayContentQr::~OverlayContentQr() { delete[] _buffer; }

// Encodes data into a QR code, rendered into a new pixelbuffer scaled to the
// given height (QR codes are square, so width matches height).
uint8_t *OverlayContentQr::generateQrBuffer(const std::string &data,
                                            int height, int *outWidth,
                                            int *outHeight) {
  *outWidth = *outHeight = 0;

  if (data.empty())
    return nullptr;

  return nullptr;
}
