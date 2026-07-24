// -=- OverlayPixelBuffer.h
//
// The an extension of a pixelbuffer whith the capability to add an overlay

#ifndef __RFB_OVERLAY_CONTENT_H__
#define __RFB_OVERLAY_CONTENT_H__

#include <cstdint>
namespace rfb {

class OverlayContent {
private:
  uint8_t _alpha;

protected:
  int _width, _height;

public:
  virtual ~OverlayContent() {}
  virtual uint8_t *getContentPixelBuffer() = 0;

  uint8_t getAlpha() { return _alpha; }
  void setAlpha(uint8_t alpha) {_alpha = alpha; }

  int getWidth() const { return _width; }
  int getHeight() const { return _height; }
};

}; // namespace rfb

#endif // __RFB_OVERLAY_PIXEL_BUFFER_H__
