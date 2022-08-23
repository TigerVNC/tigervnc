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

#include <rfb/CSecurity.h>
#include <rfb/SecurityClient.h>
#include <rdr/types.h>

namespace rfb {

  class CSecurityVeNCrypt : public CSecurity {
  public:

    CSecurityVeNCrypt(CConnection* cc, SecurityClient* sec);
    ~CSecurityVeNCrypt();
    virtual bool processMsg();
    int getType() const {return chosenType;}
    virtual bool isSecure() const;

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
    rdr::U8 majorVersion, minorVersion;
    rdr::U32 chosenType;
    rdr::U8 nAvailableTypes;
    rdr::U32 *availableTypes;
  };
}
#endif
