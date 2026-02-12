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

#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include <stdexcept>

#include <ext-image-copy-capture-v1.h>

#include <core/LogWriter.h>
#include <core/string.h>

#include "Shm.h"
#include "ShmPool.h"

#include "ImageCaptureSource.h"
#include "ImageCopyCaptureSession.h"

using namespace wayland;

static core::LogWriter vlog("WaylandImageCopyCaptureSession");

// Preferred pixel formats, in prioritized order
// FIXME: We probably prefer XRGB8888 in most cases, but we should try
//        to respect whatever the client has requested if the compositor
//        also supports it.
static std::vector<uint32_t> preferredFormats = {
  WL_SHM_FORMAT_XRGB8888,
  WL_SHM_FORMAT_ARGB8888,
  WL_SHM_FORMAT_RGBX8888,
  WL_SHM_FORMAT_RGBA8888,
  WL_SHM_FORMAT_XBGR8888,
  WL_SHM_FORMAT_ABGR8888,
};

const ext_image_copy_capture_session_v1_listener ImageCopyCaptureSession::listener = {
  .buffer_size = [](void* data, ext_image_copy_capture_session_v1*,
                    uint32_t width, uint32_t height) {
    ((ImageCopyCaptureSession*)data)->handleBufferSize(width, height);
  },
  .shm_format = [](void* data, ext_image_copy_capture_session_v1*, uint32_t format) {
    ((ImageCopyCaptureSession*)data)->handleShmFormat(format);
  },
  .dmabuf_device = [](void* data, ext_image_copy_capture_session_v1*, wl_array* device) {
    ((ImageCopyCaptureSession*)data)->handleDmabufDevice(device);
  },
  .dmabuf_format = [](void* data, ext_image_copy_capture_session_v1*,
                      uint32_t format, wl_array* modifiers) {
    ((ImageCopyCaptureSession*)data)->handleDmabufFormat(format, modifiers);
  },
  .done = [](void* data, ext_image_copy_capture_session_v1*) {
    ((ImageCopyCaptureSession*)data)->handleDone();
  },
  .stopped = [](void* data, ext_image_copy_capture_session_v1*) {
    ((ImageCopyCaptureSession*)data)->handleStopped();
  },
};

const ext_image_copy_capture_frame_v1_listener ImageCopyCaptureSession::frameListener = {
  .transform = [](void* data, ext_image_copy_capture_frame_v1*, uint32_t transform) {
    ((ImageCopyCaptureSession*)data)->handleFrameTransform(transform);
  },
  .damage = [](void* data, ext_image_copy_capture_frame_v1*,
               int32_t x, int32_t y, int32_t width, int32_t height) {
    ((ImageCopyCaptureSession*)data)->handleFrameDamage(x, y, width, height);
  },
  .presentation_time = [](void* data, ext_image_copy_capture_frame_v1*,
                          uint32_t tvSecHi, uint32_t tvSecLo, uint32_t tvNsec) {
    ((ImageCopyCaptureSession*)data)->handleFramePresentationTime(tvSecHi, tvSecLo, tvNsec);
  },
  .ready = [](void* data, ext_image_copy_capture_frame_v1*) {
    ((ImageCopyCaptureSession*)data)->handleFrameReady();
  },
  .failed = [](void* data, ext_image_copy_capture_frame_v1*, uint32_t reason) {
    ((ImageCopyCaptureSession*)data)->handleFrameFailed(reason);
  },
};


ImageCopyCaptureSession::ImageCopyCaptureSession(Display* display_,
                                                 ext_image_copy_capture_session_v1* session_,
                                                 std::function<void(uint8_t*, core::Region, uint32_t)>
                                                   frameReadyCb_,
                                                 std::function<void()>
                                                   stoppedCb_)
  : display(display_), frameReadyCb(frameReadyCb_), session(session_),
    frame(nullptr), shm(nullptr), pool(nullptr), buffer(nullptr),
    width(0), height(0), stoppedCb(stoppedCb_)
{
  ext_image_copy_capture_session_v1_add_listener(session, &listener, this);
}

ImageCopyCaptureSession::~ImageCopyCaptureSession()
{
  if (frame)
    ext_image_copy_capture_frame_v1_destroy(frame);
  if (buffer)
    wl_buffer_destroy(buffer);

  ext_image_copy_capture_session_v1_destroy(session);

  delete pool;
  delete shm;
}

uint8_t* ImageCopyCaptureSession::getBufferData() const
{
  assert(pool);

  return pool->getData();
}

void ImageCopyCaptureSession::handleBufferSize(uint32_t width_,
                                               uint32_t height_)
{
  // Detect resize
  if ((width && height) && (width_ != width || height_ != height))
    cleanupBuffers();

  width = width_;
  height = height_;
}

void ImageCopyCaptureSession::handleShmFormat(uint32_t format_)
{
  formats.push_back((uint32_t)format_);
}

void ImageCopyCaptureSession::handleDmabufDevice(wl_array* /* device */)
{
}

void ImageCopyCaptureSession::handleDmabufFormat(uint32_t /* format */,
                                                 wl_array* /* modifiers */)
{
}

void ImageCopyCaptureSession::handleDone()
{
  size_t size;
  int fd;

  assert(!shm);
  assert(!pool);
  assert(!buffer);

  if (formats.empty()) {
    vlog.error("No shm formats available");
    stoppedCb();
    return;
  }

  try {
    format = preferredFormat();
  } catch (std::exception& e) {
    vlog.error("Failed to pick shm format: %s", e.what());
    stoppedCb();
    return;
  }

  fd = memfd_create("w0vncserver-image-copy-shm", FD_CLOEXEC);
  if (fd < 0) {
    vlog.error("Failed to allocate shm: %s", strerror(errno));
    stoppedCb();
    return;
  }

  size = width * height * 4;
  if (ftruncate(fd, size) < 0) {
    vlog.error("Failed to truncate shm: %s", strerror(errno));
    close(fd);
    return;
  }

  try {
    shm = new Shm(display);
    pool = new ShmPool(shm, fd, size);
  } catch (std::exception& e) {
    vlog.error("Failed to create shm pool: %s", e.what());
    close(fd);
    stoppedCb();
    return;
  }

  close(fd);

  buffer = pool->createBuffer(0, width, height, width * 4, format);
  if (!buffer) {
    vlog.error("Failed to create buffer");
    stoppedCb();
    return;
  }

  createFrame();
}

void ImageCopyCaptureSession::handleStopped()
{
  vlog.status("Capture session stopped");

  if (frame) {
    ext_image_copy_capture_frame_v1_destroy(frame);
    frame = nullptr;
  }

  ext_image_copy_capture_session_v1_destroy(session);
  session = nullptr;

  stoppedCb();
}

void ImageCopyCaptureSession::handleFrameTransform(uint32_t /* transform */)
{
}

void ImageCopyCaptureSession::handleFrameDamage(int32_t x, int32_t y,
                                                int32_t width_,
                                                int32_t height_)
{
  core::Point tl{static_cast<int>(x), static_cast<int>(y)};
  core::Point br{static_cast<int>(x + width_),
                 static_cast<int>(y + height_)};
  damage.assign_union({{tl, br}});
}

void ImageCopyCaptureSession::handleFramePresentationTime(uint32_t /* tvSecHi */,
                                                          uint32_t /* tvSecLo */,
                                                          uint32_t /* tvNsec */)
{
}

void ImageCopyCaptureSession::handleFrameReady()
{
  assert(frame);

  frameReadyCb(pool->getData(), damage, format);

  ext_image_copy_capture_frame_v1_destroy(frame);
  frame = nullptr;

  createFrame();
}

void ImageCopyCaptureSession::handleFrameFailed(uint32_t reason)
{
  if (frame) {
    ext_image_copy_capture_frame_v1_destroy(frame);
    frame = nullptr;
  }

  if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_UNKNOWN) {
    vlog.error("Could not capture frame: unknown reason - trying again");
    createFrame();
  } else if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_BUFFER_CONSTRAINTS) {
    vlog.error("Invalid buffer - trying again");
    cleanupBuffers();
    createFrame();
  } else if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_STOPPED) {
    vlog.error("Could not capture frame, stopping capture session");
    stoppedCb();
  }
}

void ImageCopyCaptureSession::createFrame()
{
  assert(frame == nullptr);

  frame = ext_image_copy_capture_session_v1_create_frame(session);
  ext_image_copy_capture_frame_v1_add_listener(frame, &frameListener, this);

  damage.clear();

  ext_image_copy_capture_frame_v1_attach_buffer(frame, buffer);
  ext_image_copy_capture_frame_v1_capture(frame);
}

void ImageCopyCaptureSession::cleanupBuffers()
{
  if (frame)
    ext_image_copy_capture_frame_v1_destroy(frame);

  if (buffer)
    wl_buffer_destroy(buffer);

  frame = nullptr;
  buffer = nullptr;

  delete pool;
  pool = nullptr;

  delete shm;
  shm = nullptr;

  formats.clear();
  damage.clear();
}

uint32_t ImageCopyCaptureSession::preferredFormat()
{
  for (uint32_t shmFormat : preferredFormats) {
    if (std::find(formats.begin(), formats.end(), shmFormat) != formats.end())
      return shmFormat;
  }

  throw std::runtime_error("No supported shm format found");
}
