/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef WIN32
#include <core/winerrno.h>
#include <direct.h>
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

#include <core/Exception.h>
#include <core/Logger_stdio.h>
#include <core/LogWriter.h>
#include <core/Timer.h>

#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif

#include <core/xdgdirs.h>

#include <network/TcpSocket.h>

#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/x.H>

#include "fltk/theme.h"
#include "fltk/util.h"
#include "i18n.h"
#include "parameters.h"
#include "CConn.h"
#include "ServerDialog.h"
#include "UserDialog.h"
#include "touch.h"
#include "vncviewer.h"

#ifdef WIN32
#include "resource.h"
#include "win32.h"
#endif

static core::LogWriter vlog("main");

char vncServerName[VNCSERVERNAMELEN] = { '\0' };

static const char *argv0 = nullptr;

static bool inMainloop = false;
static bool exitMainloop = false;
static char *exitError = nullptr;
static bool fatalError = false;

static const char *about_text()
{
  static char buffer[1024];

  // This is used in multiple places with potentially different
  // encodings, so we need to make sure we get a fresh string every
  // time.
  snprintf(buffer, sizeof(buffer),
           _("TigerVNC v%s\n"
             "Built on: %s\n"
             "Copyright (C) 1999-%d TigerVNC team and many others (see README.rst)\n"
             "See https://www.tigervnc.org for information on TigerVNC."),
           PACKAGE_VERSION, BUILD_TIMESTAMP, 2025);

  return buffer;
}


void abort_vncviewer(const char *error, ...)
{
  fatalError = true;

  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if (exitError == nullptr) {
    va_list ap;

    va_start(ap, error);
    exitError = (char*)malloc(1024);
    vsnprintf(exitError, 1024, error, ap);
    va_end(ap);
  }

  if (inMainloop)
    exitMainloop = true;
  else {
    // We're early in the startup. Assume we can just exit().
    if (alertOnFatalError && (exitError != nullptr))
      fl_alert("%s", exitError);
    exit(EXIT_FAILURE);
  }
}

void abort_connection(const char *error, ...)
{
  assert(inMainloop);

  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if (exitError == nullptr) {
    va_list ap;

    va_start(ap, error);
    exitError = (char*)malloc(1024);
    vsnprintf(exitError, 1024, error, ap);
    va_end(ap);
  }

  exitMainloop = true;
}

void abort_connection_with_unexpected_error(const std::exception &e) {
  abort_connection(_("An unexpected error occurred when communicating "
                     "with the server:\n\n%s"), e.what());
}

void disconnect()
{
  exitMainloop = true;
}

bool should_disconnect()
{
  return exitMainloop;
}

void about_vncviewer()
{
  fl_message_title(_("About TigerVNC"));
  fl_message("%s", about_text());
}

static void mainloop(const char* vncserver, network::Socket* sock)
{
  while (true) {
    CConn *cc;

    exitMainloop = false;

    cc = new CConn(vncserver, sock);

    while (!exitMainloop) {
      int next_timer;

      next_timer = core::Timer::checkTimeouts();
      if (next_timer < 0)
        next_timer = INT_MAX;

      if (Fl::wait((double)next_timer / 1000.0) < 0.0) {
        vlog.error(_("Internal FLTK error. Exiting."));
        exit(-1);
      }
    }

    delete cc;

    if (fatalError) {
      assert(exitError != nullptr);
      if (alertOnFatalError)
        fl_alert("%s", exitError);
      break;
    }

    if (exitError == nullptr)
      break;

    if(reconnectOnError && (sock == nullptr)) {
      int ret;
      ret = fl_choice(_("%s\n\n"
                        "Attempt to reconnect?"),
                      nullptr, fl_yes, fl_no, exitError);
      free(exitError);
      exitError = nullptr;
      if (ret == 1)
        continue;
      else
        break;
    }

    if (alertOnFatalError)
      fl_alert("%s", exitError);

    break;
  }
}

#ifdef __APPLE__
static void about_callback(Fl_Widget* /*widget*/, void* /*data*/)
{
  about_vncviewer();
}

static void new_connection_cb(Fl_Widget* /*widget*/, void* /*data*/)
{
  const char *argv[2];
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    vlog.error(_("Error starting new connection: %s"), strerror(errno));
    return;
  }

  if (pid != 0)
    return;

  argv[0] = argv0;
  argv[1] = nullptr;

  execvp(argv[0], (char * const *)argv);

  vlog.error(_("Error starting new connection: %s"), strerror(errno));
  _exit(1);
}
#endif

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  vlog.info(_("Termination signal %d has been received. TigerVNC will now exit."), sig);
  exit(1);
}

static const char* getlocaledir()
{
#if defined(WIN32)
  static char localebuf[PATH_MAX];
  char *slash;

  GetModuleFileName(nullptr, localebuf, sizeof(localebuf));

  slash = strrchr(localebuf, '\\');
  if (slash == nullptr)
    return nullptr;

  *slash = '\0';

  if ((strlen(localebuf) + strlen("\\locale")) >= sizeof(localebuf))
    return nullptr;

  strcat(localebuf, "\\locale");

  return localebuf;
#elif defined(__APPLE__)
  CFBundleRef bundle;
  CFURLRef localeurl;
  CFStringRef localestr;
  Boolean ret;

  static char localebuf[PATH_MAX];

  bundle = CFBundleGetMainBundle();
  if (bundle == nullptr)
    return nullptr;

  localeurl = CFBundleCopyResourceURL(bundle, CFSTR("locale"),
                                      nullptr, nullptr);
  if (localeurl == nullptr)
    return nullptr;

  localestr = CFURLCopyFileSystemPath(localeurl, kCFURLPOSIXPathStyle);

  CFRelease(localeurl);

  ret = CFStringGetCString(localestr, localebuf, sizeof(localebuf),
                           kCFStringEncodingUTF8);
  if (!ret)
    return nullptr;

  return localebuf;
#else
  return CMAKE_INSTALL_FULL_LOCALEDIR;
#endif
}
static void init_fltk()
{
  // Adjust look of FLTK
  init_theme();

  // Proper Gnome Shell integration requires that we set a sensible
  // WM_CLASS for the window.
  Fl_Window::default_xclass("vncviewer");

  // Set the default icon for all windows.
#ifdef WIN32
  HICON lg, sm;

  lg = (HICON)LoadImage(GetModuleHandle(nullptr),
                        MAKEINTRESOURCE(IDI_ICON),
                        IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                        GetSystemMetrics(SM_CYICON),
                        LR_DEFAULTCOLOR | LR_SHARED);
  sm = (HICON)LoadImage(GetModuleHandle(nullptr),
                        MAKEINTRESOURCE(IDI_ICON),
                        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON),
                        LR_DEFAULTCOLOR | LR_SHARED);

  Fl_Window::default_icons(lg, sm);
#elif ! defined(__APPLE__)
  const int icon_sizes[] = {128, 64, 48, 32, 24, 22, 16};

  Fl_PNG_Image *icons[sizeof(icon_sizes)/sizeof(icon_sizes[0])];
  int count;

  count = 0;

  // FIXME: Follow icon theme specification
  for (int icon_size : icon_sizes) {
      char icon_path[PATH_MAX];
      bool exists;

      sprintf(icon_path, "%s/icons/hicolor/%dx%d/apps/tigervnc.png",
              CMAKE_INSTALL_FULL_DATADIR, icon_size, icon_size);

      struct stat st;
      if (stat(icon_path, &st) != 0)
        exists = false;
      else
        exists = true;

      if (exists) {
          icons[count] = new Fl_PNG_Image(icon_path);
          if (icons[count]->w() == 0 ||
              icons[count]->h() == 0 ||
              icons[count]->d() != 4) {
              delete icons[count];
              continue;
          }

          count++;
      }
  }

  Fl_Window::default_icons((const Fl_RGB_Image**)icons, count);

  for (int i = 0;i < count;i++)
      delete icons[i];
#endif

  // Turn off the annoying behaviour where popups track the mouse.
  fl_message_hotspot(false);

  // Avoid empty titles for popups
  fl_message_title_default("TigerVNC");

  // FLTK exposes these so that we can translate them.
  fl_no     = _("No");
  fl_yes    = _("Yes");
  fl_ok     = _("OK");
  fl_cancel = _("Cancel");
  fl_close  = _("Close");

#ifdef __APPLE__
  /* Needs trailing space */
  static char fltk_about[16];
  snprintf(fltk_about, sizeof(fltk_about), "%s ", _("About"));
  Fl_Mac_App_Menu::about = fltk_about;
  static char fltk_hide[16];
  snprintf(fltk_hide, sizeof(fltk_hide), "%s ", _("Hide"));
  Fl_Mac_App_Menu::hide = fltk_hide;
  static char fltk_quit[16];
  snprintf(fltk_quit, sizeof(fltk_quit), "%s ", _("Quit"));
  Fl_Mac_App_Menu::quit = fltk_quit;

  Fl_Mac_App_Menu::print = ""; // Don't want the print item
  Fl_Mac_App_Menu::services = _("Services");
  Fl_Mac_App_Menu::hide_others = _("Hide others");
  Fl_Mac_App_Menu::show = _("Show all");

  fl_mac_set_about(about_callback, nullptr);

  Fl_Sys_Menu_Bar *menubar;
  char buffer[1024];
  menubar = new Fl_Sys_Menu_Bar(0, 0, 500, 25);
  // Fl_Sys_Menu_Bar overrides methods without them being virtual,
  // which means we cannot use our generic Fl_Menu_ helpers.
  if (fltk_menu_escape(p_("SysMenu|", "&File"),
                       buffer, sizeof(buffer)) < sizeof(buffer))
      menubar->add(buffer, 0, nullptr, nullptr, FL_SUBMENU);
  if (fltk_menu_escape(p_("SysMenu|File|", "&New Connection"),
                       buffer, sizeof(buffer)) < sizeof(buffer))
      menubar->insert(1, buffer, FL_COMMAND | 'n', new_connection_cb);
#endif
}

static void usage(const char *programName)
{
#ifdef WIN32
  // If we don't have a console then we need to create one for output
  if (GetConsoleWindow() == nullptr) {
    HANDLE handle;
    int fd;

    AllocConsole();

    handle = GetStdHandle(STD_ERROR_HANDLE);
    fd = _open_osfhandle((intptr_t)handle, O_TEXT);
    *stderr = *fdopen(fd, "w");
  }
#endif

  fprintf(stderr, _(
          "\n"
          "Usage: %s [parameters] [host][:displayNum]\n"
          "       %s [parameters] [host][::port]\n"
#ifndef WIN32
          "       %s [parameters] [unix socket]\n"
#endif
          "       %s [parameters] -listen [port]\n"
          "       %s [parameters] [.tigervnc file]\n"),
          programName, programName,
#ifndef WIN32
          programName,
#endif
          programName, programName);

#if !defined(WIN32) && !defined(__APPLE__)
  fprintf(stderr, _("\n"
          "Options:\n\n"
          "  -display Xdisplay  - Specifies the X display for the viewer window\n"
          "  -geometry geometry - Initial position of the main TigerVNC window. See the\n"
          "                       man page for details.\n"));
#endif

  fprintf(stderr, _("\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n"));
  core::Configuration::listParams(79, 14);

#ifdef WIN32
  // Just wait for the user to kill the console window
  Sleep(INFINITE);
#endif

  exit(1);
}

static void
potentiallyLoadConfigurationFile(const char *filename)
{
  const bool hasPathSeparator = (strchr(filename, '/') != nullptr ||
                                 (strchr(filename, '\\')) != nullptr);

  if (hasPathSeparator) {
#ifndef WIN32
    struct stat sb;

    // This might be a UNIX socket, we need to check
    if (stat(filename, &sb) == -1) {
      // Some access problem; let loadViewerParameters() deal with it...
    } else {
      if ((sb.st_mode & S_IFMT) == S_IFSOCK)
        return;
    }
#endif

    try {
      const char* newServerName;
      newServerName = loadViewerParameters(filename);
      // This might be empty, but we still need to clear it so we
      // don't try to connect to the filename
      strncpy(vncServerName, newServerName, VNCSERVERNAMELEN-1);
      vncServerName[VNCSERVERNAMELEN-1] = '\0';
    } catch (std::exception& e) {
      vlog.error("%s", e.what());
      abort_vncviewer(_("Unable to load the specified configuration "
                        "file:\n\n%s"), e.what());
    }
  }
}

static void
migrateDeprecatedOptions()
{
  if (fullScreenAllMonitors) {
    vlog.info(_("FullScreenAllMonitors is deprecated, set FullScreenMode to 'all' instead"));

    fullScreenMode.setParam("all");
  }
  if (dotWhenNoCursor) {
    vlog.info(_("DotWhenNoCursor is deprecated, set AlwaysCursor to 1 and CursorType to 'Dot' instead"));

    alwaysCursor.setParam(true);
    cursorType.setParam("Dot");
  }
}

static void
create_base_dirs()
{
  const char *dir;

  dir = core::getvncconfigdir();
  if (dir == nullptr) {
    vlog.error(_("Could not determine VNC config directory path"));
    return;
  }

#ifndef WIN32
  const char *dotdir = strrchr(dir, '.');
  if (dotdir != nullptr && strcmp(dotdir, ".vnc") == 0)
    vlog.info(_("~/.vnc is deprecated, please consult 'man vncviewer' for paths to migrate to."));
#else
  const char *vncdir = strrchr(dir, '\\');
  if (vncdir != nullptr && strcmp(vncdir, "vnc") == 0)
    vlog.info(_("%%APPDATA%%\\vnc is deprecated, please switch to the %%APPDATA%%\\TigerVNC location."));
#endif

  if (core::mkdir_p(dir, 0755) == -1) {
    if (errno != EEXIST)
      vlog.error(_("Could not create VNC config directory \"%s\": %s"),
                 dir, strerror(errno));
  }

  dir = core::getvncdatadir();
  if (dir == nullptr) {
    vlog.error(_("Could not determine VNC data directory path"));
    return;
  }

  if (core::mkdir_p(dir, 0755) == -1) {
    if (errno != EEXIST)
      vlog.error(_("Could not create VNC data directory \"%s\": %s"),
                 dir, strerror(errno));
  }

  dir = core::getvncstatedir();
  if (dir == nullptr) {
    vlog.error(_("Could not determine VNC state directory path"));
    return;
  }

  if (core::mkdir_p(dir, 0755) == -1) {
    if (errno != EEXIST)
      vlog.error(_("Could not create VNC state directory \"%s\": %s"),
                 dir, strerror(errno));
  }
}

#ifndef WIN32
static void
createTunnel(const char *gatewayHost, const char *remoteHost,
             int remotePort, int localPort)
{
  const char *cmd = getenv("VNC_VIA_CMD");
  char *cmd2, *percent;
  char lport[10], rport[10];
  sprintf(lport, "%d", localPort);
  sprintf(rport, "%d", remotePort);
  setenv("G", gatewayHost, 1);
  setenv("H", remoteHost, 1);
  setenv("R", rport, 1);
  setenv("L", lport, 1);
  if (!cmd)
    cmd = "/usr/bin/ssh -f -L \"$L\":\"$H\":\"$R\" \"$G\" sleep 20";
  /* Compatibility with TigerVNC's method. */
  cmd2 = strdup(cmd);
  while ((percent = strchr(cmd2, '%')) != nullptr)
    *percent = '$';
  int res = system(cmd2);
  if (res != 0)
    fprintf(stderr, "Failed to create tunnel: '%s' returned %d\n", cmd2, res);
  free(cmd2);
}

static void mktunnel()
{
  const char *gatewayHost;
  std::string remoteHost;
  int localPort = network::findFreeTcpPort();
  int remotePort;

  network::getHostAndPort(vncServerName, &remoteHost, &remotePort);
  snprintf(vncServerName, VNCSERVERNAMELEN, "localhost::%d", localPort);
  vncServerName[VNCSERVERNAMELEN - 1] = '\0';
  gatewayHost = (const char*)via;
  createTunnel(gatewayHost, remoteHost.c_str(), remotePort, localPort);
}
#endif /* !WIN32 */

int main(int argc, char** argv)
{
  const char *localedir;

  argv0 = argv[0];

  setlocale(LC_ALL, "");

  localedir = getlocaledir();
  if (localedir == nullptr)
    fprintf(stderr, "Failed to determine locale directory\n");
  else
    bindtextdomain(PACKAGE_NAME, localedir);
  textdomain(PACKAGE_NAME);

  // Write about text to console, still using normal locale codeset
  fprintf(stderr,"\n%s\n", about_text());

  // Set gettext codeset to what our GUI toolkit uses. Since we are
  // passing strings from strerror/gai_strerror to the GUI, these must
  // be in GUI codeset as well.
  bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
  bind_textdomain_codeset("libc", "UTF-8");

  core::initStdIOLoggers();
#ifdef WIN32
  core::initFileLogger("C:\\temp\\vncviewer.log");
#else
  core::initFileLogger("/tmp/vncviewer.log");
#endif
  core::LogWriter::setLogParams("*:stderr:30");

#ifdef SIGHUP
  signal(SIGHUP, CleanupSignalHandler);
#endif
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  /* Load the default parameter settings */
  char defaultServerName[VNCSERVERNAMELEN] = "";
  try {
    const char* configServerName;
    configServerName = loadViewerParameters(nullptr);
    if (configServerName != nullptr) {
      strncpy(defaultServerName, configServerName, VNCSERVERNAMELEN-1);
      defaultServerName[VNCSERVERNAMELEN-1] = '\0';
    }
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
  }

  for (int i = 1; i < argc;) {
    int ret;

    ret = core::Configuration::handleParamArg(argc, argv, i);
    if (ret > 0) {
      i += ret;
      continue;
    }

    if (strcmp(argv[i], "-h") == 0 ||
        strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
    }

    if (strcmp(argv[i], "-v") == 0 ||
        strcmp(argv[i], "--version") == 0) {
      /* We already print version on every start */
      return 0;
    }

    if (argv[i][0] == '-') {
      fprintf(stderr, "\n");
      fprintf(stderr, _("%s: Unrecognized option '%s'\n"),
              argv[0], argv[i]);
      fprintf(stderr, _("See '%s --help' for more information.\n"),
              argv[0]);
      exit(1);
    }

    if (vncServerName[0] != '\0') {
      fprintf(stderr, "\n");
      fprintf(stderr, _("%s: Extra argument '%s'\n"), argv[0], argv[i]);
      fprintf(stderr, _("See '%s --help' for more information.\n"),
              argv[0]);
      exit(0);
    }

    strncpy(vncServerName, argv[i], VNCSERVERNAMELEN);
    vncServerName[VNCSERVERNAMELEN - 1] = '\0';
    i++;
  }

#if !defined(WIN32) && !defined(__APPLE__)
  if (strcmp(display, "") != 0) {
    Fl::display(display);
  }
  fl_open_display();
  XkbSetDetectableAutoRepeat(fl_display, True, nullptr);
#endif

  init_fltk();
  enable_touch();

  // Check if the server name in reality is a configuration file
  potentiallyLoadConfigurationFile(vncServerName);

  migrateDeprecatedOptions();

  create_base_dirs();

  network::Socket* sock = nullptr;

#ifndef WIN32
  /* Specifying -via and -listen together is nonsense */
  if (listenMode && strlen(via) > 0) {
    // TRANSLATORS: "Parameters" are command line arguments, or settings
    // from a file or the Windows registry.
    vlog.error(_("Parameters -listen and -via are incompatible"));
    abort_vncviewer(_("Parameters -listen and -via are incompatible"));
    return 1; /* Not reached */
  }
#endif

  if (listenMode) {
    std::list<network::SocketListener*> listeners;
    try {
      int port = 5500;
      if (isdigit(vncServerName[0]))
        port = atoi(vncServerName);

      createTcpListeners(&listeners, nullptr, port);
      if (listeners.empty())
        throw std::runtime_error(_("Unable to listen for incoming connections"));

      vlog.info(_("Listening on port %d"), port);

      /* Wait for a connection */
      while (sock == nullptr) {
        fd_set rfds;
        FD_ZERO(&rfds);
        for (network::SocketListener* listener : listeners)
          FD_SET(listener->getFd(), &rfds);

        int n = select(FD_SETSIZE, &rfds, nullptr, nullptr, nullptr);
        if (n < 0) {
          if (errno == EINTR) {
            vlog.debug("Interrupted select() system call");
            continue;
          } else {
            throw core::socket_error("select", errno);
          }
        }

        for (network::SocketListener* listener : listeners)
          if (FD_ISSET(listener->getFd(), &rfds)) {
            sock = listener->accept();
            if (sock)
              /* Got a connection */
              break;
          }
      }
    } catch (std::exception& e) {
      vlog.error("%s", e.what());
      abort_vncviewer(_("Failure waiting for incoming VNC connection:\n\n%s"), e.what());
      return 1; /* Not reached */
    }

    while (!listeners.empty()) {
      delete listeners.back();
      listeners.pop_back();
    }
  } else {
    if (vncServerName[0] == '\0') {
      ServerDialog::run(defaultServerName, vncServerName);
      if (vncServerName[0] == '\0')
        return 1;
    }

#ifndef WIN32
    if (strlen(via) > 0) {
      try {
        mktunnel();
      } catch (std::exception& e) {
        vlog.error("%s", e.what());
        abort_vncviewer(_("Failure setting up encrypted tunnel:\n\n%s"), e.what());
      }
    }
#endif
  }

  inMainloop = true;
  mainloop(vncServerName, sock);
  inMainloop = false;

  return 0;
}
