#ifndef __RFB_OVERLAY_CONTENT_PNG_H__
#define __RFB_OVERLAY_CONTENT_PNG_H__

#include <string>

#include <rfb/OverlayContent.h>

namespace rfb {

class OverlayContentPng : public OverlayContent {
public:
  OverlayContentPng(const std::string &filePath);
  virtual ~OverlayContentPng();

  virtual uint8_t *getContentPixelBuffer() override { return _buffer; }

private:
  // Decodes the PNG file at filePath into a freshly allocated ARGB32
  // buffer sized exactly to the image dimensions.
  static uint8_t *loadPngBuffer(const std::string &filePath, int *outWidth,
                                int *outHeight);

  uint8_t *_buffer;
};

} // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_PNG_H__
