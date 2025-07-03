#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <stdexcept>
#include <functional>

#include <gio/gio.h>

#include <rfb/ScreenSet.h>
#include <rfb/VNCServerST.h>
#include <core/LogWriter.h>

#include "portals/RemoteDesktop.h"
#include "PortalDesktop.h"
#include "GSocketMonitor.h"
#include "PipeWirePixelBuffer.h"

static core::LogWriter vlog("PortalDesktop");

PortalDesktop::PortalDesktop(GMainLoop* loop)
  : server_(nullptr), remoteDesktop_(nullptr), pipewire_(nullptr),
    loop_(loop)
{
}

PortalDesktop::~PortalDesktop()
{
  delete remoteDesktop_;
}

void PortalDesktop::init(rfb::VNCServer* vs)
{
  server_ = vs;
}

void PortalDesktop::start()
{
  assert(!remoteDesktop_);

  std::function<void(int, uint32_t)> cb = [this](int fd, uint32_t id) {
      pipewire_ = new PipeWirePixelBuffer(fd, id, server_);
  };

  remoteDesktop_ = new RemoteDesktop(server_, cb);
}

void PortalDesktop::stop()
{
  server_->setPixelBuffer(nullptr);

  delete pipewire_;
  pipewire_ = nullptr;

  delete remoteDesktop_;
  remoteDesktop_ = nullptr;

}

void PortalDesktop::queryConnection(network::Socket* sock,
                                    const char* userName)
{
  (void)sock;
  (void)userName;
  // FIXME: Implement this.
  server_->approveConnection(sock, false);
}

void PortalDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

unsigned int PortalDesktop::setScreenLayout(int fb_width, int fb_height,
                                       const rfb::ScreenSet& layout)
{
  (void) fb_width;
  (void) fb_height;
  (void) layout;
  vlog.debug("setScreenLayout");
  return rfb::resultProhibited;
}

void PortalDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  remoteDesktop_->keyEvent(keysym, keycode, down);
}

void PortalDesktop::pointerEvent(const core::Point& pos,
                            uint16_t buttonMask)
{
  remoteDesktop_->pointerEvent(pos.x, pos.y, buttonMask);
}

bool PortalDesktop::available()
{
  GError* error = nullptr;
  GDBusConnection* connection;
  GVariant* result;
  GDBusNodeInfo* nodeInfo;
  GDBusInterfaceInfo *interfaceInfo;
  const char* introspectionXml;
  bool interfaceMissing;
  std::vector<std::string> interfaces = {
    "org.freedesktop.portal.RemoteDesktop",
    "org.freedesktop.portal.ScreenCast",
  };

  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  result = g_dbus_connection_call_sync(connection,
                              "org.freedesktop.portal.Desktop",
                              "/org/freedesktop/portal/desktop",
                              "org.freedesktop.DBus.Introspectable",
                              "Introspect",
                              nullptr,
                              G_VARIANT_TYPE("(s)"),
                              G_DBUS_CALL_FLAGS_NONE,
                              3000,
                              nullptr,
                              &error);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error("Introspect call failed: " + error_message);
  }

  g_variant_get(result, "(&s)", &introspectionXml);
  g_object_unref(connection);

  nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
  g_variant_unref(result);

  if (error) {
    std::string error_message(error->message);
    g_error_free(error);
    throw std::runtime_error(error_message);
  }

  interfaceMissing = false;
  for (std::string interface : interfaces) {
    interfaceInfo = g_dbus_node_info_lookup_interface(nodeInfo,
                                                       interface.c_str());
    if (!interfaceInfo) {
      vlog.debug("Interface '%s' not found", interface.c_str());
      interfaceMissing = true;
    }
  }

  return !interfaceMissing;
}
