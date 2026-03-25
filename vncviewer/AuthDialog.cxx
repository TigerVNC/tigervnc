/* Copyright 2024 Adam Halim for Cendio AB
 * Copyright 2011-2026 Pierre Ossman for Cendio AB
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

#include <assert.h>

#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/fl_ask.H>

#include "fltk/layout.h"

#include "AuthDialog.h"
#include "parameters.h"
#include "i18n.h"

/* xpm:s predate const, so they have invalid definitions */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "../media/secure.xpm"
#include "../media/insecure.xpm"
#pragma GCC diagnostic pop

static Fl_Pixmap secure_icon(secure);
static Fl_Pixmap insecure_icon(insecure);

AuthDialog::AuthDialog(bool secure_, bool needsUser, bool needsPassword)
  : Fl_Window(410, 0, _("VNC authentication"))
{
  int x, y;

  Fl_Box* banner;
  Fl_Box* icon;
  Fl_Button* button;

  result_ = 0;

  banner = new Fl_Box(0, 0,  w(), 20);
  banner->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE|FL_ALIGN_IMAGE_NEXT_TO_TEXT);
  banner->box(FL_FLAT_BOX);
  if (secure_) {
    banner->label(_("This connection is secure"));
    banner->color(FL_GREEN);
    banner->image(secure_icon);
  } else {
    banner->label(_("This connection is not secure"));
    banner->color(FL_RED);
    banner->image(insecure_icon);
  }

  x = OUTER_MARGIN;
  y = banner->h() + OUTER_MARGIN;

  /* Mimic a fl_ask() box */
  icon = new Fl_Box(x, y, 50, 50, "?");
  icon->box(FL_UP_BOX);
  icon->labelfont(FL_TIMES_BOLD);
  icon->labelsize(34);
  icon->color(FL_WHITE);
  icon->labelcolor(FL_BLUE);

  x += icon->w() + INNER_MARGIN;
  y += INNER_MARGIN;

  if (needsUser) {
    y += INPUT_LABEL_OFFSET;
    username = new Fl_Input(x, y,  w()- x - OUTER_MARGIN,
                            INPUT_HEIGHT, _("Username:"));
    username->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    y += INPUT_HEIGHT + INNER_MARGIN;
  } else {
    username = nullptr;
  }

  if (needsPassword) {
    y += INPUT_LABEL_OFFSET;
    passwd = new Fl_Secret_Input(x, y,  w()- x - OUTER_MARGIN,
                                INPUT_HEIGHT, _("Password:"));
    passwd->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    y += INPUT_HEIGHT + INNER_MARGIN;

    if (reconnectOnError) {
      keepPasswdCheckbox = new Fl_Check_Button(LBLRIGHT(x, y,
                                                        CHECK_MIN_WIDTH,
                                                        CHECK_HEIGHT,
                                                        _("Keep password for reconnect")));
      y += CHECK_HEIGHT + INNER_MARGIN;
    } else {
      keepPasswdCheckbox = nullptr;
    }
  } else {
    passwd = nullptr;
    keepPasswdCheckbox = nullptr;
  }

  x = w() - OUTER_MARGIN;
  y += OUTER_MARGIN - INNER_MARGIN;

  x -= BUTTON_WIDTH;
  button = new Fl_Return_Button(x, y, BUTTON_WIDTH,
                                BUTTON_HEIGHT, fl_ok);
  button->callback(button_cb, 1);
  x -= INNER_MARGIN;

  x -= BUTTON_WIDTH;
  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, fl_cancel);
  button->callback(button_cb, 0);
  button->shortcut(FL_Escape);
  x -= INNER_MARGIN;

  y += BUTTON_HEIGHT;

  y += OUTER_MARGIN;

  end();

  size(w(), y);

  set_modal();
}

AuthDialog::~AuthDialog()
{
}

int AuthDialog::result()
{
  return result_;
}

std::string AuthDialog::getUser()
{
  return username->value();
}

std::string AuthDialog::getPassword()
{
  if (passwd)
    return passwd->value();
  return "";
}

bool AuthDialog::getKeepPassword()
{
  if (keepPasswdCheckbox)
    return keepPasswdCheckbox->value();
  return false;
}

void AuthDialog::button_cb(Fl_Widget *w, long val)
{
  AuthDialog* self;

  self = dynamic_cast<AuthDialog*>(w->window());
  assert(self != nullptr);

  self->result_ = val;
  self->hide();
}
