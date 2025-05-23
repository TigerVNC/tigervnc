#ifndef __G_SIGNAL_WRAPPER_H__
#define __G_SIGNAL_WRAPPER_H__

#include <stdint.h>

#include <functional>

#include <glib-object.h>

typedef struct _SignalWrapper {
    GObject parent_instance;
} SignalWrapper;

enum {
    SIGNAL_PIPEWIRE_OPEN,
    N_SIGNALS
};

class GSignalWrapper {
public:
  GSignalWrapper();
  ~GSignalWrapper();

  void emitSignal(uint32_t signal, void* data);
  void connect(uint32_t signal, std::function<void(void*)> callback);

private:
  SignalWrapper* signal_wrapper;
};

#endif // __G_SIGNAL_WRAPPER_H__
