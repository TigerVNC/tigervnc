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

#include <stdint.h>

#include <rfb/PixelBuffer.h>

namespace rfb { class VNCServer; class PixelFormat;}

struct pw_buffer;
class PipeWireSource;
struct PipeWirecursor;

class PipeWirePixelBuffer : public rfb::ManagedPixelBuffer {
public:
  PipeWirePixelBuffer(int32_t pipewireFd, uint32_t pipewireId,
                      rfb::VNCServer* server);
  ~PipeWirePixelBuffer();

  void processBuffer(pw_buffer* buffer);
  void processCursor(pw_buffer* buffer);
  void updatePixelbuffer(int width, int height, rfb::PixelFormat pf);
  rfb::PixelFormat convertPixelformat(int spaFormat);
  void setCursor(int width, int height, int hotX, int hotY,
                 const unsigned char* rgbaData);
  bool hasCursorData(pw_buffer* buf);
  bool supportedCursorPixelformat(int format_);

  int fd() const { return pipewireFd; }
  int id() const { return pipewireId; }
private:

  int32_t pipewireFd;
  uint32_t pipewireId;

  rfb::VNCServer* server;
  PipeWireSource* source;
  PipeWirecursor* cursor;

};
#endif // __PIPEWIRE_PIXEL_BUFFER_H__
