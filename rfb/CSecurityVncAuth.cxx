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
// CSecurityVncAuth
//

#include <string.h>
#include <stdio.h>
#include <rfb/CConnection.h>
#include <rfb/UserPasswdGetter.h>
#include <rfb/vncAuth.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>

using namespace rfb;

static LogWriter vlog("VncAuth");

CSecurityVncAuth::CSecurityVncAuth(UserPasswdGetter* upg_)
  : upg(upg_)
{
}

CSecurityVncAuth::~CSecurityVncAuth()
{
}

bool CSecurityVncAuth::processMsg(CConnection* cc, bool* done)
{
  *done = false;
  rdr::InStream* is = cc->getInStream();
  rdr::OutStream* os = cc->getOutStream();

  rdr::U8 challenge[vncAuthChallengeSize];
  is->readBytes(challenge, vncAuthChallengeSize);
  CharArray passwd;
  if (!upg->getUserPasswd(0, &passwd.buf)) {
    vlog.error("Getting password failed");
    return false;
  }
  vncAuthEncryptChallenge(challenge, passwd.buf);
  memset(passwd.buf, 0, strlen(passwd.buf));
  os->writeBytes(challenge, vncAuthChallengeSize);
  os->flush();
  *done = true;
  return true;
}
