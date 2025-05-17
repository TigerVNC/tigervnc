#ifndef __PIPEWIRE_H__
#define __PIPEWIRE_H__

#include <stdint.h>

#include <spa/param/video/format-utils.h>
#include "glib.h"
#include "pipewire/context.h"
#include "pipewire/core.h"
#include <pipewire/pipewire.h>

class WDesktop;

struct pw_data {
  // FIXME: Create a destructor for pw_data
  struct rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
  };

  struct cursor {
    uint32_t w;
    uint32_t h;
    int32_t x;
    int32_t y;
    int32_t hotspot_x;
    int32_t hotspot_y;
    int32_t stride;
    uint8_t *data;
  };

  spa_io_position* position;
  spa_io_clock* clock;

  spa_video_info format;
  int32_t stride;
  spa_rectangle size;
  rect rect;

  uint8_t* buffer;
  cursor cursor;
};

typedef struct _PipeWireSource
{
  GSource base;
  pw_loop* loop;
  pw_stream* stream;
  pw_data* data;
  WDesktop* desktop_;
} PipeWireSource;

class Pipewire {
public:
  Pipewire(int32_t pipewire_fd, uint32_t pipewire_id, WDesktop* desktop);
  ~Pipewire();

  void start();

private:
  static void on_stream_state_changed(void *_data,
                                      enum pw_stream_state old,
                                      enum pw_stream_state state,
                                      const char *error);
  static void on_stream_io_changed(void *_data, uint32_t id, void *area,
                                   uint32_t size);
  static void on_stream_param_changed(void *_data, uint32_t id,
                                      const struct spa_pod *param);
  static void on_process(void *_data);

  int32_t pipewire_fd_;
  uint32_t pipewire_id_;
  PipeWireSource *source;
  pw_loop* loop;
  pw_core* core;
  spa_hook registry_listener;
  pw_context* context_;
  pw_core* core_;
  pw_registry* registry;
  pw_properties* props_;
  spa_hook stream_listener_;
};
#endif // __PIPEWIRE_H__
