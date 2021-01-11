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

#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>

#include <algorithm>
#include <iostream>
#include <fstream>

#include <os/os.h>
#include <rfb/Exception.h>

#include "ServerDialog.h"
#include "OptionsDialog.h"
#include "fltk_layout.h"
#include "i18n.h"
#include "vncviewer.h"
#include "parameters.h"


using namespace std;
using namespace rfb;

const char* SERVER_HISTORY="tigervnc.history";

ServerDialog::ServerDialog()
  : Fl_Window(450, 160, _("VNC Viewer: Connection Details"))
{
  int x, y;
  Fl_Button *button;
  Fl_Box *divider;

  int margin = 20;
  int server_label_width = gui_str_len(_("VNC server:"));

  x = margin + server_label_width;
  y = margin;
  
  serverName = new Fl_Input_Choice(x, y, w() - margin*2 - server_label_width, INPUT_HEIGHT, _("VNC server:"));
  loadServerHistory();
  for(size_t i=0;i<serverHistory.size();++i) {
    serverName->add(serverHistory[i].c_str());
  }

  int adjust = (w() - 20) / 4;
  int button_width = adjust - margin/2;

  x = margin;
  y = margin + margin/2 + INPUT_HEIGHT;

  y += margin/2;

  button = new Fl_Button(x, y, button_width, BUTTON_HEIGHT, _("Options..."));
  button->callback(this->handleOptions, this);
  
  x += adjust;
  
  button = new Fl_Button(x, y, button_width, BUTTON_HEIGHT, _("Load..."));
  button->callback(this->handleLoad, this);

  x += adjust;

  button = new Fl_Button(x, y, button_width, BUTTON_HEIGHT, _("Save As..."));
  button->callback(this->handleSaveAs, this);
  
  x = 0;
  y += margin/2 + BUTTON_HEIGHT;

  divider = new Fl_Box(x, y, w(), 2);
  divider->box(FL_THIN_DOWN_FRAME);
  
  x += margin;
  y += margin/2;

  button = new Fl_Button(x, y, button_width, BUTTON_HEIGHT, _("About..."));
  button->callback(this->handleAbout, this);

  x = w() - margin - adjust - button_width - 20;

  button = new Fl_Button(x, y, button_width, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);

  x += adjust;

  button = new Fl_Return_Button(x, y, button_width+20, BUTTON_HEIGHT, _("Connect"));
  button->callback(this->handleConnect, this);

  callback(this->handleCancel, this);

  set_modal();
}


ServerDialog::~ServerDialog()
{
}


void ServerDialog::run(const char* servername, char *newservername)
{
  ServerDialog dialog;

  dialog.serverName->value(servername);

  dialog.show();
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


void ServerDialog::handleLoad(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;
  Fl_File_Chooser* file_chooser = new Fl_File_Chooser("", _("TigerVNC configuration (*.tigervnc)"), 
						      0, _("Select a TigerVNC configuration file"));
  file_chooser->preview(0);
  file_chooser->previewButton->hide();
  file_chooser->show();
  
  // Block until user picks something.
  while(file_chooser->shown())
    Fl::wait();
  
  // Did the user hit cancel?
  if (file_chooser->value() == NULL) {
    delete(file_chooser);
    return;
  }
  
  const char* filename = file_chooser->value();

  try {
    dialog->serverName->value(loadViewerParameters(filename));
  } catch (Exception& e) {
    fl_alert("%s", e.str());
  }

  delete(file_chooser);
}


void ServerDialog::handleSaveAs(Fl_Widget *widget, void *data)
{ 
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = dialog->serverName->value();
  const char* filename;

  Fl_File_Chooser* file_chooser = new Fl_File_Chooser("", _("TigerVNC configuration (*.tigervnc)"), 
						      2, _("Save the TigerVNC configuration to file"));
  
  file_chooser->preview(0);
  file_chooser->previewButton->hide();
  file_chooser->show();
  
  while(1) {
    
    // Block until user picks something.
    while(file_chooser->shown())
      Fl::wait();
    
    // Did the user hit cancel?
    if (file_chooser->value() == NULL) {
      delete(file_chooser);
      return;
    }
    
    filename = file_chooser->value();
    
    FILE* f = fopen(filename, "r");
    if (f) {

      // The file already exists.
      fclose(f);
      int overwrite_choice = fl_choice(_("%s already exists. Do you want to overwrite?"), 
				       _("Overwrite"), _("No"), NULL, filename);
      if (overwrite_choice == 1) {

	// If the user doesn't want to overwrite:
	file_chooser->show();
	continue;
      }
    }

    break;
  }
  
  try {
    saveViewerParameters(filename, servername);
  } catch (Exception& e) {
    fl_alert("%s", e.str());
  }
  
  delete(file_chooser);
}


void ServerDialog::handleAbout(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}


void ServerDialog::handleCancel(Fl_Widget *widget, void *data)
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

    vector<string>::iterator elem = std::find(dialog->serverHistory.begin(), dialog->serverHistory.end(), servername);
    // avoid duplicates in the history
    if(dialog->serverHistory.end() == elem) {
      dialog->serverHistory.insert(dialog->serverHistory.begin(), servername);
      dialog->saveServerHistory();
    }
  } catch (Exception& e) {
    fl_alert("%s", e.str());
  }
}


void ServerDialog::loadServerHistory()
{
#ifdef _WIN32
  loadHistoryFromRegKey(serverHistory);
  return;
#endif

  char* homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1) {
    throw Exception(_("Failed to read server history file, "
		      "can't obtain home directory path."));
  }

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s%s", homeDir, SERVER_HISTORY);
  delete[] homeDir;

  /* Read server history from file */
  ifstream f (filepath);
  if (!f.is_open()) {
    // no history file
    return;
  }

  string line;
  while(getline(f, line)) {
    serverHistory.push_back(line);
  }

  if (f.bad()) {
    throw Exception(_("Failed to read server history file, "
		      "error while reading file."));
  }
  f.close();
}

void ServerDialog::saveServerHistory()
{
#ifdef _WIN32
  saveHistoryToRegKey(serverHistory);
  return;
#endif

  char* homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1) {
    throw Exception(_("Failed to write server history file, "
		      "can't obtain home directory path."));
  }

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s%s", homeDir, SERVER_HISTORY);
  delete[] homeDir;

  /* Write server history to file */
  ofstream f (filepath);
  if (!f.is_open()) {
    throw Exception(_("Failed to write server history file, "
		      "can't open file."));
  }

  // Save the last X elements to the config file.
  for(size_t i=0; i < serverHistory.size() && i <= SERVER_HISTORY_SIZE; i++) {
    f << serverHistory[i] << endl;
  }

  if (f.bad()) {
    throw Exception(_("Failed to write server history file, "
		      "error while writing file."));
  }
  f.close();
}
