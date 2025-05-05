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
#include <libgen.h>

// FIXME: Workaround for FLTK including windows.h
#ifdef WIN32
#include <winsock2.h>
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

#include <core/Exception.h>
#include <core/LogWriter.h>
#include <core/string.h>
#include <core/xdgdirs.h>

#include <network/TcpSocket.h>

#include "fltk/layout.h"
#include "fltk/util.h"
#include "ServerDialog.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "vncviewer.h"
#include "parameters.h"

static core::LogWriter vlog("ServerDialog");

const char* SERVER_HISTORY="tigervnc.history";

ServerDialog::ServerDialog()
  : Fl_Window(450, 0, "TigerVNC")
{
  int x, y, x2;
  Fl_Button *button;
  Fl_Box *divider;

  x = OUTER_MARGIN;
  y = OUTER_MARGIN;

  serverName = new Fl_Input_Choice(LBLLEFT(x, y, w() - OUTER_MARGIN*2,
                                   INPUT_HEIGHT, _("VNC server:")));
  // Bug fix for wrong background
  serverName->color(FL_BACKGROUND2_COLOR);
  y += INPUT_HEIGHT + INNER_MARGIN;

  x2 = x;

  button = new Fl_Button(x2, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Options..."));
  button->callback(this->handleOptions, this);
  x2 += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Button(x2, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Load..."));
  button->callback(this->handleLoad, this);
  x2 += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Button(x2, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Save as..."));
  button->callback(this->handleSaveAs, this);
  x2 += BUTTON_WIDTH + INNER_MARGIN;

  y += BUTTON_HEIGHT + INNER_MARGIN;

  divider = new Fl_Box(0, y, w(), 2);
  divider->box(FL_THIN_DOWN_FRAME);

  y += divider->h() + INNER_MARGIN;

  // Symmetric margin around bottom button bar
  y += OUTER_MARGIN - INNER_MARGIN;

  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("About..."));
  button->callback(this->handleAbout, this);

  x2 = w() - OUTER_MARGIN - BUTTON_WIDTH*2 - INNER_MARGIN*1;

  button = new Fl_Button(x2, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);
  x2 += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Return_Button(x2, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Connect"));
  button->callback(this->handleConnect, this);
  x2 += BUTTON_WIDTH + INNER_MARGIN;

  y += BUTTON_HEIGHT + INNER_MARGIN;

  /* Needed for resize to work sanely */
  resizable(nullptr);
  h(y-INNER_MARGIN+OUTER_MARGIN);

  callback(this->handleCancel, this);
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
    dialog.loadServerHistory();

    dialog.serverName->clear();
    for (const std::string& entry : dialog.serverHistory)
      fltk_menu_add(dialog.serverName->menubutton(),
                    entry.c_str(), 0, nullptr);
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    fl_alert(_("Unable to load the server history:\n\n%s"),
             e.what());
  }

  while (dialog.shown()) Fl::wait();

  if (dialog.serverName->value() == nullptr) {
    newservername[0] = '\0';
    return;
  }

  strncpy(newservername, dialog.serverName->value(), VNCSERVERNAMELEN);
  newservername[VNCSERVERNAMELEN - 1] = '\0';
}

void ServerDialog::handleOptions(Fl_Widget* /*widget*/, void* /*data*/)
{
  OptionsDialog::showDialog();
}


void ServerDialog::handleLoad(Fl_Widget* /*widget*/, void* data)
{
  ServerDialog *dialog = (ServerDialog*)data;

  if (dialog->usedDir.empty())
    dialog->usedDir = core::getuserhomedir();

  Fl_File_Chooser* file_chooser = new Fl_File_Chooser(dialog->usedDir.c_str(),
                                                      _("TigerVNC configuration (*.tigervnc)"),
                                                      0, _("Select a TigerVNC configuration file"));
  file_chooser->preview(0);
  file_chooser->previewButton->hide();
  file_chooser->show();
  
  // Block until user picks something.
  while(file_chooser->shown())
    Fl::wait();
  
  // Did the user hit cancel?
  if (file_chooser->value() == nullptr) {
    delete(file_chooser);
    return;
  }
  
  const char* filename = file_chooser->value();
  dialog->updateUsedDir(filename);

  try {
    dialog->serverName->value(loadViewerParameters(filename));
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    fl_alert(_("Unable to load the specified configuration file:\n\n%s"),
             e.what());
  }

  delete(file_chooser);
}


void ServerDialog::handleSaveAs(Fl_Widget* /*widget*/, void* data)
{ 
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = dialog->serverName->value();
  const char* filename;
  if (dialog->usedDir.empty())
    dialog->usedDir = core::getuserhomedir();
  
  Fl_File_Chooser* file_chooser = new Fl_File_Chooser(dialog->usedDir.c_str(),
                                                      _("TigerVNC configuration (*.tigervnc)"),
                                                      2, _("Save the TigerVNC configuration to file"));
  
  file_chooser->preview(0);
  file_chooser->previewButton->hide();
  file_chooser->show();
  
  while(1) {
    
    // Block until user picks something.
    while(file_chooser->shown())
      Fl::wait();
    
    // Did the user hit cancel?
    if (file_chooser->value() == nullptr) {
      delete(file_chooser);
      return;
    }
    
    filename = file_chooser->value();
    dialog->updateUsedDir(filename);
    
    FILE* f = fopen(filename, "r");
    if (f) {

      // The file already exists.
      fclose(f);
      int overwrite_choice = fl_choice(_("%s already exists. Do you want to overwrite?"), 
                                       _("Overwrite"), _("No"), nullptr, filename);
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
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    fl_alert(_("Unable to save the specified configuration "
               "file:\n\n%s"), e.what());
  }
  
  delete(file_chooser);
}


void ServerDialog::handleAbout(Fl_Widget* /*widget*/, void* /*data*/)
{
  about_vncviewer();
}


void ServerDialog::handleCancel(Fl_Widget* /*widget*/, void* data)
{
  ServerDialog *dialog = (ServerDialog*)data;

  dialog->serverName->value("");
  dialog->hide();
}


void ServerDialog::handleConnect(Fl_Widget* /*widget*/, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = dialog->serverName->value();

  dialog->hide();

  try {
    saveViewerParameters(nullptr, servername);
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    fl_alert(_("Unable to save the default configuration:\n\n%s"),
             e.what());
  }

  // avoid duplicates in the history
  dialog->serverHistory.remove(servername);
  dialog->serverHistory.insert(dialog->serverHistory.begin(), servername);

  try {
    dialog->saveServerHistory();
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    fl_alert(_("Unable to save the server history:\n\n%s"),
             e.what());
  }
}


static bool same_server(const std::string& a, const std::string& b)
{
  std::string hostA, hostB;
  int portA, portB;

#ifndef WIN32
  if ((a.find("/") != std::string::npos) ||
      (b.find("/") != std::string::npos))
    return a == b;
#endif

  try {
    network::getHostAndPort(a.c_str(), &hostA, &portA);
    network::getHostAndPort(b.c_str(), &hostB, &portB);
  } catch (std::exception& e) {
    return false;
  }

  if (hostA != hostB)
    return false;

  if (portA != portB)
    return false;

  return true;
}


void ServerDialog::loadServerHistory()
{
  std::list<std::string> rawHistory;

  serverHistory.clear();

#ifdef _WIN32
  rawHistory = loadHistoryFromRegKey();
#else

  const char* stateDir = core::getvncstatedir();
  if (stateDir == nullptr)
    throw std::runtime_error(_("Could not determine VNC state directory path"));

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s/%s", stateDir, SERVER_HISTORY);

  /* Read server history from file */
  FILE* f = fopen(filepath, "r");
  if (!f) {
    if (errno == ENOENT) {
      // no history file
      return;
    }
    throw core::posix_error(
      core::format(_("Could not open \"%s\""), filepath), errno);
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
      throw core::posix_error(
        core::format(_("Failed to read line %d in file \"%s\""),
                     lineNr, filepath),
        errno);
    }

    int len = strlen(line);

    if (len == (sizeof(line) - 1)) {
      fclose(f);
      std::string msg = core::format(_("Failed to read line %d in "
                                       "file \"%s\""),
                                     lineNr, filepath);
      throw std::runtime_error(
        core::format("%s: %s", msg.c_str(), _("Line too long")));
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

    rawHistory.push_back(line);
  }

  fclose(f);
#endif

  // Filter out duplicates, even if they have different formats
  for (const std::string& entry : rawHistory) {
    if (std::find_if(serverHistory.begin(), serverHistory.end(),
                     [&entry](const std::string& s) {
                       return same_server(s, entry);
                     }) != serverHistory.end())
      continue;
    serverHistory.push_back(entry);
  }
}

void ServerDialog::saveServerHistory()
{
#ifdef _WIN32
  saveHistoryToRegKey(serverHistory);
  return;
#endif

  const char* stateDir = core::getvncstatedir();
  if (stateDir == nullptr)
    throw std::runtime_error(_("Could not determine VNC state directory path"));

  char filepath[PATH_MAX];
  snprintf(filepath, sizeof(filepath), "%s/%s", stateDir, SERVER_HISTORY);

  /* Write server history to file */
  FILE* f = fopen(filepath, "w+");
  if (!f) {
    std::string msg = core::format(_("Could not open \"%s\""), filepath);
    throw core::posix_error(msg.c_str(), errno);
  }

  // Save the last X elements to the config file.
  size_t count = 0;
  for (const std::string& entry : serverHistory) {
    if (++count > SERVER_HISTORY_SIZE)
      break;
    fprintf(f, "%s\n", entry.c_str());
  }

  fclose(f);
}

void ServerDialog::updateUsedDir(const char* filename)
{
  char * name = strdup(filename);
  usedDir = dirname(name);
  free(name);
}
