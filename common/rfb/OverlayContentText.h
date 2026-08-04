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

  // Renders text at the given pixel font size into a freshly allocated
  // ARGB32 buffer sized exactly to fit the rendered glyphs.
  // Moved to public to allow for reuse in OverlayContentQr, which uses text to
  // render error messages.
  static uint8_t *generateTextBuffer(const std::string &text, int size,
                                     int *outWidth, int *outHeight);

private:
  uint8_t *_buffer;
};

} // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_TEXT_H__