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

#include <sys/mman.h>

#include <stdexcept>

#include <wayland-client-protocol.h>

#include <core/LogWriter.h>

#include "../../w0vncserver.h"
#include "Shm.h"
#include "ShmPool.h"

using namespace wayland;

static core::LogWriter vlog("WShmPool");

ShmPool::ShmPool(Shm* shm, int fd, size_t size_)
  : pool(nullptr), data(nullptr), size(size_)
{
  data = (uint8_t*)mmap(nullptr, size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
  if (data == MAP_FAILED)
    throw std::runtime_error("Failed to mmap shm");

  pool = wl_shm_create_pool(shm->getShm(), fd, size);
}

ShmPool::~ShmPool()
{
  if (munmap(data, size) < 0) {
    // FIXME: fatal_error?
    vlog.error("Failed to munmap shm");
  }
  wl_shm_pool_destroy(pool);
}

wl_buffer* ShmPool::createBuffer(int32_t offset, int32_t width,
                                 int32_t height, int32_t stride,
                                 uint32_t format)
{
  return wl_shm_pool_create_buffer(pool, offset, width, height, stride, format);
}
