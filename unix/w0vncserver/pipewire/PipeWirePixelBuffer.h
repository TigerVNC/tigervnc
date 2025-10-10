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

#ifndef __PIPEWIRE_PIXEL_BUFFER_H__
#define __PIPEWIRE_PIXEL_BUFFER_H__

#include <rfb/PixelBuffer.h>
#include <core/Region.h>
#include <spa/buffer/buffer.h>

#include "PipeWireStream.h"

namespace rfb { class VNCServer; class PixelFormat;}

struct PipeWireCursor;

class PipeWirePixelBuffer : public rfb::ManagedPixelBuffer,
                            public PipeWireStream {
public:
  PipeWirePixelBuffer(int32_t pipewireFd, uint32_t pipewireId,
                      rfb::VNCServer* server);
  ~PipeWirePixelBuffer();

private:
  virtual void processBuffer(pw_buffer* buffer) override;
  virtual void setParameters(int width, int height, rfb::PixelFormat pf) override;

protected:
  void processFrame(spa_buffer* buffer);
  void processCursor(spa_buffer* buffer);
  void processDamage(spa_buffer* buffer);

  bool hasCursorData(spa_buffer* buffer);

  void setCursor(int width, int height, int hotX, int hotY,
                 const unsigned char* rgbaData);
  bool supportedCursorPixelformat(int format_);

private:
  rfb::VNCServer* server;
  rfb::PixelFormat pipewirePixelFormat;
  core::Region accumulatedDamage;
  PipeWireCursor* cursor;
  uint64_t lastSequence;
};
#endif // __PIPEWIRE_PIXEL_BUFFER_H__
