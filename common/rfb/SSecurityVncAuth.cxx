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

SSecurityVncAuth::SSecurityVncAuth(void)
  : sentChallenge(false), responsePos(0), pg(&vncAuthPasswd)
{
}

bool SSecurityVncAuth::processMsg(SConnection* sc)
{
  rdr::InStream* is = sc->getInStream();
  rdr::OutStream* os = sc->getOutStream();

  if (!sentChallenge) {
    rdr::RandomStream rs;
    rs.readBytes(challenge, vncAuthChallengeSize);
    os->writeBytes(challenge, vncAuthChallengeSize);
    os->flush();
    sentChallenge = true;
    return false;
  }

  while (responsePos < vncAuthChallengeSize && is->checkNoWait(1))
    response[responsePos++] = is->readU8();

  if (responsePos < vncAuthChallengeSize) return false;

  PlainPasswd passwd(pg->getVncAuthPasswd());

  if (!passwd.buf)
    throw AuthFailureException("No password configured for VNC Auth");

  // Calculate the expected response
  rdr::U8 key[8];
  int pwdLen = strlen(passwd.buf);
  for (int i=0; i<8; i++)
    key[i] = i<pwdLen ? passwd.buf[i] : 0;
  deskey(key, EN0);
  for (int j = 0; j < vncAuthChallengeSize; j += 8)
    des(challenge+j, challenge+j);

  // Check the actual response
  if (memcmp(challenge, response, vncAuthChallengeSize) != 0)
    throw AuthFailureException();

  return true;
}

VncAuthPasswdParameter::VncAuthPasswdParameter(const char* name,
                                               const char* desc,
                                               StringParameter* passwdFile_)
: BinaryParameter(name, desc, 0, 0, ConfServer), passwdFile(passwdFile_) {
}

char* VncAuthPasswdParameter::getVncAuthPasswd() {
  ObfuscatedPasswd obfuscated;
  getData((void**)&obfuscated.buf, &obfuscated.length);

  if (obfuscated.length == 0) {
    if (passwdFile) {
      CharArray fname(passwdFile->getData());
      if (!fname.buf[0]) {
        vlog.info("neither %s nor %s params set", getName(), passwdFile->getName());
        return 0;
      }

      FILE* fp = fopen(fname.buf, "r");
      if (!fp) {
        vlog.error("opening password file '%s' failed",fname.buf);
        return 0;
      }

      vlog.debug("reading password file");
      obfuscated.buf = new char[128];
      obfuscated.length = fread(obfuscated.buf, 1, 128, fp);
      fclose(fp);
    } else {
      vlog.info("%s parameter not set", getName());
    }
  }

  try {
    PlainPasswd password(obfuscated);
    return password.takeBuf();
  } catch (...) {
    return 0;
  }
}

