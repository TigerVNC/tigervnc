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

#include <rfb/SSecurityStack.h>
#include <rfb/SConnection.h>

namespace rfb {

  class SSecurityVeNCrypt : public SSecurity {
  public:
    SSecurityVeNCrypt(SConnection* sc, SecurityServer *sec);
    ~SSecurityVeNCrypt();
    virtual bool processMsg();
    virtual int getType() const { return chosenType; }
    virtual const char* getUserName() const;
    virtual SConnection::AccessRights getAccessRights() const;

  protected:
    SSecurity *ssecurity;
    SecurityServer *security;
    bool haveSentVersion, haveRecvdMajorVersion, haveRecvdMinorVersion;
    bool haveSentTypes, haveChosenType;
    rdr::U8 majorVersion, minorVersion, numTypes;
    rdr::U32 *subTypes, chosenType;
  };
}
#endif
