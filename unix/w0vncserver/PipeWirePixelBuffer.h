#ifndef __PIPEWIRE_PIXEL_BUFFER_H__
#define __PIPEWIRE_PIXEL_BUFFER_H__

#include <stdint.h>

#include <glib.h>
#include <pipewire/stream.h>

#include <rfb/PixelBuffer.h>

namespace rfb { class VNCServer; class PixelFormat;}

class PipeWireSource;

class PipeWirePixelBuffer : public rfb::ManagedPixelBuffer {
public:
  PipeWirePixelBuffer(int32_t pipewireFd, uint32_t pipewireId,
                      rfb::VNCServer* server);
  ~PipeWirePixelBuffer();

  void processBuffer(pw_buffer* buffer);
  void updatePixelbuffer(int width, int height, rfb::PixelFormat pf);
  rfb::PixelFormat convertPixelformat(int format_);

  int fd() const { return pipewireFd_; }
  int id() const { return pipewireId_; }
private:

  int32_t pipewireFd_;
  uint32_t pipewireId_;

  rfb::VNCServer* server_;
  PipeWireSource* source;
};
#endif // __PIPEWIRE_PIXEL_BUFFER_H__
