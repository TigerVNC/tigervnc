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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <ext-image-capture-source-v1.h>

#include <core/LogWriter.h>

#include "Display.h"
#include "Output.h"
#include "ImageCaptureSource.h"

using namespace wayland;

static core::LogWriter vlog("WaylandImageCaptureSource");

ImageCaptureSource::~ImageCaptureSource()
{
  if (source)
    ext_image_capture_source_v1_destroy(source);
}

OutputImageCaptureSource::OutputImageCaptureSource(Display* display,
                                                   Output* output)
  : Object(display, "ext_output_image_capture_source_manager_v1",
           &ext_output_image_capture_source_manager_v1_interface)
{
  manager = (ext_output_image_capture_source_manager_v1*)boundObject;
  source = ext_output_image_capture_source_manager_v1_create_source(manager, output->getOutput());
  if (!source) {
    ext_output_image_capture_source_manager_v1_destroy(manager);
    throw std::runtime_error("Failed to create capture source");
  }
}

OutputImageCaptureSource::~OutputImageCaptureSource()
{
  if (manager)
    ext_output_image_capture_source_manager_v1_destroy(manager);
}

// FIXME: Create a ext_foreign_toplevel_handle_v1 object wrapper
ForeignToplevelImageCaptureSource::ForeignToplevelImageCaptureSource(Display* display_,
                                                                     ext_foreign_toplevel_handle_v1* toplevel)
  : Object(display_, "ext_foreign_toplevel_image_capture_source_manager_v1",
           &ext_foreign_toplevel_image_capture_source_manager_v1_interface),
    manager(nullptr)
{
  manager = (ext_foreign_toplevel_image_capture_source_manager_v1*)boundObject;
  source = ext_foreign_toplevel_image_capture_source_manager_v1_create_source(manager, toplevel);
  if (!source) {
    ext_foreign_toplevel_image_capture_source_manager_v1_destroy(manager);
    throw std::runtime_error("Failed to create capture source");
  }
}

ForeignToplevelImageCaptureSource::~ForeignToplevelImageCaptureSource()
{
  if (manager)
    ext_foreign_toplevel_image_capture_source_manager_v1_destroy(manager);
}
