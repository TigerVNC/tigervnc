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
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>

#include "ServerDialog.h"
#include "OptionsDialog.h"
#include "fltk_layout.h"
#include "i18n.h"
#include "vncviewer.h"
#include "parameters.h"
#include "rfb/Exception.h"

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
  
  serverName = new Fl_Input(x, y, w() - margin*2 - server_label_width, INPUT_HEIGHT, _("VNC server:"));

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


const char *ServerDialog::run(const char* servername)
{
  ServerDialog dialog;
  static char buffer[256];

  dialog.serverName->value(servername);
  
  dialog.show();
  while (dialog.shown()) Fl::wait();

  if (dialog.serverName->value() == NULL)
    return NULL;

  strncpy(buffer, dialog.serverName->value(), sizeof(buffer));
  buffer[sizeof(buffer)-1] = '\0';

  return buffer;
}

void ServerDialog::handleOptions(Fl_Widget *widget, void *data)
{
  OptionsDialog::showDialog();
}


void ServerDialog::handleLoad(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;
  Fl_File_Chooser* file_chooser = new Fl_File_Chooser("", "TigerVNC configuration (*.tigervnc)", 
						      0, "Select a TigerVNC configuration file");
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
  
  const char* filename = strdup(file_chooser->value());

  try {
    dialog->serverName->value(loadViewerParameters(filename));
  } catch (rfb::Exception& e) {
    fl_alert("%s", e.str());
  }

  delete(file_chooser);
}


void ServerDialog::handleSaveAs(Fl_Widget *widget, void *data)
{ 
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = strdup(dialog->serverName->value());
  char* filename;

  Fl_File_Chooser* file_chooser = new Fl_File_Chooser("", "TigerVNC configuration (*.tigervnc)", 
						      2, "Save the TigerVNC configuration to file");
  
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
    
    filename = strdup(file_chooser->value());
    
    FILE* f = fopen(filename, "r");
    if (f) {

      // The file already exists.
      fclose(f);
      int overwrite_choice = fl_choice("%s already exists. Do you want to overwrite?", 
				       "Overwrite", "No", NULL, filename);
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
  } catch (rfb::Exception& e) {
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

  dialog->serverName->value(NULL);
  dialog->hide();
}


void ServerDialog::handleConnect(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;
  const char* servername = strdup(dialog->serverName->value());

  dialog->hide();
  
  try {
    saveViewerParameters(NULL, servername);
  } catch (rfb::Exception& e) {
    fl_alert("%s", e.str());
  }
}
