/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
//
// SSecurityVncAuth
//
// XXX not thread-safe, because d3des isn't - do we need to worry about this?
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/SSecurityVncAuth.h>
#include <rdr/RandomStream.h>
#include <rfb/SConnection.h>
#include <rfb/Password.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/Exception.h>
#include <string.h>
#include <stdio.h>
extern "C" {
#include <rfb/d3des.h>
}


using namespace rfb;

static LogWriter vlog("SVncAuth");

StringParameter SSecurityVncAuth::vncAuthPasswdFile
("PasswordFile", "Password file for VNC authentication", "", ConfServer);
AliasParameter rfbauth("rfbauth", "Alias for PasswordFile",
		       &SSecurityVncAuth::vncAuthPasswdFile, ConfServer);
VncAuthPasswdParameter SSecurityVncAuth::vncAuthPasswd
("Password", "Obfuscated binary encoding of the password which clients must supply to "
 "access the server", &SSecurityVncAuth::vncAuthPasswdFile);

SSecurityVncAuth::SSecurityVncAuth(SConnection* sc)
  : SSecurity(sc), sentChallenge(false),
    pg(&vncAuthPasswd), accessRights(0)
{
}

bool SSecurityVncAuth::verifyResponse(const PlainPasswd &password)
{
  rdr::U8 expectedResponse[vncAuthChallengeSize];

  // Calculate the expected response
  rdr::U8 key[8];
  int pwdLen = strlen(password.buf);
  for (int i=0; i<8; i++)
    key[i] = i<pwdLen ? password.buf[i] : 0;
  deskey(key, EN0);
  for (int j = 0; j < vncAuthChallengeSize; j += 8)
    des(challenge+j, expectedResponse+j);

  // Check the actual response
  return memcmp(response, expectedResponse, vncAuthChallengeSize) == 0;
}

bool SSecurityVncAuth::processMsg()
{
  rdr::InStream* is = sc->getInStream();
  rdr::OutStream* os = sc->getOutStream();

  if (!sentChallenge) {
    rdr::RandomStream rs;
    if (!rs.hasData(vncAuthChallengeSize))
      throw Exception("Could not generate random data for VNC auth challenge");
    rs.readBytes(challenge, vncAuthChallengeSize);
    os->writeBytes(challenge, vncAuthChallengeSize);
    os->flush();
    sentChallenge = true;
    return false;
  }

  if (!is->hasData(vncAuthChallengeSize))
    return false;

  is->readBytes(response, vncAuthChallengeSize);

  PlainPasswd passwd, passwdReadOnly;
  pg->getVncAuthPasswd(&passwd, &passwdReadOnly);

  if (!passwd.buf)
    throw AuthFailureException("No password configured for VNC Auth");

  if (verifyResponse(passwd)) {
    accessRights = SConnection::AccessDefault;
    return true;
  }

  if (passwdReadOnly.buf && verifyResponse(passwdReadOnly)) {
    accessRights = SConnection::AccessView;
    return true;
  }

  throw AuthFailureException();
}

VncAuthPasswdParameter::VncAuthPasswdParameter(const char* name,
                                               const char* desc,
                                               StringParameter* passwdFile_)
: BinaryParameter(name, desc, 0, 0, ConfServer), passwdFile(passwdFile_) {
}

void VncAuthPasswdParameter::getVncAuthPasswd(PlainPasswd *password, PlainPasswd *readOnlyPassword) {
  ObfuscatedPasswd obfuscated, obfuscatedReadOnly;
  getData((void**)&obfuscated.buf, &obfuscated.length);

  if (obfuscated.length == 0) {
    if (passwdFile) {
      CharArray fname(passwdFile->getData());
      if (!fname.buf[0]) {
        vlog.info("neither %s nor %s params set", getName(), passwdFile->getName());
        return;
      }

      FILE* fp = fopen(fname.buf, "r");
      if (!fp) {
        vlog.error("opening password file '%s' failed",fname.buf);
        return;
      }

      vlog.debug("reading password file");
      obfuscated.buf = new char[8];
      obfuscated.length = fread(obfuscated.buf, 1, 8, fp);
      obfuscatedReadOnly.buf = new char[8];
      obfuscatedReadOnly.length = fread(obfuscatedReadOnly.buf, 1, 8, fp);
      fclose(fp);
    } else {
      vlog.info("%s parameter not set", getName());
    }
  }

  try {
    PlainPasswd plainPassword(obfuscated);
    password->replaceBuf(plainPassword.takeBuf());
    PlainPasswd plainPasswordReadOnly(obfuscatedReadOnly);
    readOnlyPassword->replaceBuf(plainPasswordReadOnly.takeBuf());
  } catch (...) {
  }
}

