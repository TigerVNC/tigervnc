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
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#include <rfb/Logger_stdio.h>
#include <rfb/SecurityClient.h>
#include <rfb/Security.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#include <rfb/LogWriter.h>
#include <rfb/Timer.h>
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

#ifdef WIN32
#include "resource.h"
#include "win32.h"
#endif

rfb::LogWriter vlog("main");

using namespace network;
using namespace rfb;
using namespace std;

static char aboutText[1024];
extern const char* buildTime;

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
  fl_message(aboutText);
}

static void about_callback(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  vlog.info("CleanupSignalHandler called");
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

      sprintf(icon_path, "%s/icons/hicolor/%dx%d/tigervnc.png",
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
  exit(1);
}

int main(int argc, char** argv)
{
  const char* vncServerName = NULL;
  UserDialog dlg;

  const char englishAbout[] = N_("TigerVNC Viewer %d-bit v%s (%s)\n"
                                 "%s\n"
                                 "Copyright (C) 1999-2011 TigerVNC Team and many others (see README.txt)\n"
                                 "See http://www.tigervnc.org for information on TigerVNC.");

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE_NAME, LOCALE_DIR);
  textdomain(PACKAGE_NAME);

  rfb::SecurityClient::setDefaults();

  // Write about text to console, still using normal locale codeset
  snprintf(aboutText, sizeof(aboutText),
           gettext(englishAbout), (int)sizeof(size_t)*8, PACKAGE_VERSION,
           __BUILD__, buildTime);
  fprintf(stderr,"\n%s\n", aboutText);

  // Set gettext codeset to what our GUI toolkit uses. Since we are
  // passing strings from strerror/gai_strerror to the GUI, these must
  // be in GUI codeset as well.
  bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
  bind_textdomain_codeset("libc", "UTF-8");

  // Re-create the aboutText for the GUI, now using GUI codeset
  snprintf(aboutText, sizeof(aboutText),
           gettext(englishAbout), (int)sizeof(size_t)*8, PACKAGE_VERSION,
           __BUILD__, buildTime);

  rfb::initStdIOLoggers();
  rfb::LogWriter::setLogParams("*:stderr:30");

#ifdef SIGHUP
  signal(SIGHUP, CleanupSignalHandler);
#endif
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  init_fltk();

  Configuration::enableViewerParams();

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

      vncServerName = argv[i];
    }

  if (!::autoSelect.hasBeenSet()) {
    // Default to AutoSelect=0 if -PreferredEncoding or -FullColor is used
    ::autoSelect.setParam(!::preferredEncoding.hasBeenSet() &&
                          !::fullColour.hasBeenSet() &&
                          !::fullColourAlias.hasBeenSet());
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
    ::customCompressLevel.setParam(::compressLevel.hasBeenSet());
  }

  mkvnchomedir();

  CSecurity::upg = &dlg;
#ifdef HAVE_GNUTLS
  CSecurityTLS::msg = &dlg;
#endif

  if (vncServerName == NULL) {
    vncServerName = ServerDialog::run();
    if ((vncServerName == NULL) || (vncServerName[0] == '\0'))
      return 1;
  }

  CConn *cc = new CConn(vncServerName);

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
    fl_alert(exitError);

  return 0;
}
