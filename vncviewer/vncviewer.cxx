/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <os/os.h>
#include <rfb/Logger_stdio.h>
#include <rfb/SecurityClient.h>
#include <rfb/Security.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#include <rfb/LogWriter.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_ask.H>
#include <FL/x.H>

#include "i18n.h"
#include "parameters.h"
#include "CConn.h"
#include "UserDialog.h"

rfb::LogWriter vlog("main");

using namespace network;
using namespace rfb;
using namespace std;

static char aboutText[1024];

static bool exitMainloop = false;

void exit_vncviewer()
{
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
  char* vncServerName = 0;
  UserDialog dlg;

  const char englishAbout[] = N_("TigerVNC Viewer version %s\n"
                                 "Copyright (C) 2002-2005 RealVNC Ltd.\n"
                                 "Copyright (C) 2000-2006 TightVNC Group\n"
                                 "Copyright (C) 2004-2009 Peter Astrand for Cendio AB\n"
                                 "See http://www.tigervnc.org for information on TigerVNC.");

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE_NAME, LOCALEDIR);
  textdomain(PACKAGE_NAME);

  rfb::SecurityClient::setDefaults();

  // Write about text to console, still using normal locale codeset
  snprintf(aboutText, sizeof(aboutText),
           gettext(englishAbout), PACKAGE_VERSION);
  fprintf(stderr,"\n%s\n", aboutText);

  // Set gettext codeset to what our GUI toolkit uses. Since we are
  // passing strings from strerror/gai_strerror to the GUI, these must
  // be in GUI codeset as well.
  bind_textdomain_codeset(PACKAGE_NAME, "UTF-8");
  bind_textdomain_codeset("libc", "UTF-8");

  // Re-create the aboutText for the GUI, now using GUI codeset
  snprintf(aboutText, sizeof(aboutText),
           gettext(englishAbout), PACKAGE_VERSION);

  rfb::initStdIOLoggers();
  rfb::LogWriter::setLogParams("*:stderr:30");

#ifdef SIGHUP
  signal(SIGHUP, CleanupSignalHandler);
#endif
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  init_fltk();

  Configuration::enableViewerParams();

  for (int i = 1; i < argc; i++) {
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

  CConn cc(vncServerName);

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

  return 0;
}
