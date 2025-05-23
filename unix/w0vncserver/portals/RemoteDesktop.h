#ifndef __REMOTE_DESKTOP_H__
#define __REMOTE_DESKTOP_H__

#include <stdint.h>

#include <vector>
#include <functional>

#include <gio/gio.h>

#include "Portal.h"

// Maximum number of buttons
#define BUTTONS 9

// evdev input event codes
#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

// Source types
static const int SRC_MONITOR = (1 << 0);
static const int SRC_WINDOW =  (1 << 1);
static const int SRC_VIRTUAL = (1 << 2);

// Cursor modes
static const int CUR_HIDEN =    (1 << 0);
static const int CUR_EMBEDDED = (1 << 1);
static const int CUR_METADATA = (1 << 2);

struct PipeWireStreamData {
  const char* id;
  uint32_t pwNodeID;
  uint32_t width;
  uint32_t height;
  uint32_t x;
  uint32_t y;
  uint32_t sourceType;
  const char* mappingId;
};

namespace rfb { class VNCServer; }

class RemoteDesktop : public Portal {
public:
  RemoteDesktop(
    rfb::VNCServer* server,
    std::function<void(int fd, uint32_t nodeId)> startPipewireCb);
  ~RemoteDesktop();

  void start();
  void createSession();
  void selectSources();
  void openPipewireRemote();

  void startPipewire(int fd, uint32_t nodeId);
  void parseStreams(GVariant* streams);

  void setSessionHandle(char* handle);

  char* sessionHandle() { return sessionHandle_; }
  std::vector<PipeWireStreamData*> pipewireStreams() { return streams_; }

private:
  PipeWireStreamData* parseStream(GVariant* stream);

private:
  bool pipewireStarted;
  rfb::VNCServer* server_;

  std::vector<PipeWireStreamData*> streams_;

  GDBusProxy *screencastProxy_;

  std::function<void(int fd, uint32_t nodeId)> startPipewireCb_;
};

#endif
