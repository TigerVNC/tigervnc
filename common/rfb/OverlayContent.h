// -=- OverlayContent.h
//
// Baseclass for overlay content, which can be rendered on top of the VNC session

#ifndef __RFB_OVERLAY_CONTENT_H__
#define __RFB_OVERLAY_CONTENT_H__

#include <cstdint>
namespace rfb {

class OverlayContent {

protected:
  int _width, _height;

public:
  virtual ~OverlayContent() {}
  virtual uint8_t* getContentPixelBuffer() = 0;

  int getWidth() const { return _width; }
  int getHeight() const { return _height; }
};

}; // namespace rfb

#endif // __RFB_OVERLAY_CONTENT_H__
