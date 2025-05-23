#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <pwd.h>

#include <glib.h>
#include <glib-unix.h>

#include <network/TcpSocket.h>
#include <network/UnixSocket.h>
#include <rfb/VNCServerST.h>
#include <rfb/ServerCore.h>
#include <rdr/FdOutStream.h>
#include <core/Logger_stdio.h>
#include <core/LogWriter.h>

#include "GSocketMonitor.h"
#include "RFBTimerSource.h"
#include "PortalDesktop.h"

static const char* defaultDesktopName();

core::IntParameter
  rfbport("rfbport",
          "TCP port to listen for RFB protocol", 5900, -1, 65535);
core::StringParameter
  rfbunixpath("rfbunixpath",
              "Unix socket to listen for RFB protocol", "");
core::IntParameter
  rfbunixmode("rfbunixmode",
              "Unix socket access mode", 0600, 0000, 0777);
core::BoolParameter
  localhostOnly("localhost",
                "Only allow connections from localhost", false);
core::StringParameter
  desktopName("desktop", "Name of VNC desktop", defaultDesktopName());
core::StringParameter
  interface("interface",
            "Listen on the specified network address", "all");

static const char* defaultDesktopName()
{
  long host_max = sysconf(_SC_HOST_NAME_MAX);
  if (host_max < 0)
    return "";

  std::vector<char> hostname(host_max + 1);
  if (gethostname(hostname.data(), hostname.size()) == -1)
    return "";

  struct passwd* pwent = getpwuid(getuid());
  if (pwent == nullptr)
    return "";

  int len = snprintf(nullptr, 0, "%s@%s", pwent->pw_name, hostname.data());
  if (len < 0)
    return "";

  char* name = new char[len + 1];

  snprintf(name, len + 1, "%s@%s", pwent->pw_name, hostname.data());

  return name;
}

static core::LogWriter vlog("w0vncserver");

char* programName;

static void printVersion(FILE *fp)
{
  fprintf(fp, "TigerVNC server version %s\n", PACKAGE_VERSION);
}

static void usage()
{
  printVersion(stderr);
  fprintf(stderr, "\nUsage: %s [<parameters>]\n", programName);
  fprintf(stderr, "       %s --version\n", programName);
  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  core::Configuration::listParams(79, 14);
  exit(1);
}

int CleanupSignalHandler(void* data)
{
  GMainLoop* loop;

  loop = (GMainLoop*)data;
  assert(loop);

  g_main_loop_quit(loop);
  return 1;
}

int main(int argc, char** argv)
{
  programName = argv[0];

  core::initStdIOLoggers();
  core::LogWriter::setLogParams("*:stderr:30");

  core::Configuration::removeParam("AcceptCutText");
  core::Configuration::removeParam("SendCutText");
  core::Configuration::removeParam("MaxCutText");

  for (int i = 1; i < argc;) {
    int ret;

    ret = core::Configuration::handleParamArg(argc, argv, i);
    if (ret > 0) {
      i += ret;
      continue;
    }

    if (strcmp(argv[i], "-h") == 0 ||
        strcmp(argv[i], "-help") == 0 ||
        strcmp(argv[i], "--help") == 0) {
      usage();
    }

    if (strcmp(argv[i], "-v") == 0 ||
        strcmp(argv[i], "-version") == 0 ||
        strcmp(argv[i], "--version") == 0) {
      printVersion(stderr);
      return 0;
    }

    if (argv[i][0] == '-') {
      fprintf(stderr, "%s: Unrecognized option '%s'\n",
              programName, argv[i]);
      fprintf(stderr, "See '%s --help' for more information.\n",
              programName);
      exit(1);
    }

    fprintf(stderr, "%s: Extra argument '%s'\n", programName, argv[i]);
    fprintf(stderr, "See '%s --help' for more information.\n",
            programName);
    exit(1);
  }

    GMainLoop* loop;
    std::list<network::SocketListener*> listeners;

    if (rfbunixpath.getValueStr()[0] != '\0') {
      listeners.push_back(new network::UnixListener(rfbunixpath, rfbunixmode));
      vlog.info("Listening on %s (mode %04o)", (const char*)rfbunixpath, (int)rfbunixmode);
    }

    if ((int)rfbport != -1) {
      std::list<network::SocketListener*> tcp_listeners;
      const char *addr = interface;

      if (strcasecmp(addr, "all") == 0)
        addr = nullptr;
      if (localhostOnly)
        createLocalTcpListeners(&tcp_listeners, (int)rfbport);
      else
        createTcpListeners(&tcp_listeners, addr, (int)rfbport);

      if (!tcp_listeners.empty()) {
        listeners.splice (listeners.end(), tcp_listeners);
        vlog.info("Listening for VNC connections on %s interface(s), port %d",
                  localhostOnly ? "local" : (const char*)interface,
                  (int)rfbport);
      }
    }

  loop = g_main_loop_new(nullptr, false);
  g_unix_signal_add(SIGINT, CleanupSignalHandler, loop);
  g_unix_signal_add(SIGTERM, CleanupSignalHandler, loop);
  g_unix_signal_add(SIGHUP, CleanupSignalHandler, loop);

  try {
    GSocketMonitor monitor(&listeners);
    RFBTimerSource timerSource;
    timerSource.attach(g_main_loop_get_context(loop));
    monitor.attach(g_main_loop_get_context(loop));

    if (PortalDesktop::available()) {
      PortalDesktop remote(loop);
      rfb::VNCServerST server(desktopName, &remote);
      monitor.listen(&server);
      g_main_loop_run(loop);
    } else {
      vlog.error("No remote desktop implementation found.");
      exit(-1);
    }

  } catch (std::exception& e) {
    vlog.error("main: %s", e.what());
    return 1;
  }
}
