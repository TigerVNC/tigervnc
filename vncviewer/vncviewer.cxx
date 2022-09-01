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
#include <os/winerrno.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

#include <rfb/Logger_stdio.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#include <rfb/LogWriter.h>
#include <rfb/Timer.h>
#include <rfb/Exception.h>
#include <rdr/Exception.h>
#include <network/TcpSocket.h>
#include <os/os.h>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/x.H>

#include "i18n.h"
#include "parameters.h"
#include "CConn.h"
#include "ServerDialog.h"
#include "UserDialog.h"
#include "touch.h"
#include "vncviewer.h"
#include "fltk_layout.h"

#ifdef WIN32
#include "resource.h"
#include "win32.h"
#endif

static rfb::LogWriter vlog("main");

using namespace network;
using namespace rfb;
using namespace std;

char vncServerName[VNCSERVERNAMELEN] = { '\0' };

static const char *argv0 = NULL;

static bool inMainloop = false;
static bool exitMainloop = false;
static char *exitError = NULL;
static bool fatalError = false;

static const char *about_text()
{
  static char buffer[1024];

  // This is used in multiple places with potentially different
  // encodings, so we need to make sure we get a fresh string every
  // time.
  snprintf(buffer, sizeof(buffer),
           _("TigerVNC Viewer v%s\n"
             "Built on: %s\n"
             "Copyright (C) 1999-%d TigerVNC Team and many others (see README.rst)\n"
             "See https://www.tigervnc.org for information on TigerVNC."),
           PACKAGE_VERSION, BUILD_TIMESTAMP, 2022);

  return buffer;
}


void abort_vncviewer(const char *error, ...)
{
  fatalError = true;

  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if (exitError == NULL) {
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
    if (alertOnFatalError && (exitError != NULL))
      fl_alert("%s", exitError);
    exit(EXIT_FAILURE);
  }
}

void abort_connection(const char *error, ...)
{
  assert(inMainloop);

  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if (exitError == NULL) {
    va_list ap;

    va_start(ap, error);
    exitError = (char*)malloc(1024);
    vsnprintf(exitError, 1024, error, ap);
    va_end(ap);
  }

  exitMainloop = true;
}

void abort_connection_with_unexpected_error(const rdr::Exception &e) {
  abort_connection(_("An unexpected error occurred when communicating "
                     "with the server:\n\n%s"), e.str());
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
  fl_message_title(_("About TigerVNC Viewer"));
  fl_message("%s", about_text());
}

static void mainloop(const char* vncserver, network::Socket* sock)
{
  while (true) {
    CConn *cc;

    exitMainloop = false;

    cc = new CConn(vncServerName, sock);

    while (!exitMainloop) {
      int next_timer;

      next_timer = Timer::checkTimeouts();
      if (next_timer == 0)
        next_timer = INT_MAX;

      if (Fl::wait((double)next_timer / 1000.0) < 0.0) {
        vlog.error(_("Internal FLTK error. Exiting."));
        exit(-1);
      }
    }

    delete cc;

    if (fatalError) {
      assert(exitError != NULL);
      if (alertOnFatalError)
        fl_alert("%s", exitError);
      break;
    }

    if (exitError == NULL)
      break;

    if(reconnectOnError && (sock == NULL)) {
      int ret;
      ret = fl_choice(_("%s\n\n"
                        "Attempt to reconnect?"),
                      NULL, fl_yes, fl_no, exitError);
      free(exitError);
      exitError = NULL;
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
static void about_callback(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}

static void new_connection_cb(Fl_Widget *widget, void *data)
{
  const char *argv[2];
  pid_t pid;

  pid = fork();
  if (pid == -1) {
    vlog.error(_("Error starting new TigerVNC Viewer: %s"), strerror(errno));
    return;
  }

  if (pid != 0)
    return;

  argv[0] = argv0;
  argv[1] = NULL;

  execvp(argv[0], (char * const *)argv);

  vlog.error(_("Error starting new TigerVNC Viewer: %s"), strerror(errno));
  _exit(1);
}
#endif

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  vlog.info(_("Termination signal %d has been received. TigerVNC Viewer will now exit."), sig);
  exit(1);
}

static const char* getlocaledir()
{
#if defined(WIN32)
  static char localebuf[PATH_MAX];
  char *slash;

  GetModuleFileName(NULL, localebuf, sizeof(localebuf));

  slash = strrchr(localebuf, '\\');
  if (slash == NULL)
    return NULL;

  *slash = '\0';

  if ((strlen(localebuf) + strlen("\\locale")) >= sizeof(localebuf))
    return NULL;

  strcat(localebuf, "\\locale");

  return localebuf;
#elif defined(__APPLE__)
  CFBundleRef bundle;
  CFURLRef localeurl;
  CFStringRef localestr;
  Boolean ret;

  static char localebuf[PATH_MAX];

  bundle = CFBundleGetMainBundle();
  if (bundle == NULL)
    return NULL;

  localeurl = CFBundleCopyResourceURL(bundle, CFSTR("locale"),
                                      NULL, NULL);
  if (localeurl == NULL)
    return NULL;

  localestr = CFURLCopyFileSystemPath(localeurl, kCFURLPOSIXPathStyle);

  CFRelease(localeurl);

  ret = CFStringGetCString(localestr, localebuf, sizeof(localebuf),
                           kCFStringEncodingUTF8);
  if (!ret)
    return NULL;

  return localebuf;
#else
  return CMAKE_INSTALL_FULL_LOCALEDIR;
#endif
}
static void init_fltk()
{
  // Basic text size (10pt @ 96 dpi => 13px)
  FL_NORMAL_SIZE = 13;

  // Select a FLTK scheme and background color that looks somewhat
  // close to modern systems
  Fl::scheme("gtk+");
  Fl::background(220, 220, 220);

  // macOS has a slightly brighter default background though
#ifdef __APPLE__
  Fl::background(240, 240, 240);
#endif

  // Proper Gnome Shell integration requires that we set a sensible
  // WM_CLASS for the window.
  Fl_Window::default_xclass("vncviewer");

  // Set the default icon for all windows.
#ifdef WIN32
  HICON lg, sm;

  lg = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON),
                        IMAGE_ICON, GetSystemMetrics(SM_CXICON),
                        GetSystemMetrics(SM_CYICON),
                        LR_DEFAULTCOLOR | LR_SHARED);
  sm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON),
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
  for (size_t i = 0;i < sizeof(icon_sizes)/sizeof(icon_sizes[0]);i++) {
      char icon_path[PATH_MAX];
      bool exists;

      sprintf(icon_path, "%s/icons/hicolor/%dx%d/apps/tigervnc.png",
              CMAKE_INSTALL_FULL_DATADIR, icon_sizes[i], icon_sizes[i]);

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

  // This makes the "icon" in dialogs rounded, which fits better
  // with the above schemes.
  fl_message_icon()->box(FL_UP_BOX);

  // Turn off the annoying behaviour where popups track the mouse.
  fl_message_hotspot(false);

  // Avoid empty titles for popups
  fl_message_title_default(_("TigerVNC Viewer"));

#ifdef WIN32
  // Most "normal" Windows apps use this font for UI elements.
  Fl::set_font(FL_HELVETICA, "Tahoma");
#endif

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
  Fl_Mac_App_Menu::hide_others = _("Hide Others");
  Fl_Mac_App_Menu::show = _("Show All");

  fl_mac_set_about(about_callback, NULL);

  Fl_Sys_Menu_Bar *menubar;
  char buffer[1024];
  menubar = new Fl_Sys_Menu_Bar(0, 0, 500, 25);
  // Fl_Sys_Menu_Bar overrides methods without them being virtual,
  // which means we cannot use our generic Fl_Menu_ helpers.
  if (fltk_menu_escape(p_("SysMenu|", "&File"),
                       buffer, sizeof(buffer)) < sizeof(buffer))
      menubar->add(buffer, 0, 0, 0, FL_SUBMENU);
  if (fltk_menu_escape(p_("SysMenu|File|", "&New Connection"),
                       buffer, sizeof(buffer)) < sizeof(buffer))
      menubar->insert(1, buffer, FL_COMMAND | 'n', new_connection_cb);
#endif
}

static void mkvnchomedir()
{
  // Create .vnc in the user's home directory if it doesn't already exist
  char* homeDir = NULL;

  if (getvnchomedir(&homeDir) == -1) {
    vlog.error(_("Could not obtain the home directory path"));
  } else {
    int result = mkdir(homeDir, 0755);
    if (result == -1 && errno != EEXIST)
      vlog.error(_("Could not create VNC home directory: %s"), strerror(errno));
    delete [] homeDir;
  }
}

static void usage(const char *programName)
{
#ifdef WIN32
  // If we don't have a console then we need to create one for output
  if (GetConsoleWindow() == NULL) {
    HANDLE handle;
    int fd;

    AllocConsole();

    handle = GetStdHandle(STD_ERROR_HANDLE);
    fd = _open_osfhandle((intptr_t)handle, O_TEXT);
    *stderr = *fdopen(fd, "w");
  }
#endif

  fprintf(stderr,
          "\n"
          "usage: %s [parameters] [host][:displayNum]\n"
          "       %s [parameters] [host][::port]\n"
#ifndef WIN32
          "       %s [parameters] [unix socket]\n"
#endif
          "       %s [parameters] -listen [port]\n"
          "       %s [parameters] [.tigervnc file]\n",
          programName, programName,
#ifndef WIN32
          programName,
#endif
          programName, programName);

#if !defined(WIN32) && !defined(__APPLE__)
  fprintf(stderr,"\n"
          "Options:\n\n"
          "  -display Xdisplay  - Specifies the X display for the viewer window\n"
          "  -geometry geometry - Initial position of the main VNC viewer window. See the\n"
          "                       man page for details.\n");
#endif

  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  Configuration::listParams(79, 14);

#ifdef WIN32
  // Just wait for the user to kill the console window
  Sleep(INFINITE);
#endif

  exit(1);
}

static void
potentiallyLoadConfigurationFile(char *vncServerName)
{
  const bool hasPathSeparator = (strchr(vncServerName, '/') != NULL ||
                                 (strchr(vncServerName, '\\')) != NULL);

  if (hasPathSeparator) {
#ifndef WIN32
    struct stat sb;

    // This might be a UNIX socket, we need to check
    if (stat(vncServerName, &sb) == -1) {
      // Some access problem; let loadViewerParameters() deal with it...
    } else {
      if ((sb.st_mode & S_IFMT) == S_IFSOCK)
        return;
    }
#endif

    try {
      const char* newServerName;
      newServerName = loadViewerParameters(vncServerName);
      // This might be empty, but we still need to clear it so we
      // don't try to connect to the filename
      strncpy(vncServerName, newServerName, VNCSERVERNAMELEN-1);
      vncServerName[VNCSERVERNAMELEN-1] = '\0';
    } catch (rfb::Exception& e) {
      vlog.error("%s", e.str());
      abort_vncviewer(_("Unable to load the specified configuration "
                        "file:\n\n%s"), e.str());
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
}

#ifndef WIN32
static int
interpretViaParam(char *remoteHost, int *remotePort, int localPort)
{
  const int SERVER_PORT_OFFSET = 5900;
  char *pos = strchr(vncServerName, ':');
  if (pos == NULL)
    *remotePort = SERVER_PORT_OFFSET;
  else {
    int portOffset = SERVER_PORT_OFFSET;
    size_t len;
    *pos++ = '\0';
    len = strlen(pos);
    if (*pos == ':') {
      /* Two colons is an absolute port number, not an offset. */
      pos++;
      len--;
      portOffset = 0;
    }
    if (!len || strspn (pos, "-0123456789") != len )
      return 1;
    *remotePort = atoi(pos) + portOffset;
  }

  if (*vncServerName != '\0')
    strcpy(remoteHost, vncServerName);
  else
    strcpy(remoteHost, "localhost");

  snprintf(vncServerName, VNCSERVERNAMELEN, "localhost::%d", localPort);
  vncServerName[VNCSERVERNAMELEN - 1] = '\0';

  return 0;
}

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
  while ((percent = strchr(cmd2, '%')) != NULL)
    *percent = '$';
  system(cmd2);
  free(cmd2);
}

static int mktunnel()
{
  const char *gatewayHost;
  char remoteHost[VNCSERVERNAMELEN];
  int localPort = findFreeTcpPort();
  int remotePort;

  if (interpretViaParam(remoteHost, &remotePort, localPort) != 0)
    return 1;
  gatewayHost = (const char*)via;
  createTunnel(gatewayHost, remoteHost, remotePort, localPort);

  return 0;
}
#endif /* !WIN32 */

int main(int argc, char** argv)
{
  const char *localedir;
  UserDialog dlg;

  argv0 = argv[0];

  setlocale(LC_ALL, "");

  localedir = getlocaledir();
  if (localedir == NULL)
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

  rfb::initStdIOLoggers();
#ifdef WIN32
  rfb::initFileLogger("C:\\temp\\vncviewer.log");
#else
  rfb::initFileLogger("/tmp/vncviewer.log");
#endif
  rfb::LogWriter::setLogParams("*:stderr:30");

#ifdef SIGHUP
  signal(SIGHUP, CleanupSignalHandler);
#endif
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  Configuration::enableViewerParams();

  /* Load the default parameter settings */
  char defaultServerName[VNCSERVERNAMELEN] = "";
  try {
    const char* configServerName;
    configServerName = loadViewerParameters(NULL);
    if (configServerName != NULL) {
      strncpy(defaultServerName, configServerName, VNCSERVERNAMELEN-1);
      defaultServerName[VNCSERVERNAMELEN-1] = '\0';
    }
  } catch (rfb::Exception& e) {
    vlog.error("%s", e.str());
  }

  for (int i = 1; i < argc;) {
    /* We need to resolve an ambiguity for booleans */
    if (argv[i][0] == '-' && i+1 < argc) {
        VoidParameter *param;

        param = Configuration::getParam(&argv[i][1]);
        if ((param != NULL) &&
            (dynamic_cast<BoolParameter*>(param) != NULL)) {
          if ((strcasecmp(argv[i+1], "0") == 0) ||
              (strcasecmp(argv[i+1], "1") == 0) ||
              (strcasecmp(argv[i+1], "true") == 0) ||
              (strcasecmp(argv[i+1], "false") == 0) ||
              (strcasecmp(argv[i+1], "yes") == 0) ||
              (strcasecmp(argv[i+1], "no") == 0)) {
              param->setParam(argv[i+1]);
              i += 2;
              continue;
          }
      }
    }

    if (Configuration::setParam(argv[i])) {
      i++;
      continue;
    }

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (Configuration::setParam(&argv[i][1], argv[i+1])) {
          i += 2;
          continue;
        }
      }

      usage(argv[0]);
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
  XkbSetDetectableAutoRepeat(fl_display, True, NULL);
#endif

  init_fltk();
  enable_touch();

  // Check if the server name in reality is a configuration file
  potentiallyLoadConfigurationFile(vncServerName);

  migrateDeprecatedOptions();

  mkvnchomedir();

  CSecurity::upg = &dlg;
#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  CSecurity::msg = &dlg;
#endif

  Socket *sock = NULL;

#ifndef WIN32
  /* Specifying -via and -listen together is nonsense */
  if (listenMode && strlen(via.getValueStr()) > 0) {
    // TRANSLATORS: "Parameters" are command line arguments, or settings
    // from a file or the Windows registry.
    vlog.error(_("Parameters -listen and -via are incompatible"));
    abort_vncviewer(_("Parameters -listen and -via are incompatible"));
    return 1; /* Not reached */
  }
#endif

  if (listenMode) {
    std::list<SocketListener*> listeners;
    try {
      int port = 5500;
      if (isdigit(vncServerName[0]))
        port = atoi(vncServerName);

      createTcpListeners(&listeners, 0, port);

      vlog.info(_("Listening on port %d"), port);

      /* Wait for a connection */
      while (sock == NULL) {
        fd_set rfds;
        FD_ZERO(&rfds);
        for (std::list<SocketListener*>::iterator i = listeners.begin();
             i != listeners.end();
             i++)
          FD_SET((*i)->getFd(), &rfds);

        int n = select(FD_SETSIZE, &rfds, 0, 0, 0);
        if (n < 0) {
          if (errno == EINTR) {
            vlog.debug("Interrupted select() system call");
            continue;
          } else {
            throw rdr::SystemException("select", errno);
          }
        }

        for (std::list<SocketListener*>::iterator i = listeners.begin ();
             i != listeners.end();
             i++)
          if (FD_ISSET((*i)->getFd(), &rfds)) {
            sock = (*i)->accept();
            if (sock)
              /* Got a connection */
              break;
          }
      }
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      abort_vncviewer(_("Failure waiting for incoming VNC connection:\n\n%s"), e.str());
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
    if (strlen (via.getValueStr()) > 0 && mktunnel() != 0)
      usage(argv[0]);
#endif
  }

  inMainloop = true;
  mainloop(vncServerName, sock);
  inMainloop = false;

  return 0;
}
