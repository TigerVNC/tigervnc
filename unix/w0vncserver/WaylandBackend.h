#ifndef __WAYLAND_BACKEND_H__
#define __WAYLAND_BACKEND_H__

#include <rfb/VNCServerST.h>
#include <core/Rect.h>
#include <rfb/ScreenSet.h>

#include <glib.h>
#include <stdint.h>

class WaylandBackend {
public:
  WaylandBackend() {};
  virtual ~WaylandBackend() {};

  virtual void init() =  0;
  virtual void pointerEvent(const core::Point& pos,
                            uint16_t buttonMask) = 0;
  virtual void keyEvent(uint32_t keysym, uint32_t keycode,
                        bool down) = 0;
  virtual unsigned int setScreenLayout(int fb_width, int fb_height,
                                       const rfb::ScreenSet& layout) = 0;
};
#endif // __WAYLAND_BACKEND_H__
