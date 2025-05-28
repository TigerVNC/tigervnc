#ifndef __PIPEWIRE_PIXEL_BUFFER_H__
#define __PIPEWIRE_PIXEL_BUFFER_H__

#include <stdint.h>

#include <glib.h>
#include <spa/param/video/format-utils.h>
#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/pipewire.h>

#include <rfb/PixelBuffer.h>

namespace rfb { class VNCServer; }

class PipeWirePixelBuffer;
struct PipeWireSource;

class PipeWirePixelBuffer : public rfb::FullFramePixelBuffer {
public:
  PipeWirePixelBuffer(int32_t pipewireFd, uint32_t pipewireId,
                      rfb::VNCServer* server);
  ~PipeWirePixelBuffer();

  void start();

private:
  friend struct PipeWireSource;

  int32_t pipewireFd_;
  uint32_t pipewireId_;

  PipeWireSource *source;
  pw_loop* loop;
  pw_context* context;
  pw_core* core;
  pw_registry* registry;
  pw_properties* props;
  spa_hook streamListener;
};
#endif // __PIPEWIRE_PIXEL_BUFFER_H__
