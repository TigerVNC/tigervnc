#ifndef __SCREENCAST_H__
#define __SCREENCAST_H__

#include "Portal.h"

#include "gio/gio.h"
#include "glib.h"
#include <stdint.h>

// Source types
static const int SRC_MONITOR = (1 << 0);
static const int SRC_WINDOW =  (1 << 1);
static const int SRC_VIRTUAL = (1 << 2);

// Cursor modes
static const int CUR_HIDEN =    (1 << 0);
static const int CUR_EMBEDDED = (1 << 1);
static const int CUR_METADATA = (1 << 2);

class PortalBackend;

class Screencast : public Portal
{
public:
  // Re-use the same connection that RemoteDesktop is using
  Screencast(GDBusConnection* connection, PortalBackend* parent);
  ~Screencast();

  void select_sources();
  void open_pipewire_remote();
private:
  static void on_dbus_get(GObject *source, GAsyncResult *res,
                          void *user_data);
  static void on_screencast_proxy_response(GObject *source,
                                           GAsyncResult *res,
                                           void *user_data);
  static void on_call_response(GDBusProxy *source, GAsyncResult *res,
                               void *user_data);

  static void on_create_session(GDBusConnection *connection,
                         const char *sender,
                         const char *object_path,
                         const char *interface_name,
                         const char *signal_name, GVariant *parameters,
                         void *user_data);
  static void on_select_sources(GDBusConnection *connection,
                                const char *sender,
                                const char *object_path,
                                const char *interface_name,
                                const char *signal_name,
                                GVariant *parameters,
                                void *session_handle);
  static void on_session_start(GDBusConnection *connection,
                              const char *sender_name,
                              const char *object_path,
                              const char *interface_name,
                              const char *signal_name,
                              GVariant *parameters,
                              void *session_handle);
  static void on_open_pipewire_remote(GDBusProxy *proxy,
                                      GAsyncResult *res,
                                      void *user_data);
  static int on_pipewire_event(GIOChannel *source,
                               GIOCondition condition, void* data);
  void create_session();
private:
  GDBusProxy* proxy_;
  PortalBackend* parent_;

};
#endif

void select_sources(GDBusProxy* proxy, char* session_handle);