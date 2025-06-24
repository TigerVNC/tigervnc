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

#ifndef __WAYLAND_SHM_POOL_H__
#define __WAYLAND_SHM_POOL_H__

#include <stddef.h>
#include <stdint.h>

struct wl_shm_pool;
struct wl_buffer;

namespace wayland {
  class Shm;

  class ShmPool {
  public:
    ShmPool(Shm* shm, int fd, size_t size);
    ~ShmPool();

    wl_shm_pool* getShmPool() { return pool; }
    uint8_t* getData() { return data; }
    size_t getSize() { return size; }

    wl_buffer* createBuffer(int32_t offset, int32_t width, int32_t height,
                            int32_t stride, uint32_t format);

  private:
    wl_shm_pool* pool;
    uint8_t* data;
    size_t size;
  };
};

#endif // __WAYLAND_SHM_POOL_H__
