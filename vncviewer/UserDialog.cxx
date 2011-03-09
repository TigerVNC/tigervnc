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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <FL/fl_ask.H>

#include <rfb/util.h>
#include <rfb/Password.h>
#include <rfb/Exception.h>

#include "i18n.h"
#include "parameters.h"
#include "UserDialog.h"

using namespace rfb;

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

    fp = fopen(passwordFileStr.buf, "r");
    if (!fp)
      throw rfb::Exception(_("Opening password file failed"));

    obfPwd.length = fread(obfPwd.buf, 1, obfPwd.length, fp);
    fclose(fp);

    PlainPasswd passwd(obfPwd);
    *password = passwd.takeBuf();

    return;
  }

  if (!user) {
    *password = strDup(fl_password(_("VNC authentication"), ""));
    if (!*password)
      throw rfb::Exception(_("Authentication cancelled"));

    return;
  }

  fl_alert(_("NOT IMPLEMENTED!"));

  *user = strDup("");
  *password = strDup("");
}

bool UserDialog::showMsgBox(int flags, const char* title, const char* text)
{
  fl_message_title(title);
  fl_message(text);

  return false;
}
