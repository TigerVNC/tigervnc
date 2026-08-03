// -=- OverlayContentQr.cxx
//
// Encodes data into a QR code and renders it as a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/LogWriter.h>

#include <rfb/OverlayContentQr.h>

#include <qrcodegen/qrcodegen.hpp>

using namespace rfb;

static core::LogWriter vlog("OverlayContentQr");

// Amount of padding around the qr-code
static const int quietZone = 1;

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

  if (data.empty() || (height <= 0))
    return nullptr;

  qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(data.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);


  int nativeSize = qr.getSize() + quietZone * 2;
  uint8_t *buffer = new uint8_t[height * height * 4];

  for (int y = 0; y < height; y++) {
    int moduleY = (y * nativeSize) / height - quietZone;
    for (int x = 0; x < height; x++) {
      int moduleX = (x * nativeSize) / height - quietZone;
      bool dark = qr.getModule(moduleX, moduleY);
      uint8_t *p = buffer + (y * height + x) * 4;
      uint8_t value = dark ? 0x00 : 0xff;
      p[0] = p[1] = p[2] = value;
      p[3] = 0xff;
    }
  }

  *outWidth = *outHeight = height;

  return buffer;
}
