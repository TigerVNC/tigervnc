/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include "ServerDialog.h"
#include "OptionsDialog.h"
#include "fltk_layout.h"
#include "i18n.h"
#include "vncviewer.h"

ServerDialog::ServerDialog()
  : Fl_Window(400, 112, _("VNC Viewer: Connection Details"))
{
  int width;
  Fl_Button *button;

  width = gui_str_len(_("VNC server:"));
  serverName = new Fl_Input(20 + width, 20, w() - 20*2 - width, 25, _("VNC server:"));

  width = (w() - 20) / 4;

  button = new Fl_Button(20 + width*0, 20+25+20, width - 20, 27, _("About..."));
  button->callback(this->handleAbout, this);

  button = new Fl_Button(20 + width*1, 20+25+20, width - 20, 27, _("Options..."));
  button->callback(this->handleOptions, this);

  button = new Fl_Button(20 + width*2, 20+25+20, width - 20, 27, _("Cancel"));
  button->callback(this->handleCancel, this);

  button = new Fl_Return_Button(20 + width*3, 20+25+20, width - 20, 27, _("OK"));
  button->callback(this->handleOK, this);

  callback(this->handleCancel, this);

  set_modal();
}


ServerDialog::~ServerDialog()
{
}


const char *ServerDialog::run()
{
  ServerDialog dialog;
  static char buffer[256];

  dialog.show();
  while (dialog.shown()) Fl::wait();

  if (dialog.serverName->value() == NULL)
    return NULL;

  strncpy(buffer, dialog.serverName->value(), sizeof(buffer));
  buffer[sizeof(buffer)-1] = '\0';

  return buffer;
}


void ServerDialog::handleAbout(Fl_Widget *widget, void *data)
{
  about_vncviewer();
}


void ServerDialog::handleOptions(Fl_Widget *widget, void *data)
{
  OptionsDialog::showDialog();
}


void ServerDialog::handleCancel(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;

  dialog->serverName->value(NULL);
  dialog->hide();
}


void ServerDialog::handleOK(Fl_Widget *widget, void *data)
{
  ServerDialog *dialog = (ServerDialog*)data;

  dialog->hide();
}
