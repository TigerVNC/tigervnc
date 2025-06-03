#ifndef __PORTAL_H__
#define __PORTAL_H__

#include <stdint.h>

#include <gio/gio.h>

// FIXME: Maybe Portal classes should be namespaced
class Portal {
public:
  Portal();
  virtual ~Portal();

protected:

  void call(GDBusProxy* proxy, const char* method, GVariant* parameters,
            GDBusCallFlags flags, GDBusSignalCallback signalCallback,
            const char* requestHandle, void* userData);

  void newRequestHandle(const char** requestHandle,
                        const char** handleToken);
  const char* newSessionHandle();

  static void handleSignalResponse(GDBusConnection *connection,
                                   const char *senderName,
                                   const char *objectPath,
                                   const char *interfaceName,
                                   const char *signalName,
                                   GVariant *parameters,
                                   void *userData);
  static void handleCallCb(GDBusProxy *source, GAsyncResult *res,
                       void *userData);
protected:
  GDBusConnection* connection_;
  char* sessionHandle_;
private:
  char* uniqueName_;
};
#endif
