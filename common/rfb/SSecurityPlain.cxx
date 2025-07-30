/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
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

#include <core/Configuration.h>
#include <core/i18n.h>
#include <core/string.h>

#include <rfb/SSecurityPlain.h>
#include <rfb/SConnection.h>
#include <rfb/Exception.h>
#include <rdr/InStream.h>
#if !defined(WIN32) && !defined(__APPLE__)
#include <rfb/UnixPasswordValidator.h>
#include <unistd.h>
#include <pwd.h>
#endif
#ifdef WIN32
#include <rfb/WinPasswdValidator.h>
#endif

using namespace rfb;

core::StringListParameter PasswordValidator::plainUsers
("PlainUsers",
 _("Users permitted to access the server"),
 {});

bool PasswordValidator::validUser(const char* username)
{
  for (const char* user : plainUsers) {
    if (strcmp(user, "*") == 0)
      return true;
#if !defined(WIN32) && !defined(__APPLE__)
    if (strcmp(user, "%u") == 0) {
      struct passwd *pw = getpwnam(username);
      if (pw && pw->pw_uid == getuid())
        return true;
    }
#endif
    // FIXME: We should compare uid, as the usernames might not be case
    //        sensitive, or have other normalisation
    if (strcmp(user, username) == 0)
      return true;
  }
  return false;
}

SSecurityPlain::SSecurityPlain(SConnection* sc_) : SSecurity(sc_)
{
#ifdef WIN32
  valid = new WinPasswdValidator();
#elif !defined(__APPLE__)
  valid = new UnixPasswordValidator();
#else
  valid = nullptr;
#endif

  state = 0;
}

bool SSecurityPlain::processMsg()
{
  rdr::InStream* is = sc->getInStream();
  char password[1024];

  if (!valid)
    throw std::logic_error(_("No password database configured"));

  if (state == 0) {
    if (!is->hasData(8))
      return false;

    ulen = is->readU32();
    if (ulen >= sizeof(username))
      throw auth_error(_("Username is too long"));

    plen = is->readU32();
    if (plen >= sizeof(password))
      throw auth_error(_("Password is too long"));

    state = 1;
  }

  if (state == 1) {
    if (!is->hasData(ulen + plen))
      return false;
    state = 2;
    is->readBytes((uint8_t*)username, ulen);
    is->readBytes((uint8_t*)password, plen);
    password[plen] = 0;
    username[ulen] = 0;
    plen = 0;
    std::string msg = _("Authentication failed");
    if (!valid->validate(sc, username, password, msg))
      throw auth_error(msg);
  }

  return true;
}

