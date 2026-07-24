#ifndef __RFB_OVERLAY_CONTENT_TEXT_H__
#define __RFB_OVERLAY_CONTENT_TEXT_H__

#include <string>

#include <rfb/OverlayContent.h>

namespace rfb {

class OverlayContentText : public OverlayContent {
public:
  OverlayContentText(const std::string &text, int fontSize);
  virtual ~OverlayContentText();

  virtual uint8_t *getContentPixelBuffer() override { return _buffer; }

private:
  // Renders text at the given pixel font size into a freshly allocated
  // ARGB32 buffer sized exactly to fit the rendered glyphs.
  static uint8_t *generateTextBuffer(const std::string &text, int size,
                                     int *outWidth, int *outHeight);

  uint8_t *_buffer;
};

} // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_TEXT_H__