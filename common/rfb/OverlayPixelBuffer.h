// -=- OverlayPixelBuffer.h
//
// The an extension of a pixelbuffer whith the capability to add an overlay

#ifndef __RFB_OVERLAY_PIXEL_BUFFER_H__
#define __RFB_OVERLAY_PIXEL_BUFFER_H__

#include <core/Rect.h>

#include <rfb/OverlayContent.h>
#include <rfb/PixelBuffer.h>

namespace rfb {

class OverlayPixelBuffer : public ManagedPixelBuffer {
public:
  OverlayPixelBuffer(const PixelBuffer *parentBuf, const char *overlayPos,
                     const char *overlayText, int overlayFontSize);
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
  void updateOverlay(const char *overlayPos, const char *overlayText,
                     int overlayFontSize);
  // Current area covered by the overlay text, e.g. to mark it as changed.
  core::Rect getOverlayRect() const { return _overlayRect; }

private:
  // Blends overlaybuffer with the parents buffer
  void blendBuffer(const uint8_t *buf, int bufWidth, int bufHeight,
                   const core::Point &pos, double alpha) const;
  // Calculates the absolute top-left position of the watermark
  core::Point calcOverlayPosition(const char *overlayPos, int contentWidth,
                           int contentHeight) const;
  void renderOverlay();

  const PixelBuffer *parent;
  uint8_t *overlayBuffer;
  OverlayContent *_content;
  core::Point _textPos;
  core ::Rect _overlayRect;
  std::string _overlayPos;
  std::string _overlayText;
  int _overlayFontSize;
  int _overlayPadding;
  double _overlayAlpha;
};

}; // namespace rfb

#endif // __RFB_OVERLAY_PIXEL_BUFFER_H__
