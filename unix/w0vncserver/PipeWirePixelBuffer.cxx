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
#include <stdint.h>

#include <glib.h>
#include <glib-object.h>

#include <pipewire/pipewire.h>
#include <spa/param/video/raw.h>
#include <spa/utils/defs.h>
#include <spa/buffer/buffer.h>

#include <core/LogWriter.h>
#include <core/string.h>
#include <rfb/VNCServer.h>
#include <rfb/PixelFormat.h>

#include "PipeWireStream.h"
#include "PipeWirePixelBuffer.h"

static core::LogWriter vlog("PipewirePixelBuffer");

PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server_)
  : PipeWireStream(pipewireFd, pipewireId), server(server_)
{
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
}

void PipeWirePixelBuffer::processBuffer(pw_buffer* buffer)
{
  spa_buffer* spaBuffer;

  spaBuffer = buffer->buffer;

  processFrame(spaBuffer);
}

void PipeWirePixelBuffer::setParameters(int width, int height,
                                        rfb::PixelFormat pf)
{
  setSize(width, height);
  setPF(pf);
  pipewirePixelFormat = pf;
  server->setPixelBuffer(this);
}

void PipeWirePixelBuffer::processFrame(spa_buffer* buffer)
{
  int srcStride;
  uint8_t* srcBuffer;
  spa_chunk* chunk;

  chunk = buffer->datas[0].chunk;

  if (chunk->size == 0 || chunk->flags  & SPA_CHUNK_FLAG_CORRUPTED)
    return;

  // Check size
  if (chunk->size != (uint32_t) (width() * height() * (pipewirePixelFormat.bpp / 8))) {
    vlog.error("Invalid chunk size: %d", chunk->size);
    return;
  }

  srcBuffer = (uint8_t*)buffer->datas[0].data;
  srcStride = chunk->stride / (pipewirePixelFormat.bpp / 8);

  imageRect(pipewirePixelFormat, getRect(), srcBuffer, srcStride);

  server->add_changed(getRect());
}
