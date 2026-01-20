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

#include <ext-image-copy-capture-v1.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "Display.h"
#include "Seat.h"
#include "ImageCaptureSource.h"
#include "ImageCopyCaptureSession.h"
#include "ImageCopyCaptureManager.h"

using namespace wayland;

static core::LogWriter vlog("WaylandImageCopyCaptureManager");

ImageCopyCaptureManager::ImageCopyCaptureManager(Display* display_,
                                                 ImageCaptureSource* source_,
                                                 std::function<void(uint8_t*, core::Region, uint32_t)>
                                                   bufferEventCb_,
                                                 std::function<void()>
                                                   stoppedCb_)
  : Object(display_, "ext_image_copy_capture_manager_v1",
           &ext_image_copy_capture_manager_v1_interface),
    manager(nullptr), display(display_), source(source_),
    session(nullptr), bufferEventCb(bufferEventCb_),
    stoppedCb(stoppedCb_)
{
  manager = (ext_image_copy_capture_manager_v1*)boundObject;
}

ImageCopyCaptureManager::~ImageCopyCaptureManager()
{
  delete session;
  if (manager)
    ext_image_copy_capture_manager_v1_destroy(manager);
}

void ImageCopyCaptureManager::createSession()
{
  ext_image_copy_capture_session_v1* sessionHandle =
    ext_image_copy_capture_manager_v1_create_session(manager, source->getSource(),
                                                     EXT_IMAGE_COPY_CAPTURE_MANAGER_V1_OPTIONS_PAINT_CURSORS);
  if (!sessionHandle)
    throw std::runtime_error(core::format("Unable to create image copy "
                                          "capture session"));

  session = new ImageCopyCaptureSession(display, sessionHandle,
                                        bufferEventCb, stoppedCb);
}

