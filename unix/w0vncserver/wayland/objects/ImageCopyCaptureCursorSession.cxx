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
#include <core/Rect.h>

#include "ImageCopyCaptureSession.h"
#include "ImageCopyCaptureCursorSession.h"

using namespace wayland;

static core::LogWriter vlog("WaylandImageCopyCaptureCursorSession");

const ext_image_copy_capture_cursor_session_v1_listener
ImageCopyCaptureCursorSession::listener = {
  .enter = [](void* data, ext_image_copy_capture_cursor_session_v1*) {
    ((ImageCopyCaptureCursorSession*)data)->handleEnter();
  },
  .leave = [](void* data, ext_image_copy_capture_cursor_session_v1*) {
    ((ImageCopyCaptureCursorSession*)data)->handleLeave();
  },
  .position = [](void* data, ext_image_copy_capture_cursor_session_v1*,
                 int32_t x, int32_t y) {
    ((ImageCopyCaptureCursorSession*)data)->handlePosition(x, y);
  },
  .hotspot = [](void* data, ext_image_copy_capture_cursor_session_v1*,
                int32_t x, int32_t y) {
    ((ImageCopyCaptureCursorSession*)data)->handleHotspot(x, y);
  },
};

ImageCopyCaptureCursorSession::ImageCopyCaptureCursorSession(Display* display_,
                                                             ext_image_copy_capture_cursor_session_v1* session_,
                                                             std::function<void(int, int, const core::Point&, uint32_t,const uint8_t*)>
                                                               cursorFrameCb_,
                                                             std::function<void(const core::Point&)>
                                                               cursorPosCb_,
                                                             std::function<void()>
                                                               stoppedCb_)
  : session(session_), captureSession(nullptr),
    cursorFrameCb(cursorFrameCb_), cursorPosCb(cursorPosCb_),
    hotspot({0, 0})
{
  ext_image_copy_capture_cursor_session_v1_add_listener(session, &listener, this);

  ext_image_copy_capture_session_v1* captureSessionHandle =
    ext_image_copy_capture_cursor_session_v1_get_capture_session(session);

  if (!captureSessionHandle) {
    ext_image_copy_capture_cursor_session_v1_destroy(session);
    throw std::runtime_error("Could not create cursor capture session");
  }

  std::function<void(uint8_t*, core::Region, uint32_t)> bufferEventCb =
    [this](uint8_t*, core::Region, uint32_t) {
      cursorFrameCb(captureSession->getWidth(),
                    captureSession->getHeight(),
                    hotspot, captureSession->getFormat(),
                    captureSession->getBufferData());
  };

  captureSession = new ImageCopyCaptureSession(display_,
                                               captureSessionHandle,
                                               bufferEventCb,
                                               stoppedCb_);
}

ImageCopyCaptureCursorSession::~ImageCopyCaptureCursorSession()
{
  delete captureSession;
  if (session)
    ext_image_copy_capture_cursor_session_v1_destroy(session);
}

void ImageCopyCaptureCursorSession::handleEnter()
{
}

void ImageCopyCaptureCursorSession::handleLeave()
{
}

void ImageCopyCaptureCursorSession::handlePosition(int32_t x, int32_t y)
{
  cursorPosCb({x, y});
}

void ImageCopyCaptureCursorSession::handleHotspot(int32_t x, int32_t y)
{
  hotspot = {x, y};
}
