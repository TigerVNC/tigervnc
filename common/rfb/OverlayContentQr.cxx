// -=- OverlayContentQr.cxx
//
// Encodes data into a QR code and renders it as a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/LogWriter.h>

#include <rfb/OverlayContentQr.h>

#include <qrcodegen/qrcodegen.hpp>

#include <pixman.h>

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

//Scale buffer to desired size
static uint8_t *scaleArgbBuffer(const uint8_t *srcBuffer, int srcWidth,
                                int srcHeight, int dstWidth, int dstHeight) {
  uint8_t *dstBuffer = new uint8_t[dstWidth * dstHeight * 4];
  
  pixman_image_t *srcImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, srcWidth, srcHeight,
      reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(srcBuffer)),
      srcWidth * 4);
  pixman_image_t *dstImage = pixman_image_create_bits(
      PIXMAN_a8r8g8b8, dstWidth, dstHeight,
      reinterpret_cast<uint32_t *>(dstBuffer),
      dstWidth * 4);

  pixman_transform_t transform;
  pixman_transform_init_scale(
      &transform,
      pixman_double_to_fixed(static_cast<double>(srcWidth) / dstWidth),
      pixman_double_to_fixed(static_cast<double>(srcHeight) / dstHeight));
  pixman_image_set_transform(srcImage, &transform);
  pixman_image_set_filter(srcImage, PIXMAN_FILTER_NEAREST, nullptr, 0);

  pixman_image_composite(PIXMAN_OP_SRC, srcImage, nullptr, dstImage, 0, 0, 0,
                         0, 0, 0, static_cast<uint16_t>(dstWidth),
                         static_cast<uint16_t>(dstHeight));

  pixman_image_unref(srcImage);
  pixman_image_unref(dstImage);

  return dstBuffer;
}


// Encodes data into a QR code, rendered into a new pixelbuffer scaled to the
// given height (QR codes are square, so width matches height).
uint8_t *OverlayContentQr::generateQrBuffer(const std::string &data,
                                            int height, int *outWidth,
                                            int *outHeight) {
  *outWidth = *outHeight = 0;

  if (data.empty() || (height <= 0))
    return nullptr;

  qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(data.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);

  // Render smallest possible qr-code, 1 pixel per module
  int nativeSize = qr.getSize() + quietZone * 2;
  uint8_t *native = new uint8_t[nativeSize * nativeSize * 4];

  for (int y = 0; y < nativeSize; y++) {
    for (int x = 0; x < nativeSize; x++) {
      bool dark = qr.getModule(x - quietZone, y - quietZone);
      uint8_t *p = native + (y * nativeSize + x) * 4;
      uint8_t value = dark ? 0x00 : 0xff;
      p[0] = p[1] = p[2] = value;
      p[3] = 0xff;
    }
  }

  //Scaling up to desired size
  uint8_t *scaled = scaleArgbBuffer(native, nativeSize, nativeSize, height, height);

  *outWidth = *outHeight = height;

  return scaled;
}
