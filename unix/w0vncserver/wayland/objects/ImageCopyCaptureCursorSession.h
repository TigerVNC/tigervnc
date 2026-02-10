/* Copyright 2026 Tobias Fahleson for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __WAYLAND_IMAGE_COPY_CAPTURE_CURSOR_SESSION_H__
#define __WAYLAND_IMAGE_COPY_CAPTURE_CURSOR_SESSION_H__

#include <functional>

#include <core/Rect.h>

#include "Object.h"

struct ext_image_copy_capture_cursor_session_v1;
struct ext_image_copy_capture_cursor_session_v1_listener;

namespace wayland {
  class Display;
  class ImageCopyCaptureSession;

  class ImageCopyCaptureCursorSession {
  public:
    ImageCopyCaptureCursorSession(Display* display,
                                  ext_image_copy_capture_cursor_session_v1* session,
                                  std::function<void(int, int, const core::Point&, uint32_t,const uint8_t*)>
                                    cursorFrameCb,
                                  std::function<void(const core::Point&)> cursorPosCb,
                                  std::function<void()> stoppedCb);
    ~ImageCopyCaptureCursorSession();

  private:
    void handleEnter();
    void handleLeave();
    void handlePosition(int32_t x, int32_t y);
    void handleHotspot(int32_t x, int32_t y);

  private:
    ext_image_copy_capture_cursor_session_v1* session;
    ImageCopyCaptureSession* captureSession;
    std::function<void(int, int, const core::Point&, uint32_t,const uint8_t*)> cursorFrameCb;
    std::function<void(const core::Point&)> cursorPosCb;
    core::Point hotspot;
    static const ext_image_copy_capture_cursor_session_v1_listener listener;
  };
};

#endif // __WAYLAND_IMAGE_COPY_CAPTURE_CURSOR_SESSION_H__
