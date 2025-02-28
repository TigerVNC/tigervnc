/*
 * Copyright (C) 2005-2006 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
 * Copyright (C) 2010 TigerVNC Team
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
// SSecurityVeNCrypt
//

#ifndef __SSECURITYVENCRYPT_H__
#define __SSECURITYVENCRYPT_H__

#include <rfb/SSecurity.h>

namespace rfb {

  class SConnection;
  class SecurityServer;

  class SSecurityVeNCrypt : public SSecurity {
  public:
    SSecurityVeNCrypt(SConnection* sc, SecurityServer *sec);
    ~SSecurityVeNCrypt();
    bool processMsg() override;
    int getType() const override { return chosenType; }
    const char* getUserName() const override;
    AccessRights getAccessRights() const override;

  protected:
    SSecurity *ssecurity;
    SecurityServer *security;
    bool haveSentVersion, haveRecvdMajorVersion, haveRecvdMinorVersion;
    bool haveSentTypes, haveChosenType;
    uint8_t majorVersion, minorVersion, numTypes;
    uint32_t *subTypes, chosenType;
  };
}
#endif
