#ifndef __PIPEWIRE_PIXEL_BUFFER_H__
#define __PIPEWIRE_PIXEL_BUFFER_H__

#include <stdint.h>

#include <glib.h>
#include <spa/param/video/format-utils.h>
#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/pipewire.h>

#include <rfb/PixelBuffer.h>

struct PipeWireData {
  // FIXME: Create a destructor for PipeWireData
  struct Rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
  };

  struct Cursor {
    uint32_t w;
    uint32_t h;
    int32_t x;
    int32_t y;
    int32_t hotspotX;
    int32_t hotspotY;
    int32_t stride;
    uint8_t *data;
  };

  spa_video_info format;
  int32_t stride;
  spa_rectangle size;
  Rect rect;

  uint8_t *buffer;
  Cursor cursor;
};

namespace rfb { class VNCServer; }

class PipeWirePixelBuffer;

struct PipeWireSource {
  GSource base;
  pw_loop* loop;
  pw_stream* stream;
  PipeWireData* data;
  rfb::VNCServer* server;
  PipeWirePixelBuffer* instance;

  void setBuffer(int width, int height, uint8_t* data, int stride);
  void setFormat(rfb::PixelFormat format);
};

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
