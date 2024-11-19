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
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Pixmap.H>

#include <core/Exception.h>

#include <rfb/CConnection.h>
#include <rfb/Exception.h>
#include <rfb/obfuscate.h>

#include "fltk/layout.h"
#include "fltk/util.h"
#include "i18n.h"
#include "parameters.h"
#include "UserDialog.h"

#include "AuthDialog.h"

std::string UserDialog::savedUsername;
std::string UserDialog::savedPassword;

UserDialog::UserDialog()
{
}

UserDialog::~UserDialog()
{
}

void UserDialog::resetPassword()
{
  savedUsername.clear();
  savedPassword.clear();
}

void UserDialog::getUserPasswd(bool secure_, std::string* user,
                               std::string* password)
{
  const char *passwordFileName(passwordFile);
  int ret_val;

  assert(password);
  char *envUsername = getenv("VNC_USERNAME");
  char *envPassword = getenv("VNC_PASSWORD");

  if(user && envUsername && envPassword) {
    *user = envUsername;
    *password = envPassword;
    return;
  }

  if (!user && envPassword) {
    *password = envPassword;
    return;
  }

  if (user && !savedUsername.empty() && !savedPassword.empty()) {
    *user = savedUsername;
    *password = savedPassword;
    return;
  }

  if (!user && !savedPassword.empty()) {
    *password = savedPassword;
    return;
  }

  if (!user && passwordFileName[0]) {
    std::vector<uint8_t> obfPwd(8);
    FILE* fp;

    fp = fopen(passwordFileName, "rb");
    if (!fp)
      throw core::posix_error(_("Opening password file failed"), errno);

    obfPwd.resize(fread(obfPwd.data(), 1, obfPwd.size(), fp));
    fclose(fp);

    *password = rfb::deobfuscate(obfPwd.data(), obfPwd.size());

    return;
  }

  AuthDialog d(secure_, user != nullptr, password != nullptr);
  d.show();
  while (d.shown())
    Fl::wait();
  ret_val = d.result();

  if (ret_val == 1) {
    bool keepPasswd;

    if (reconnectOnError)
      keepPasswd = d.getKeepPassword();
    else
      keepPasswd = false;

    if (user) {
      *user = d.getUser();
      if (keepPasswd)
        savedUsername = d.getUser();
    }
    *password = d.getPassword();
    if (keepPasswd)
      savedPassword = d.getPassword();
  }

  if (ret_val != 1)
    throw rfb::auth_cancelled();
}

bool UserDialog::showMsgBox(rfb::MsgBoxFlags flags,
                            const char* title, const char* text)
{
  char buffer[1024];

  if (fltk_escape(text, buffer, sizeof(buffer)) >= sizeof(buffer))
    return 0;

  // FLTK doesn't give us a flexible choice of the icon, so we ignore those
  // bits for now.

  fl_message_title(title);

  switch (flags & 0xf) {
  case rfb::M_OKCANCEL:
    return fl_choice("%s", nullptr, fl_ok, fl_cancel, buffer) == 1;
  case rfb::M_YESNO:
    return fl_choice("%s", nullptr, fl_yes, fl_no, buffer) == 1;
  case rfb::M_OK:
  default:
    if (((flags & 0xf0) == rfb::M_ICONERROR) ||
        ((flags & 0xf0) == rfb::M_ICONWARNING))
      fl_alert("%s", buffer);
    else
      fl_message("%s", buffer);
    return true;
  }

  return false;
}
