// -=- OverlayPixelBuffer.h
//
// The extension of a managed pixelbuffer whith the capability to add an overlay

#ifndef __RFB_OVERLAY_PIXEL_BUFFER_H__
#define __RFB_OVERLAY_PIXEL_BUFFER_H__

#include <vector>

#include <core/Rect.h>

#include <rfb/OverlayContent.h>
#include <rfb/PixelBuffer.h>

namespace rfb {

class OverlayPixelBuffer : public ManagedPixelBuffer {
public:
  OverlayPixelBuffer(const PixelBuffer *parentBuf, const char *overlayType,
                     const char *overlayPos, const char *overlayInput,
                     const int overlayAlpha, int overlaySize,
                     int overlayPadding, const char *overlayFont);
  virtual ~OverlayPixelBuffer();

  virtual const uint8_t *getBuffer(const core::Rect &r,
                                   int *stride) const override;
  // Synchronize the overlay buffer with the parent buffer over the specified
  // damaged regions.
  void syncBuffers(const core::Region &r);
  // Set a new parent, will have to be called on each resize of the parent
  // buffer.
  void setParent(const PixelBuffer *parentBuf);
  void setSize(int w, int h) override;
  // Update the overlay position/text and redraw, e.g. after a runtime config
  // change.
  void updateOverlay(const char *overlayType, const char *overlayPos,
                     const char *overlayInput, const int overlayAlpha,
                     int overlaySize, int overlayPadding,
                     const char *overlayFont);
  // Current areas covered by the overlay content
  const std::vector<core::Rect> &getOverlayRects() const {
    return _overlayRects;
  }

private:
  // Blends overlaybuffer with the parents buffer
  void blendBuffer(const uint8_t *buf, int bufWidth, int bufHeight,
                   const core::Point &pos, double alpha) const;
  // Calculates the absolute top-left position of the watermark
  core::Point calcOverlayPosition(const char *overlayPos, int contentWidth,
                                  int contentHeight) const;
  // Calculates positions for multiple overlays
  std::vector<core::Rect> calcOverlayPositions(const char *overlayPos,
                                               int contentWidth,
                                               int contentHeight) const;
  void renderOverlay();

  const PixelBuffer *parent;
  uint8_t *overlayBuffer;
  OverlayContent *_content;
  std::string _overlayType;
  std::vector<core::Rect> _overlayRects;
  std::string _overlayPos;
  std::string _overlayInput;
  int _overlaySize;
  int _overlayPadding;
  double _overlayAlpha;
  std::string _overlayFont;
};

}; // namespace rfb

#endif // __RFB_OVERLAY_PIXEL_BUFFER_H__
