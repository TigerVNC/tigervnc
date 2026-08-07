// -=- OverlayContentText.h
//
// An overlay content implementation that renders text into a pixel buffer.

#ifndef __RFB_OVERLAY_CONTENT_TEXT_H__
#define __RFB_OVERLAY_CONTENT_TEXT_H__

#include <string>

#include <rfb/OverlayContent.h>

namespace rfb {

class OverlayContentText : public OverlayContent {
public:
  OverlayContentText(const std::string &text, int fontSize,
                     const std::string &font = "");
  virtual ~OverlayContentText();

  virtual uint8_t *getContentPixelBuffer() override { return _buffer; }

  // Renders text at the given pixel font size into a freshly allocated
  // ARGB32 buffer sized exactly to fit the rendered glyphs. font is a
  // fontconfig pattern (e.g. a family name). Empty selects a default font
  // Public to allow for reuse in OverlayContentPng to show error message
  static uint8_t *generateTextBuffer(const std::string &text, int size,
                                     int *outWidth, int *outHeight,
                                     const std::string &font = "");

private:
  uint8_t *_buffer;
};

} // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_TEXT_H__