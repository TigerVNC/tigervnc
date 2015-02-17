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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef WIN32
#include <os/winerrno.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#endif

#include <rfb/Logger_stdio.h>
#include <rfb/SecurityClient.h>
#include <rfb/Security.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#include <rfb/LogWriter.h>
#include <rfb/Timer.h>
#include <rfb/Exception.h>
#include <network/TcpSocket.h>
#include <os/os.h>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/fl_ask.H>
#include <FL/x.H>

#include "i18n.h"
#include "parameters.h"
#include "CConn.h"
#include "ServerDialog.h"
#include "UserDialog.h"
#include "vncviewer.h"

#ifdef WIN32
#include "resource.h"
#include "win32.h"
#endif

rfb::LogWriter vlog("main");

using namespace network;
using namespace rfb;
using namespace std;

static const char _aboutText[] = N_("TigerVNC Viewer %d-bit v%s\n"
                                    "Built on: %s\n"
                                    "Copyright (C) 1999-%d TigerVNC Team and many others (see README.txt)\n"
                                    "See http://www.tigervnc.org for information on TigerVNC.");
static char aboutText[1024];

char vncServerName[VNCSERVERNAMELEN] = { '\0' };

static bool exitMainloop = false;
static const char *exitError = NULL;

void exit_vncviewer(const char *error)
{
  // Prioritise the first error we get as that is probably the most
  // relevant one.
  if ((error != NULL) && (exitError == NULL))
    exitError = strdup(error);

  exitMainloop = true;
}

void about_vncviewer()
{
  fl_message_title(_("About TigerVNC Viewer"));
  fl_message("%s", aboutText);
}

static void about_callback(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  vlog.info(_("Termination signal %d has been received. TigerVNC Viewer will now exit."), sig);
  exit(1);
}

static void init_fltk()
{
  // Basic text size (10pt @ 96 dpi => 13px)
  FL_NORMAL_SIZE = 13;

#ifndef __APPLE__
  // Select a FLTK scheme and background color that looks somewhat
  // close to modern Linux and Windows.
  Fl::scheme("gtk+");
  Fl::background(220, 220, 220);
#else
  // On Mac OS X there is another scheme that fits better though.
  Fl::scheme("plastic");
#endif

  // Proper Gnome Shell integration requires that we set a sensible
  // WM_CLASS for the window.
  Fl_Window::default_xclass("vncviewer");

  // Set the default icon for all windows.
#ifdef HAVE_FLTK_ICONS
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
  const int icon_sizes[] = {48, 32, 24, 16};

  Fl_PNG_Image *icons[4];
  int count;

  count = 0;

  // FIXME: Follow icon theme specification
  for (size_t i = 0;i < sizeof(icon_sizes)/sizeof(icon_sizes[0]);i++) {
      char icon_path[PATH_MAX];
      bool exists;

      sprintf(icon_path, "%s/icons/hicolor/%dx%d/apps/tigervnc.png",
              DATA_DIR, icon_sizes[i], icon_sizes[i]);

#ifndef WIN32
      struct stat st;
      if (stat(icon_path, &st) != 0)
#else
      struct _stat st;
      if (_stat(icon_path, &st) != 0)
          return(false);
#endif
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
#endif // FLTK_HAVE_ICONS

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
#endif
}

static void mkvnchomedir()
{
  // Create .vnc in the user's home directory if it doesn't already exist
  char* homeDir = NULL;

  if (getvnchomedir(&homeDir) == -1) {
    vlog.error(_("Could not create VNC home directory: can't obtain home "
                 "directory path."));
  } else {
    int result = mkdir(homeDir, 0755);
    if (result == -1 && errno != EEXIST)
      vlog.error(_("Could not create VNC home directory: %s."), strerror(errno));
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
          "\nusage: %s [parameters] [host:displayNum] [parameters]\n"
          "       %s [parameters] -listen [port] [parameters]\n",
          programName, programName);
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
    strncpy(remoteHost, vncServerName, VNCSERVERNAMELEN);
  else
    strncpy(remoteHost, "localhost", VNCSERVERNAMELEN);

  remoteHost[VNCSERVERNAMELEN - 1] = '\0';

  snprintf(vncServerName, VNCSERVERNAMELEN, "localhost::%d", localPort);
  vncServerName[VNCSERVERNAMELEN - 1] = '\0';
  vlog.error(vncServerName);

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

  gatewayHost = strDup(via.getValueStr());
  if (interpretViaParam(remoteHost, &remotePort, localPort) != 0)
    return 1;
  createTunnel(gatewayHost, remoteHost, remotePort, localPort);

  return 0;
}
#endif /* !WIN32 */

int main(int argc, char** argv)
{
  UserDialog dlg;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE_NAME, LOCALE_DIR);
  textdomain(PACKAGE_NAME);

  // Generate the about string now that we get the proper translation
  snprintf(aboutText, sizeof(aboutText), _aboutText,
           (int)sizeof(size_t)*8, PACKAGE_VERSION,
           BUILD_TIMESTAMP, 2014);

  rfb::SecurityClient::setDefaults();

  // Write about text to console, still using normal locale codeset
  fprintf(stderr,"\n%s\n", aboutText);

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

  init_fltk();

#if !defined(WIN32) && !defined(__APPLE__)
  fl_open_display();
  XkbSetDetectableAutoRepeat(fl_display, True, NULL);
#endif

  Configuration::enableViewerParams();

  /* Load the default parameter settings */
  const char* defaultServerName;
  try {
    defaultServerName = loadViewerParameters(NULL);
  } catch (rfb::Exception& e) {
    fl_alert("%s", e.str());
  }
  
  int i = 1;
  if (!Fl::args(argc, argv, i) || i < argc)
    for (; i < argc; i++) {
      if (Configuration::setParam(argv[i]))
        continue;

      if (argv[i][0] == '-') {
        if (i+1 < argc) {
          if (Configuration::setParam(&argv[i][1], argv[i+1])) {
            i++;
            continue;
          }
        }
        usage(argv[0]);
      }

      strncpy(vncServerName, argv[i], VNCSERVERNAMELEN);
      vncServerName[VNCSERVERNAMELEN - 1] = '\0';
    }

  if (!::autoSelect.hasBeenSet()) {
    // Default to AutoSelect=0 if -PreferredEncoding or -FullColor is used
    if (::preferredEncoding.hasBeenSet() || ::fullColour.hasBeenSet() ||
	::fullColourAlias.hasBeenSet()) {
      ::autoSelect.setParam(false);
    }
  }
  if (!::fullColour.hasBeenSet() && !::fullColourAlias.hasBeenSet()) {
    // Default to FullColor=0 if AutoSelect=0 && LowColorLevel is set
    if (!::autoSelect && (::lowColourLevel.hasBeenSet() ||
                          ::lowColourLevelAlias.hasBeenSet())) {
      ::fullColour.setParam(false);
    }
  }
  if (!::customCompressLevel.hasBeenSet()) {
    // Default to CustomCompressLevel=1 if CompressLevel is used.
    if(::compressLevel.hasBeenSet()) {
      ::customCompressLevel.setParam(true);
    }
  }

  mkvnchomedir();

  CSecurity::upg = &dlg;
#ifdef HAVE_GNUTLS
  CSecurityTLS::msg = &dlg;
#endif

  Socket *sock = NULL;

#ifndef WIN32
  /* Specifying -via and -listen together is nonsense */
  if (listenMode && strlen(via.getValueStr()) > 0) {
    // TRANSLATORS: "Parameters" are command line arguments, or settings
    // from a file or the Windows registry.
    vlog.error(_("Parameters -listen and -via are incompatible"));
    fl_alert(_("Parameters -listen and -via are incompatible"));
    exit_vncviewer();
    return 1;
  }
#endif

  if (listenMode) {
    try {
      int port = 5500;
      if (isdigit(vncServerName[0]))
        port = atoi(vncServerName);

      TcpListener listener(NULL, port);

      vlog.info(_("Listening on port %d\n"), port);
      sock = listener.accept();   
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      fl_alert("%s", e.str());
      exit_vncviewer();
      return 1; 
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

  CConn *cc = new CConn(vncServerName, sock);

  while (!exitMainloop) {
    int next_timer;

    next_timer = Timer::checkTimeouts();
    if (next_timer == 0)
      next_timer = INT_MAX;

    if (Fl::wait((double)next_timer / 1000.0) < 0.0) {
      vlog.error(_("Internal FLTK error. Exiting."));
      break;
    }
  }

  delete cc;

  if (exitError != NULL)
    fl_alert("%s", exitError);

  return 0;
}
