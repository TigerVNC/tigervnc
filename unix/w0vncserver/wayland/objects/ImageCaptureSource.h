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

#ifndef __WAYLAND_IMAGE_CAPTURE_SOURCE_H__
#define __WAYLAND_IMAGE_CAPTURE_SOURCE_H__

#include "Object.h"

struct ext_foreign_toplevel_handle_v1;
struct ext_image_capture_source_v1;
struct ext_output_image_capture_source_manager_v1;
struct ext_foreign_toplevel_image_capture_source_manager_v1;

namespace wayland {
  class Display;
  class Output;

  class ImageCaptureSource {
  protected:
    ImageCaptureSource() {};
  public:
    virtual ~ImageCaptureSource();

    ext_image_capture_source_v1* getSource() { return source; }

  protected:
    ext_image_capture_source_v1* source;
  };

  class OutputImageCaptureSource : public Object,
                                   public ImageCaptureSource {
  public:
    OutputImageCaptureSource(Display* display, Output* output);
    ~OutputImageCaptureSource();

  private:
    ext_output_image_capture_source_manager_v1* manager;
  };

  class ForeignToplevelImageCaptureSource : public Object,
                                            public ImageCaptureSource {
  public:
    ForeignToplevelImageCaptureSource(Display* display,
                                      ext_foreign_toplevel_handle_v1* toplevel);
    ~ForeignToplevelImageCaptureSource();

  private:
    ext_foreign_toplevel_image_capture_source_manager_v1* manager;
  };
};

#endif // __WAYLAND_IMAGE_CAPTURE_SOURCE_H__
