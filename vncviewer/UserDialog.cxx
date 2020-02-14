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
#include <FL/Fl_Pixmap.H>

#include <rfb/util.h>
#include <rfb/Password.h>
#include <rfb/Exception.h>

#include "i18n.h"
#include "fltk_layout.h"
#include "parameters.h"
#include "UserDialog.h"

/* xpm:s predate const, so they have invalid definitions */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "../media/secure.xpm"
#include "../media/insecure.xpm"
#pragma GCC diagnostic pop

using namespace rfb;

static Fl_Pixmap secure_icon(secure);
static Fl_Pixmap insecure_icon(insecure);

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

void UserDialog::getUserPasswd(bool secure, char** user, char** password)
{
  CharArray passwordFileStr(passwordFile.getData());

  assert(password);
  char *envUsername = getenv("VNC_USERNAME");
  char *envPassword = getenv("VNC_PASSWORD");

  if(user && envUsername && envPassword) {
    *user = strdup(envUsername);
    *password = strdup(envPassword);
    return;
  }

  if (!user && envPassword) {
    *password = strdup(envPassword);
    return;
  }

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

  Fl_Window *win;
  Fl_Box *banner;
  Fl_Input *username;
  Fl_Secret_Input *passwd;
  Fl_Box *icon;
  Fl_Button *button;

  int y;

  win = new Fl_Window(410, 145, _("VNC authentication"));
  win->callback(button_cb,(void *)0);

  banner = new Fl_Box(0, 0, win->w(), 20);
  banner->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE|FL_ALIGN_IMAGE_NEXT_TO_TEXT);
  banner->box(FL_FLAT_BOX);
  if (secure) {
    banner->label(_("This connection is secure"));
    banner->color(FL_GREEN);
    banner->image(secure_icon);
  } else {
    banner->label(_("This connection is not secure"));
    banner->color(FL_RED);
    banner->image(insecure_icon);
  }

  y = 20 + 10;

  icon = new Fl_Box(10, y, 50, 50, "?");
  icon->box(FL_UP_BOX);
  icon->labelfont(FL_TIMES_BOLD);
  icon->labelsize(34);
  icon->color(FL_WHITE);
  icon->labelcolor(FL_BLUE);

  y += 5;

  if (user) {
    (new Fl_Box(70, y, win->w()-70-10, 20, _("Username:")))
      ->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
    y += 20 + 5;
    username = new Fl_Input(70, y, win->w()-70-10, 25);
    y += 25 + 5;
  } else {
    /*
     * Compiler is not bright enough to understand that
     * username won't be used further down...
     */
    username = NULL;
  }

  (new Fl_Box(70, y, win->w()-70-10, 20, _("Password:")))
    ->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
  y += 20 + 5;
  passwd = new Fl_Secret_Input(70, y, win->w()-70-10, 25);
  y += 25 + 5;

  y += 5;

  button = new Fl_Return_Button(310, y, 90, 25, fl_ok);
  button->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
  button->callback(button_cb, (void*)0);

  button = new Fl_Button(210, y, 90, 25, fl_cancel);
  button->align(FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
  button->callback(button_cb, (void*)1);
  button->shortcut(FL_Escape);

  y += 25 + 10;

  win->end();

  win->size(win->w(), y);

  win->set_modal();

  ret_val = -1;

  win->show();
  while (win->shown()) Fl::wait();

  if (ret_val == 0) {
    if (user)
      *user = strDup(username->value());
    *password = strDup(passwd->value());
  }

  delete win;

  if (ret_val != 0)
    throw rfb::Exception(_("Authentication cancelled"));
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
