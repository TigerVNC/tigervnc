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
// SSecurityVncAuth - legacy VNC authentication protocol.
// The getPasswd call can be overridden if you wish to store
// the VncAuth password in an implementation-specific place.
// Otherwise, the password is read from a BinaryParameter
// called Password.

#ifndef __RFB_SSECURITYVNCAUTH_H__
#define __RFB_SSECURITYVNCAUTH_H__

#include <stdint.h>

#include <rfb/Configuration.h>
#include <rfb/SSecurity.h>
#include <rfb/Security.h>

namespace rfb {

  class VncAuthPasswdGetter {
  public:
    // getVncAuthPasswd() fills buffer of given password and readOnlyPassword.
    // If there was no read only password in the file, readOnlyPassword buffer is null.
    virtual void getVncAuthPasswd(std::string *password, std::string *readOnlyPassword)=0;

    virtual ~VncAuthPasswdGetter() { }
  };

  class VncAuthPasswdParameter : public VncAuthPasswdGetter, BinaryParameter {
  public:
    VncAuthPasswdParameter(const char* name, const char* desc, StringParameter* passwdFile_);
    void getVncAuthPasswd(std::string *password, std::string *readOnlyPassword) override;
  protected:
    StringParameter* passwdFile;
  };

  class SSecurityVncAuth : public SSecurity {
  public:
    SSecurityVncAuth(SConnection* sc);
    bool processMsg() override;
    int getType() const override {return secTypeVncAuth;}
    const char* getUserName() const override {return nullptr;}
    AccessRights getAccessRights() const override { return accessRights; }
    static StringParameter vncAuthPasswdFile;
    static VncAuthPasswdParameter vncAuthPasswd;
  private:
    bool verifyResponse(const char* password);
    enum {vncAuthChallengeSize = 16};
    uint8_t challenge[vncAuthChallengeSize];
    uint8_t response[vncAuthChallengeSize];
    bool sentChallenge;
    VncAuthPasswdGetter* pg;
    AccessRights accessRights;
  };
}
#endif
