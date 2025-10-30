/* Copyright 2025 Adam Halim for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include <glib.h>
#include <glib-unix.h>
#include <gio/gio.h>

#include <network/TcpSocket.h>
#include <network/UnixSocket.h>
#include <rfb/VNCServerST.h>
#include <rfb/ServerCore.h>
#include <rdr/FdOutStream.h>
#include <core/Logger_stdio.h>
#include <core/LogWriter.h>

#include "parameters.h"
#include "GSocketSource.h"
#include "RFBTimerSource.h"
#include "portals/PortalDesktop.h"
#include "wayland/WaylandDesktop.h"

static core::LogWriter vlog("main");

char* programName;
static GMainLoop* loop = nullptr;
static GDBusConnection* connection = nullptr;
static bool fatalError = false;

void fatal_error(const char *error, ...) {
  char exitError[1024];

  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if (fatalError)
    return;

  fatalError = true;

  va_list ap;
  va_start(ap, error);
  vsnprintf(exitError, sizeof(exitError), error, ap);
  va_end(ap);

  vlog.error("%s", exitError);

  assert(loop);
  g_main_loop_quit(loop);
}

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

int CleanupSignalHandler(void*)
{
  g_main_loop_quit(loop);
  return 1;
}

int main(int argc, char** argv)
{
  std::list<network::SocketListener*> listeners;
  int32_t sigintTag;
  int32_t sigtermTag;
  int32_t sighupTag;
  RFBTimerSource* timerSource;
  GSocketSource* monitor;
  rfb::SDesktop* desktop;
  rfb::VNCServerST* server;

  programName = argv[0];

  core::initStdIOLoggers();
  core::LogWriter::setLogParams("*:stderr:30");

  core::Configuration::removeParam("AcceptCutText");
  core::Configuration::removeParam("AcceptSetDesktopSize");
  core::Configuration::removeParam("SendCutText");
  core::Configuration::removeParam("MaxCutText");
  core::Configuration::removeParam("SendPrimary");

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
  } catch (std::exception& e) {
    fprintf(stderr, "%s\n", e.what());
    return -1;
  }

  loop = g_main_loop_new(nullptr, false);
  sigintTag = g_unix_signal_add(SIGINT, CleanupSignalHandler, nullptr);
  sigtermTag = g_unix_signal_add(SIGTERM, CleanupSignalHandler, nullptr);
  sighupTag = g_unix_signal_add(SIGHUP, CleanupSignalHandler, nullptr);

  try {
    if (PortalDesktop::available()) {
      desktop = new PortalDesktop();
    } else if (WaylandDesktop::available()) {
      desktop = new WaylandDesktop(loop);
    } else {
      fprintf(stderr, "No remote desktop implementation found.\n");
      return -1;
    }
    server = new rfb::VNCServerST(desktopName, desktop);
    timerSource = new RFBTimerSource();
    monitor = new GSocketSource(server, &listeners);
  } catch (std::exception& e) {
    vlog.error("main: %s", e.what());
    return 1;
  }

  monitor->attach(g_main_loop_get_context(loop));
  timerSource->attach(g_main_loop_get_context(loop));
  monitor->listen();
  g_main_loop_run(loop);

  g_source_remove(sigintTag);
  g_source_remove(sigtermTag);
  g_source_remove(sighupTag);

  delete monitor;
  delete timerSource;
  delete server;
  delete desktop;

  for (network::SocketListener* listener : listeners)
    delete listener;

  // The GDBusConnection is a singleton that we want to close once
  // before we exit.
  connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (connection)
    g_dbus_connection_close_sync(connection, nullptr, nullptr);

  g_main_loop_unref(loop);
}
