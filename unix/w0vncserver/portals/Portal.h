#ifndef __PORTAL_H__
#define __PORTAL_H__

#include <stdint.h>
#include "gio/gio.h"

// FIXME: Maybe Portal classes should be namespaced
class Portal {
public:
  Portal();
  virtual ~Portal();

  GDBusConnection* connection() { return connection_; }

protected:
  void new_request_handle(char** request_handle, char** handle_token);
  char* new_session_handle();

  void signal_subscribe(const char* path, GDBusSignalCallback cb,
                        void* user_data);
  static void on_signal_response(GDBusConnection *connection,
                                const char *sender_name,
                                const char *object_path,
                                const char *interface_name,
                                const char *signal_name,
                                GVariant *parameters,
                                void *user_data);
  static void on_call_cb(GDBusProxy *source, GAsyncResult *res,
                         void *user_data);

protected:
  GDBusConnection* connection_;
private:
  char* unique_name_;
};
#endif
