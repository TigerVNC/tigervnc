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
// CSecurityVeNCrypt
//

#ifndef __CSECURITYVENCRYPT_H__
#define __CSECURITYVENCRYPT_H__

#include <stdint.h>

#include <rfb/CSecurity.h>
#include <rfb/SecurityClient.h>

namespace rfb {

  class CSecurityVeNCrypt : public CSecurity {
  public:

    CSecurityVeNCrypt(CConnection* cc, SecurityClient* sec);
    ~CSecurityVeNCrypt();
    bool processMsg() override;
    int getType() const override {return chosenType;}
    bool isSecure() const override;

  protected:
    CSecurity *csecurity;
    SecurityClient *security;
    bool haveRecvdMajorVersion;
    bool haveRecvdMinorVersion;
    bool haveSentVersion;
    bool haveAgreedVersion;
    bool haveListOfTypes;
    bool haveNumberOfTypes;
    bool haveChosenType;
    uint8_t majorVersion, minorVersion;
    uint32_t chosenType;
    uint8_t nAvailableTypes;
    uint32_t *availableTypes;
  };
}
#endif
