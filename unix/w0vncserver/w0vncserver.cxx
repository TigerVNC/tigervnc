#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>
#include <glib-unix.h>

#include <network/TcpSocket.h>
#include <rfb/VNCServerST.h>
#include <rfb/ServerCore.h>
#include <rdr/FdOutStream.h>
#include <core/Logger_stdio.h>
#include <core/LogWriter.h>

#include "PortalBackend.h"


core::IntParameter
  rfbport("rfbport",
          "TCP port to listen for RFB protocol", 5900, -1, 65535);


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

int main(int argc, char** argv)
{
  programName = argv[0];

  core::initStdIOLoggers();
  core::LogWriter::setLogParams("*:stderr:30");

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

  try {
    std::vector<std::string> interfaces;
    interfaces.push_back("org.freedesktop.portal.RemoteDesktop");
    interfaces.push_back("org.freedesktop.portal.ScreenCast");

    if (!Portal::check_interfaces(interfaces)) {
      fprintf(stderr, "No RemoteDesktop portal implementation found\n");
      exit(-1);
    }

    PortalBackend remote{rfbport};
    remote.init();

  } catch (std::exception& e) {
    vlog.error("main: %s", e.what());
    return 1;
  }
}
