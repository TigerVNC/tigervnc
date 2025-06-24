/* Copyright 2025 Adam Halim for Cendio AB
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

#include <wlr-screencopy-unstable-v1.h>

#include <core/LogWriter.h>
#include <core/string.h>
#include <rfb/PixelFormat.h>

#include "../../w0vncserver.h"
#include "Display.h"
#include "Object.h"
#include "Output.h"
#include "Shm.h"
#include "ShmPool.h"
#include "ScreencopyManager.h"

using namespace wayland;

static core::LogWriter vlog("WawylandScreencopyManager");

const zwlr_screencopy_frame_v1_listener ScreencopyManager::listener = {
  .buffer = [](void* data, zwlr_screencopy_frame_v1*, uint32_t format,
               uint32_t width, uint32_t height, uint32_t stride) {
    ((ScreencopyManager*)data)->handleScreencopyBuffer(format, width, height, stride);
  },
  .flags = [](void* data, zwlr_screencopy_frame_v1*, uint32_t flags) {
    ((ScreencopyManager*)data)->handleScreencopyFlags(flags);
  },
  .ready = [](void* data, zwlr_screencopy_frame_v1*,
              uint32_t /* tvSecHi */, uint32_t /* tvSecLo */,
              uint32_t /* tvNsec */) {
    ((ScreencopyManager*)data)->handleScreencopyReady();
  },
  .failed = [](void* data, zwlr_screencopy_frame_v1*) {
     ((ScreencopyManager*)data)->handleScreencopyFailed();
  },
  .damage = [](void* data, zwlr_screencopy_frame_v1*, uint32_t x,
               uint32_t y, uint32_t width, uint32_t height) {
    ((ScreencopyManager*)data)->handleScreencopyDamage(x, y, width, height);
  },
  .linux_dmabuf = [](void* data, zwlr_screencopy_frame_v1*,
                     uint32_t format, uint32_t width, uint32_t height) {
    ((ScreencopyManager*)data)->handleScreencopyLinuxDmabuf(format, width, height);
  },
  .buffer_done = [](void* data, zwlr_screencopy_frame_v1*) {
    ((ScreencopyManager*)data)->handleScreencopyBufferDone();
  }
};

struct BufferInfo {
  uint32_t format;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
};

ScreencopyManager::ScreencopyManager(Display* display,
                                           Output* output_)
 : Object(display, "zwlr_screencopy_manager_v1",
           &zwlr_screencopy_manager_v1_interface),
   output(output_), screencopyManager(nullptr), frame(nullptr),
   info(nullptr), shm(nullptr), pool(nullptr), buffer(nullptr)
{
  size_t size;

  screencopyManager = (zwlr_screencopy_manager_v1*)boundObject;

  display->roundtrip();

  shm = new Shm(display);

  size = output_->getWidth() * output_->getHeight() * 4;
  assert(size);

  initBuffers(size);
}

ScreencopyManager::~ScreencopyManager()
{
  if (screencopyManager)
    zwlr_screencopy_manager_v1_destroy(screencopyManager);
  if (frame)
    zwlr_screencopy_frame_v1_destroy(frame);
  if (buffer)
    wl_buffer_destroy(buffer);

  delete info;
  delete shm;
  delete pool;
}

uint8_t* ScreencopyManager::getBufferData()
{
  return pool->getData();
}

void ScreencopyManager::captureFrame()
{
  assert(frame == nullptr);

  // FIXME: Handle multiple outputs
  // FIXME: This will render the cursor as part of the stream.
  //        Getting the cursor position requires the newer
  //        ext-image-copy-capture-v1 protocol
  frame = zwlr_screencopy_manager_v1_capture_output(screencopyManager,
                                                    1, output->getOutput());
  zwlr_screencopy_frame_v1_add_listener(frame, &listener, this);
}

void ScreencopyManager::captureFrameDone()
{
  assert(frame != nullptr);

  zwlr_screencopy_frame_v1_destroy(frame);
  frame = nullptr;
}

void ScreencopyManager::resize()
{
  if (frame)
    zwlr_screencopy_frame_v1_destroy(frame);
  frame = nullptr;

  if (buffer)
    wl_buffer_destroy(buffer);
  buffer = nullptr;

  delete pool;
  pool = nullptr;

  initBuffers(output->getWidth() * output->getHeight() * 4);
}

void ScreencopyManager::initBuffers(size_t size)
{
  int fd;

  fd = memfd_create("w0vncserver-shm", FD_CLOEXEC);
  if (fd < 0) {
    fatal_error("Failed to allocate shm: %s", strerror(errno));
    return;
  }

  if (ftruncate(fd, size) < 0) {
    fatal_error("Failed to truncate shm: %s", strerror(errno));
    return;
  }

  pool = new ShmPool(shm, fd, size);

  close(fd);
}

rfb::PixelFormat ScreencopyManager::convertPixelformat(uint32_t format)
{
  switch (format) {
    case WL_SHM_FORMAT_XRGB8888:
    case WL_SHM_FORMAT_ARGB8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              16, 8, 0);
    case WL_SHM_FORMAT_RGBX8888:
    case WL_SHM_FORMAT_RGBA8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              24, 16, 8);
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
      return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                              0, 8, 16);
    default:
      throw std::runtime_error(core::format("format %d not supported",
                                            format));
  }
}

void ScreencopyManager::handleScreencopyBuffer(uint32_t format,
                                               uint32_t width,
                                               uint32_t height,
                                               uint32_t stride)
{
  if (output->getHeight() != height || output->getWidth() != width) {
    vlog.debug("Detected resize, destroying frame");
    zwlr_screencopy_frame_v1_destroy(frame);
    frame = nullptr;
    return;
  }

  delete info;

  info = new BufferInfo {
    .format = format,
    .width = width,
    .height = height,
    .stride = stride
  };
}

void ScreencopyManager::handleScreencopyFlags(uint32_t /* flags */)
{
  // FIXME: This tells us if the contents are y-inverted.
  //        Should probably handle this.
}

void ScreencopyManager::handleScreencopyReady()
{
  captureFrameDone();
}

rfb::PixelFormat ScreencopyManager::getPixelFormat()
{
  assert(info);

  rfb::PixelFormat pf;

  try {
    pf = convertPixelformat(info->format);
  } catch (std::runtime_error& e) {
    fatal_error("Failed to get pixelformat: %s", e.what());
  }

  return pf;
}

void ScreencopyManager::handleScreencopyFailed()
{
  vlog.error("Frame could not be copied");
  captureFrameDone();
}

void ScreencopyManager::handleScreencopyDamage(uint32_t /* x */,
                                               uint32_t /* y */,
                                               uint32_t /* width */,
                                               uint32_t /* height */)
{
  // FIXME: Implement damage.
}

void ScreencopyManager::handleScreencopyLinuxDmabuf(uint32_t /* format */,
                                                    uint32_t /* width */,
                                                    uint32_t /* height */)
{
  // FIXME: Implement this?
}

void ScreencopyManager::handleScreencopyBufferDone()
{
  if (pool->getSize() != output->getWidth() * output->getHeight() * 4) {
    vlog.debug("Detected resize, aborting capture");
    captureFrameDone();
    return;
  }

  if (!buffer) {
    // FIXME: Check if buffer paramters have changed
    // FIXME: Sanity check with BufferInfo
    buffer = pool->createBuffer(0, output->getWidth(), output->getHeight(),
                                output->getWidth() * 4, info->format);
  }

  zwlr_screencopy_frame_v1_copy(frame, buffer);
}
