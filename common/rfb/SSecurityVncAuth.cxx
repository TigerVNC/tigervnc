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
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/obfuscate.h>
#include <assert.h>
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

SSecurityVncAuth::SSecurityVncAuth(SConnection* sc_)
  : SSecurity(sc_), sentChallenge(false),
    pg(&vncAuthPasswd), accessRights(AccessNone)
{
}

bool SSecurityVncAuth::verifyResponse(const char* password)
{
  uint8_t expectedResponse[vncAuthChallengeSize];

  // Calculate the expected response
  uint8_t key[8];
  int pwdLen = strlen(password);
  for (int i=0; i<8; i++)
    key[i] = i<pwdLen ? password[i] : 0;
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

  std::string passwd, passwdReadOnly;
  pg->getVncAuthPasswd(&passwd, &passwdReadOnly);

  if (passwd.empty())
    throw Exception("No password configured");

  if (verifyResponse(passwd.c_str())) {
    accessRights = AccessDefault;
    return true;
  }

  if (!passwdReadOnly.empty() &&
      verifyResponse(passwdReadOnly.c_str())) {
    accessRights = AccessView;
    return true;
  }

  throw AuthFailureException("Authentication failed");
}

VncAuthPasswdParameter::VncAuthPasswdParameter(const char* name_,
                                               const char* desc,
                                               StringParameter* passwdFile_)
: BinaryParameter(name_, desc, nullptr, 0, ConfServer),
  passwdFile(passwdFile_)
{
}

void VncAuthPasswdParameter::getVncAuthPasswd(std::string *password, std::string *readOnlyPassword) {
  std::vector<uint8_t> obfuscated, obfuscatedReadOnly;
  obfuscated = getData();

  if (obfuscated.size() == 0) {
    if (passwdFile) {
      const char *fname = *passwdFile;
      if (!fname[0]) {
        vlog.info("neither %s nor %s params set", getName(), passwdFile->getName());
        return;
      }

      FILE* fp = fopen(fname, "r");
      if (!fp) {
        vlog.error("opening password file '%s' failed", fname);
        return;
      }

      vlog.debug("reading password file");
      obfuscated.resize(8);
      obfuscated.resize(fread(obfuscated.data(), 1, 8, fp));
      obfuscatedReadOnly.resize(8);
      obfuscatedReadOnly.resize(fread(obfuscatedReadOnly.data(), 1, 8, fp));
      fclose(fp);
    } else {
      vlog.info("%s parameter not set", getName());
    }
  }

  assert(password != nullptr);
  assert(readOnlyPassword != nullptr);

  try {
    *password = deobfuscate(obfuscated.data(), obfuscated.size());
    *readOnlyPassword = deobfuscate(obfuscatedReadOnly.data(), obfuscatedReadOnly.size());
  } catch (...) {
  }
}

