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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>

#include <rfb/util.h>
#include <rfb/Password.h>
#include <rfb/Exception.h>

#include "i18n.h"
#include "fltk_layout.h"
#include "parameters.h"
#include "UserDialog.h"

using namespace rfb;

static int ret_val = 0;

static void button_cb(Fl_Widget *widget, void *val) {
  ret_val = (fl_intptr_t)val;
  widget->window()->hide();
}

UserDialog::UserDialog()
{
}

UserDialog::~UserDialog()
{
}

void UserDialog::getUserPasswd(char** user, char** password)
{
  CharArray passwordFileStr(passwordFile.getData());

  assert(password);

  if (!user && passwordFileStr.buf[0]) {
    ObfuscatedPasswd obfPwd(256);
    FILE* fp;

    fp = fopen(passwordFileStr.buf, "rb");
    if (!fp)
      throw rfb::Exception(_("Opening password file failed"));

    obfPwd.length = fread(obfPwd.buf, 1, obfPwd.length, fp);
    fclose(fp);

    PlainPasswd passwd(obfPwd);
    *password = passwd.takeBuf();

    return;
  }

  if (!user) {
    fl_message_title(_("VNC authentication"));
    *password = strDup(fl_password(_("Password:"), ""));
    if (!*password)
      throw rfb::Exception(_("Authentication cancelled"));

    return;
  }

  // Largely copied from FLTK so that we get the same look and feel
  // as the simpler password input.
  Fl_Window *win = new Fl_Window(410, 145, _("VNC authentication"));
  win->callback(button_cb,(void *)0);

  Fl_Input *username = new Fl_Input(70, 25, 300, 25, _("Username:"));
  username->align(FL_ALIGN_TOP_LEFT);

  Fl_Secret_Input *passwd = new Fl_Secret_Input(70, 70, 300, 25, _("Password:"));
  passwd->align(FL_ALIGN_TOP_LEFT);

  Fl_Box *icon = new Fl_Box(10, 10, 50, 50, "?");
  icon->box(FL_UP_BOX);
  icon->labelfont(FL_TIMES_BOLD);
  icon->labelsize(34);
  icon->color(FL_WHITE);
  icon->labelcolor(FL_BLUE);

  Fl_Button *button;

  button = new Fl_Return_Button(310, 110, 90, 25, fl_ok);
  button->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
  button->callback(button_cb, (void*)0);

  button = new Fl_Button(210, 110, 90, 25, fl_cancel);
  button->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
  button->callback(button_cb, (void*)1);
  button->shortcut(FL_Escape);

  win->end();

  win->set_modal();

  ret_val = -1;

  win->show();
  while (win->shown()) Fl::wait();

  if (ret_val == 0) {
    *user = strDup(username->value());
    *password = strDup(passwd->value());
  } else {
    *user = strDup("");
    *password = strDup("");
  }

  delete win;
}

bool UserDialog::showMsgBox(int flags, const char* title, const char* text)
{
  char buffer[1024];

  if (fltk_escape(text, buffer, sizeof(buffer)) >= sizeof(buffer))
    return 0;

  // FLTK doesn't give us a flexible choice of the icon, so we ignore those
  // bits for now.

  fl_message_title(title);

  switch (flags & 0xf) {
  case M_OKCANCEL:
    return fl_choice("%s", NULL, fl_ok, fl_cancel, buffer) == 1;
  case M_YESNO:
    return fl_choice("%s", NULL, fl_yes, fl_no, buffer) == 1;
  case M_OK:
  default:
    if (((flags & 0xf0) == M_ICONERROR) ||
        ((flags & 0xf0) == M_ICONWARNING))
      fl_alert("%s", buffer);
    else
      fl_message("%s", buffer);
    return true;
  }

  return false;
}
