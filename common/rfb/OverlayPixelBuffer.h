// -=- OverlayPixelBuffer.h
//
// The an extension of a pixelbuffer whith the capability to add an overlay

#ifndef __RFB_OVERLAY_PIXEL_BUFFER_H__
#define __RFB_OVERLAY_PIXEL_BUFFER_H__

#include <core/Rect.h>

#include <rfb/PixelBuffer.h>

namespace rfb {

class OverlayPixelBuffer : public ManagedPixelBuffer {
public:
  OverlayPixelBuffer(const PixelBuffer *parentBuf, const char *overlayPos,
                     const char *overlayText);
  OverlayPixelBuffer(const PixelBuffer *parentBuf);
  virtual ~OverlayPixelBuffer();

  virtual const uint8_t *getBuffer(const core::Rect &r,
                                   int *stride) const override;
  void placeOverlay(const core::Rect &r) const;
  void setOverlayRect(const char *overlayPos);
  // Synchronize the overlay buffer with the parent buffer over the specified
  // damaged regions.
  void syncBuffers(const core::Region &r);
  // Set a new parent, will have to be called on each resize of the parent
  // buffer.
  void setParent(const PixelBuffer *parentBuf);
  void setSize(int w, int h) override;
  // Update the overlay position/text and redraw, e.g. after a runtime config
  // change.
  void updateOverlay(const char *overlayPos, const char *overlayText);
  // Current area covered by the overlay box, e.g. to mark it as changed.
  core::Rect getOverlayRect() const { return _overlayRect; }
  // Size (width, height) the overlay text takes up when rendered at the
  // current font size, excluding padding.
  core::Point getTextSize() const;

private:
  void renderText(const core::Rect &rect, void *destImage) const;
  const PixelBuffer *parent;
  uint8_t *overlayBuffer;
  core ::Rect _overlayRect;
  std::string _overlayPos;
  std::string _overlayText;
  int _overlayFontSize;
  int _overlayPadding;
};

}; // namespace rfb

#endif // __RFB_OVERLAY_PIXEL_BUFFER_H__
