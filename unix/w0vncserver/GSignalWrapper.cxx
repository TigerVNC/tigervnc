
#include <assert.h>

#include <array>

#include <glib-object.h>

#include "GSignalWrapper.h"

G_DECLARE_FINAL_TYPE(SignalWrapper, signal_wrapper, C, SIGNAL_WRAPPER,
                     GObject);

static std::array<std::pair<int, const char*>, N_SIGNALS> signal_info =
  {{
    {SIGNAL_PIPEWIRE_OPEN, "pipewire-open"},
  }};

static uint32_t signal_ids[N_SIGNALS];

G_DEFINE_FINAL_TYPE(SignalWrapper, signal_wrapper, G_TYPE_OBJECT);

static void signal_wrapper_class_init(SignalWrapperClass* klass)
{
  signal_ids[SIGNAL_PIPEWIRE_OPEN] = g_signal_new(
      signal_info[SIGNAL_PIPEWIRE_OPEN].second,
      G_TYPE_FROM_CLASS(klass),
      G_SIGNAL_RUN_FIRST,
      0, nullptr, nullptr, nullptr,
      G_TYPE_NONE,
      1,
      G_TYPE_POINTER
  );
}

static void signal_wrapper_init(SignalWrapper* /* self */)
{
}


GSignalWrapper::GSignalWrapper() : signal_wrapper(nullptr)
{
  signal_wrapper = static_cast<SignalWrapper*>(
    g_object_new(signal_wrapper_get_type(), nullptr));
}

GSignalWrapper::~GSignalWrapper()
{
  g_object_unref(signal_wrapper);
}

void GSignalWrapper::emitSignal(uint32_t signal, void* data)
{
  assert(signal < N_SIGNALS);

  g_signal_emit(signal_wrapper, signal_ids[signal], 0, data);
}

void GSignalWrapper::connect(uint32_t signal,
                             std::function<void(void*)> callback)
{
  assert(signal < N_SIGNALS);

  std::function<void(void*)>* cbPtr =
    new std::function<void(void*)>(std::move(callback));

  g_signal_connect_data(
    signal_wrapper, signal_info[signal].second,
    G_CALLBACK(+[](SignalWrapper*, void* args, void* userData) {
      std::function<void(void*)>* cb =
        static_cast<std::function<void(void*)>*>(userData);

      (*cb)(args);
    }),
    cbPtr,
    [](void* data, GClosure*) {
      delete static_cast<std::function<void()>*>(data);
    },
    G_CONNECT_DEFAULT);
}