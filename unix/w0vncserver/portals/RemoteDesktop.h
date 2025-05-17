#ifndef __REMOTE_DESKTOP_H__
#define __REMOTE_DESKTOP_H__

#include "Portal.h"
#include "gio/gio.h"
#include <vector>

// evdev input event codes
#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112
#define BTN_SIDE    0x113
#define BTN_EXTRA   0x114

// Maximum number of buttons
#define BUTTONS 9

// Scroll wheel events
#define WHEEL_VERTICAL_DOWN    3
#define WHEEL_VERTICAL_UP      4
#define WHEEL_HORIZONTAL_LEFT  5
#define WHEEL_HORIZONTAL_RIGHT 6

// Devices types
static const int DEV_KEYBOARD =    (1 << 0);
static const int DEV_POINTER =     (1 << 1);
static const int DEV_TOUCHSCREEN = (1 << 2);


typedef struct rd_stream_ {
  const char* id;
  uint32_t pw_node_id;
  uint32_t width;
  uint32_t height;
  uint32_t x;
  uint32_t y;
  uint32_t source_type;
  const char* mapping_id;
} rd_stream;

class PortalBackend;

class RemoteDesktop : public Portal {
public:
  RemoteDesktop(PortalBackend* remote);
  ~RemoteDesktop();

  void init();

  void keyEvent(uint32_t keysym, uint32_t keycode, bool down);
  void pointerEvent(int32_t x, int32_t y, uint16_t buttonMask);

  void start();


  std::vector<rd_stream*> streams() { return streams_; }
  void create_session();
  void select_devices();
private:
  static void on_remote_desktop_proxy_response(GObject *source,
                                               GAsyncResult *res,
                                               void *user_data);
  static void on_create_session(GDBusConnection *connection,
                                const char *sender,
                                const char *object_path,
                                const char *interface_name,
                                const char *signal_name,
                                GVariant *parameters,
                                  void *user_data);
  static void on_select_devices(GDBusConnection *connection,
                                const char *sender,
                                const char *object_path,
                                const char *interface_name,
                                const char *signal_name,
                                GVariant *parameters,
                                void *user_data);
  static void on_start(GDBusConnection *connection,
                       const char *sender,
                       const char *object_path,
                       const char *interface_name,
                       const char *signal_name,
                       GVariant *parameters,
                       void *user_data);

  void parse_streams(GVariant* streams);
  rd_stream* parse_stream(GVariant* stream);

  void handle_pointerEvent(uint16_t buttonMask);
  void handle_button_press(uint16_t buttonMask, int32_t button);
  void handle_scroll_wheel(int32_t button);

private:
  bool clipboardEnabled;
  uint16_t oldButtonMask;

  std::vector<rd_stream*> streams_;

  GDBusProxy *proxy_;

  PortalBackend* parent_;

};

#endif
