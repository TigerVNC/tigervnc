/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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
// SSecurityFactoryStandard
//

#include <rfb/secTypes.h>
#include <rfb/SSecurityNone.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/SSecurityFactoryStandard.h>

using namespace rfb;

static LogWriter vlog("SSecurityFactoryStandard");

VncAuthPasswdParameter* SSecurityFactoryStandard::vncAuthPasswd = 0;


SSecurity* SSecurityFactoryStandard::getSSecurity(int secType, bool noAuth) {
  switch (secType) {
  case secTypeNone:    return new SSecurityNone();
  case secTypeVncAuth:
    if (!vncAuthPasswd)
      throw rdr::Exception("No VncAuthPasswdParameter defined!");
    return new SSecurityVncAuth(vncAuthPasswd);
  default:
    throw Exception("Unsupported secType?");
  }
}

VncAuthPasswdParameter::VncAuthPasswdParameter() {
  if (SSecurityFactoryStandard::vncAuthPasswd)
    throw rdr::Exception("duplicate VncAuthPasswdParameter!");
  SSecurityFactoryStandard::vncAuthPasswd = this;
}


VncAuthPasswdConfigParameter::VncAuthPasswdConfigParameter()
: passwdParam("Password",
   "Obfuscated binary encoding of the password which clients must supply to "
   "access the server", 0, 0) {
}

char* VncAuthPasswdConfigParameter::getVncAuthPasswd() {
  CharArray obfuscated;
  int len;
  passwdParam.getData((void**)&obfuscated.buf, &len);
  printf("vnc password len=%d\n", len); // ***
  if (len == 8) {
    CharArray password(9);
    memcpy(password.buf, obfuscated.buf, 8);
    vncAuthUnobfuscatePasswd(password.buf);
    return password.takeBuf();
  }
  return 0;
}


VncAuthPasswdFileParameter::VncAuthPasswdFileParameter()
  : param("PasswordFile", "Password file for VNC authentication", "") {
}

char* VncAuthPasswdFileParameter::getVncAuthPasswd() {
  CharArray fname(param.getData());
  if (!fname.buf[0]) {
    vlog.error("passwordFile parameter not set");
    return 0;
  }
  FILE* fp = fopen(fname.buf, "r");
  if (!fp) {
    vlog.error("opening password file '%s' failed",fname.buf);
    return 0;
  }
  CharArray passwd(9);
  int len = fread(passwd.buf, 1, 9, fp);
  fclose(fp);
  if (len != 8) {
    vlog.error("password file '%s' is the wrong length",fname.buf);
    return 0;
  }
  vncAuthUnobfuscatePasswd(passwd.buf);
  return passwd.takeBuf();
}

