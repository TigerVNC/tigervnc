#ifndef __PIPEWIRE_SOURCE_H__
#define __PIPEWIRE_SOURCE_H__

#include <glib.h>
#include <pipewire/stream.h>

struct pw_loop;
struct pw_context;
struct pw_core;
struct _PipeWireSource;
class PipeWirePixelBuffer;

class PipeWireSource {
public:
  PipeWireSource(PipeWirePixelBuffer* pb);
  ~PipeWireSource();

  void start();

private:
  int sourceLoopDispatch(GSource* source, GSourceFunc callback,
                          void* userData);
  void handleStreamStateChanged(void* _data, enum pw_stream_state old,
                                enum pw_stream_state state,
                                const char* error);
  void handleStreamParamChanged(void* _data, uint32_t id,
                                const spa_pod* param);
  void handleProcess(void* _data);

private:
  pw_loop* loop;
  _PipeWireSource* source_;
  pw_context* context;
  pw_core* core_;
  pw_stream* stream;
  PipeWirePixelBuffer* pb_;
  static GSourceFuncs pipewireSourceFuncs;
  spa_hook streamListener;
  static const pw_stream_events streamEventsHandler;
};

#endif // __PIPEWIRE_SOURCE_H__