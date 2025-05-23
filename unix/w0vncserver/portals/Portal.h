#ifndef __PORTAL_H__
#define __PORTAL_H__

#include <stdint.h>

#include <gio/gio.h>

// FIXME: Maybe Portal classes should be namespaced
class Portal {
public:
  Portal();
  virtual ~Portal();

  GDBusConnection* connection() { return connection_; }
  void signalSubscribe(const char* path, GDBusSignalCallback cb,
                       void* userData);

protected:
  void newRequestHandle(char** requestHandle, char** handleToken);
  char* newSessionHandle();

  static void onSignalResponse(GDBusConnection *connection,
                                 const char *senderName,
                                 const char *objectPath,
                                 const char *interfaceName,
                                 const char *signalName,
                                 GVariant *parameters,
                                 void *userData);
  static void onCallCb(GDBusProxy *source, GAsyncResult *res,
                       void *userData);

protected:
  GDBusConnection* connection_;
  char* sessionHandle_;
private:
  char* uniqueName_;
};
#endif
