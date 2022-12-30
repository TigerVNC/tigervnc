/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2012 Samuel Mannehed <samuel@cendio.se> for Cendio AB
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

#include <errno.h>
#include <algorithm>

#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>

#include <os/os.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include "fltk/layout.h"
#include "ServerDialog.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "vncviewer.h"
#include "parameters.h"

/* xpm:s predate const, so they have invalid definitions */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "../media/tigervnc_logo.xpm"
#pragma GCC diagnostic pop

using namespace std;
using namespace rfb;

static Fl_Pixmap logo(tigervnc_logo);

static LogWriter vlog("ServerDialog");

const char* SERVER_HISTORY="tigervnc.history";

ServerDialog::ServerDialog()
  : Fl_Window(450, 0, "TigerVNC")
{
  int y;
  Fl_Box *box;
  Fl_Widget *button;
  //Fl_Menu_Button *menu;

  y = 32;

  box = new Fl_Box(32*2, y, w() - 32*4, 32);
  box->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  box->box(FL_NO_BOX);
  box->image(logo);

  y += box->h() + 32;

  y += INPUT_LABEL_OFFSET;
  serverName = new Fl_Input_Choice(32*2, y, w() - 32*4,
                                   INPUT_HEIGHT, _("Server name:"));
  serverName->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
  // Bug fix for wrong background
  serverName->color(FL_BACKGROUND2_COLOR);
  y += serverName->h() + 16;
/*
  menu = new Fl_Menu_Button(24, y, 25, BUTTON_HEIGHT, "");
  menu->add(_("Options..."), 0, handleOptions, this, FL_MENU_DIVIDER);
  menu->add(_("Load..."), 0, handleLoad, this, 0);
  menu->add(_("Save As..."), 0, handleSaveAs, this, FL_MENU_DIVIDER);
  menu->add(_("About..."), 0, handleAbout, this, 0);
*/
  button = new Fl_Button(32*2, y, BUTTON_WIDTH, BUTTON_HEIGHT,
                         _("Options"));
  button->callback(handleOptions, this);

  button = new Fl_Return_Button(w() - 32*2 - BUTTON_WIDTH, y,
                                BUTTON_WIDTH, BUTTON_HEIGHT,
                                _("Connect"));
  button->callback(handleConnect, this);
  y += button->h() + 16;

  box = new Fl_Box(32*2, y, w() - 32*4, 2);
  box->box(FL_FLAT_BOX);
  box->color(fl_gray_ramp(3));
  y += box->h();

  static char ver[1024];

  box = new Fl_Box(32*2, y, w() - 32*4, 32);
  box->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  box->box(FL_NO_BOX);
  snprintf(ver, sizeof(ver), _("Version %s"), PACKAGE_VERSION);
  box->label(ver);
  box->labelsize(10);
  //box->labelcolor(fl_gray_ramp(3));
  box->labelfont(FL_BOLD);
  y += box->h() + 16;

  resizable(NULL);
  size(w(), y);

  callback(handleClose, this);
}


ServerDialog::~ServerDialog()
{
}


void ServerDialog::run(const char* servername, char *newservername)
{
  ServerDialog dialog;

  dialog.serverName->value(servername);

  dialog.show();

  try {
    size_t i;

    dialog.loadServerHistory();

    dialog.serverName->clear();
    for(i = 0; i < dialog.serverHistory.size(); ++i)
      dialog.serverName->add(dialog.serverHistory[i].c_str());
  } catch (Exception& e) {
    vlog.error("%s", e.str());
    fl_alert(_("Unable to load the server history:\n\n%s"),
             e.str());
  }

  while (dialog.shown()) Fl::wait();

  if (dialog.serverName->value() == NULL) {
    newservername[0] = '\0';
    return;
  }

  strncpy(newservername, dialog.serverName->value(), VNCSERVERNAMELEN);
  newservername[VNCSERVERNAMELEN - 1] = '\0';
}

void ServerDialog::handleOptions(Fl_Widget *widget, void *data)
{
  OptionsDialog::showDialog();
}




void ServerDialog::handleAbout(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}


void ServerDialog::handleClose(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;

  dialog->serverName->value("");
  dialog->hide();
}


void ServerDialog::handleConnect(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = dialog->serverName->value();

  dialog->hide();

  try {
    saveViewerParameters(NULL, servername);
  } catch (Exception& e) {
    vlog.error("%s", e.str());
    fl_alert(_("Unable to save the default configuration:\n\n%s"),
             e.str());
  }

  try {
    vector<string>::iterator elem = std::find(dialog->serverHistory.begin(), dialog->serverHistory.end(), servername);
    // avoid duplicates in the history
    if(dialog->serverHistory.end() == elem) {
      dialog->serverHistory.insert(dialog->serverHistory.begin(), servername);
      dialog->saveServerHistory();
    }
  } catch (Exception& e) {
    vlog.error("%s", e.str());
    fl_alert(_("Unable to save the server history:\n\n%s"),
             e.str());
  }
}


void ServerDialog::loadServerHistory()
{
  serverHistory.clear();

#ifdef _WIN32
  loadHistoryFromRegKey(serverHistory);
  return;
#endif

  char* homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1)
    throw Exception(_("Could not obtain the home directory path"));

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s%s", homeDir, SERVER_HISTORY);
  delete[] homeDir;

  /* Read server history from file */
  FILE* f = fopen(filepath, "r");
  if (!f) {
    if (errno == ENOENT) {
      // no history file
      return;
    }
    throw Exception(_("Could not open \"%s\": %s"),
                    filepath, strerror(errno));
  }

  int lineNr = 0;
  while (!feof(f)) {
    char line[256];

    // Read the next line
    lineNr++;
    if (!fgets(line, sizeof(line), f)) {
      if (feof(f))
        break;

      fclose(f);
      throw Exception(_("Failed to read line %d in file %s: %s"),
                      lineNr, filepath, strerror(errno));
    }

    int len = strlen(line);

    if (len == (sizeof(line) - 1)) {
      fclose(f);
      throw Exception(_("Failed to read line %d in file %s: %s"),
                      lineNr, filepath, _("Line too long"));
    }

    if ((len > 0) && (line[len-1] == '\n')) {
      line[len-1] = '\0';
      len--;
    }
    if ((len > 0) && (line[len-1] == '\r')) {
      line[len-1] = '\0';
      len--;
    }

    if (len == 0)
      continue;

    serverHistory.push_back(line);
  }

  fclose(f);
}

void ServerDialog::saveServerHistory()
{
#ifdef _WIN32
  saveHistoryToRegKey(serverHistory);
  return;
#endif

  char* homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1)
    throw Exception(_("Could not obtain the home directory path"));

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s%s", homeDir, SERVER_HISTORY);
  delete[] homeDir;

  /* Write server history to file */
  FILE* f = fopen(filepath, "w+");
  if (!f)
    throw Exception(_("Could not open \"%s\": %s"),
                    filepath, strerror(errno));

  // Save the last X elements to the config file.
  for(size_t i=0; i < serverHistory.size() && i <= SERVER_HISTORY_SIZE; i++)
    fprintf(f, "%s\n", serverHistory[i].c_str());

  fclose(f);
}
