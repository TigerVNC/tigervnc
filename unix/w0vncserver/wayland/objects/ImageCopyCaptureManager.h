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

#ifndef __WAYLAND_IMAGE_COPY_CAPTURE_MANAGER_H__
#define __WAYLAND_IMAGE_COPY_CAPTURE_MANAGER_H__

#include <functional>

#include "Object.h"

struct ext_image_copy_capture_manager_v1;

namespace core { class Region; class Point; }

namespace wayland {
  class Display;
  class Seat;

  class ImageCaptureSource;
  class ImageCopyCaptureSession;

  class ImageCopyCaptureManager : public Object {
  public:
    ImageCopyCaptureManager(Display* display,
                            ImageCaptureSource* source,
                            std::function<void(uint8_t*, core::Region, uint32_t)>
                              bufferEventCb,
                            std::function<void()> stoppedCb);
    ~ImageCopyCaptureManager();

    void createSession();

  private:
    ext_image_copy_capture_manager_v1* manager;
    Display* display;
    ImageCaptureSource* source;
    ImageCopyCaptureSession* session;
    std::function<void(uint8_t*, core::Region, uint32_t)> bufferEventCb;
    std::function<void()> stoppedCb;
  };

};

#endif // __WAYLAND_IMAGE_COPY_CAPTURE_MANAGER_H__
