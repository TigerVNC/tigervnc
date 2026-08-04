#ifndef __RFB_OVERLAY_CONTENT_QR_H__
#define __RFB_OVERLAY_CONTENT_QR_H__

#include <string>

#include <rfb/OverlayContent.h>

namespace rfb {

class OverlayContentQr : public OverlayContent {
public:
  OverlayContentQr(const std::string &data, int height);
  virtual ~OverlayContentQr();

  virtual uint8_t *getContentPixelBuffer() override { return _buffer; }

private:
  // Encodes data into a QR code and renders it into a freshly allocated ARGB32
  // buffer
  static uint8_t *generateQrBuffer(const std::string &data, int height,
                                   int *outWidth, int *outHeight);

  uint8_t *_buffer;
};

} // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_QR_H__
