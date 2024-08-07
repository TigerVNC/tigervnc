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

#include <rfb/SSecurityPlain.h>
#include <rfb/SConnection.h>
#include <rfb/Exception.h>
#include <rfb/util.h>
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

StringParameter PasswordValidator::plainUsers
("PlainUsers",
 "Users permitted to access via Plain security type (including TLSPlain, X509Plain etc.)"
#ifdef HAVE_NETTLE
 " or RSA-AES security types (RA2, RA2ne, RA2_256, RA2ne_256)"
#endif
 ,
 "");

bool PasswordValidator::validUser(const char* username)
{
  std::vector<std::string> users;

  users = split(plainUsers, ',');

  for (size_t i = 0; i < users.size(); i++) {
    if (users[i] == "*")
      return true;
#if !defined(WIN32) && !defined(__APPLE__)
    if (users[i] == "%u") {
      struct passwd *pw = getpwnam(username);
      if (pw && pw->pw_uid == getuid())
        return true;
    }
#endif
    if (users[i] == username)
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
    throw Exception("No password validator configured");

  if (state == 0) {
    if (!is->hasData(8))
      return false;

    ulen = is->readU32();
    if (ulen >= sizeof(username))
      throw AuthFailureException("Too long username");

    plen = is->readU32();
    if (plen >= sizeof(password))
      throw AuthFailureException("Too long password");

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
    if (!valid->validate(sc, username, password))
      throw AuthFailureException("Authentication failed");
  }

  return true;
}

