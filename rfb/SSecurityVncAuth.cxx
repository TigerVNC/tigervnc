/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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

#include <rfb/SSecurityVncAuth.h>
#include <rdr/RandomStream.h>
#include <rfb/SConnection.h>
#include <rfb/vncAuth.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <string.h>
#include <stdio.h>

using namespace rfb;

static LogWriter vlog("VncAuth");


SSecurityVncAuth::SSecurityVncAuth(VncAuthPasswdGetter* pg_)
  : sentChallenge(false), responsePos(0), pg(pg_)
{
}

bool SSecurityVncAuth::processMsg(SConnection* sc, bool* done)
{
  *done = false;
  rdr::InStream* is = sc->getInStream();
  rdr::OutStream* os = sc->getOutStream();

  if (!sentChallenge) {
    rdr::RandomStream rs;
    rs.readBytes(challenge, vncAuthChallengeSize);
    os->writeBytes(challenge, vncAuthChallengeSize);
    os->flush();
    sentChallenge = true;
    return true;
  }

  if (responsePos >= vncAuthChallengeSize) return false;
  while (is->checkNoWait(1) && responsePos < vncAuthChallengeSize) {
    response[responsePos++] = is->readU8();
  }

  if (responsePos < vncAuthChallengeSize) return true;

  CharArray passwd(pg->getVncAuthPasswd());

  // Beyond this point, there is no more VNCAuth protocol to perform.
  *done = true;

  if (!passwd.buf) {
    failureMessage_.buf = strDup("No password configured for VNC Auth");
    vlog.error(failureMessage_.buf);
    return false;
  }

  vncAuthEncryptChallenge(challenge, passwd.buf);
  memset(passwd.buf, 0, strlen(passwd.buf));

  if (memcmp(challenge, response, vncAuthChallengeSize) != 0) {
    return false;
  }

  return true;
}
