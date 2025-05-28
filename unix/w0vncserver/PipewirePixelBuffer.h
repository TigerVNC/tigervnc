#ifndef __PIPEWIRE_STREAMER_H__
#define __PIPEWIRE_STREAMER_H__

#include <stdint.h>

#include <glib.h>
#include <spa/param/video/format-utils.h>
#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/pipewire.h>

#include <rfb/PixelBuffer.h>

namespace rfb { class VNCServer; }

struct PipeWireSource;

class PipewirePixelBuffer : public rfb::ManagedPixelBuffer {
public:
  PipewirePixelBuffer(int32_t pipewireFd, uint32_t pipewireId,
                      rfb::VNCServer* server);
  ~PipewirePixelBuffer();

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
#endif // __PIPEWIRE_STREAMER_H__
