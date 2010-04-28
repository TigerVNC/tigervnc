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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_GNUTLS
#error "This header should not be included without HAVE_GNUTLS defined"
#endif

#include <rfb/SSecurityStack.h>
#include <rfb/SConnection.h>

namespace rfb {

  /* VeNCrypt subtypes */
  const int secTypePlain	= 256;
  const int secTypeTLSNone	= 257;
  const int secTypeTLSVnc	= 258;
  const int secTypeTLSPlain	= 259;
  const int secTypeX509None	= 260;
  const int secTypeX509Vnc	= 261;
  const int secTypeX509Plain	= 262;

  class SSecurityVeNCrypt : public SSecurity {
  public:
    SSecurityVeNCrypt(void);
    ~SSecurityVeNCrypt();
    virtual bool processMsg(SConnection* sc);// { return true; }
    virtual int getType() const { return secTypeVeNCrypt; }
    virtual const char* getUserName() const { return NULL; }

    static StringParameter X509_CertFile, X509_KeyFile, secTypesStr;

    /* XXX Derive Security class and merge those functions appropriately ? */
    static void getSecTypes(std::list<rdr::U32>* secTypes);
    static rdr::U32 secTypeNum(const char *name);
    static char* secTypeName(rdr::U32 num);
    static std::list<rdr::U32> parseSecTypes(const char *types);
  protected:
    static SSecurityStack* getSSecurityStack(int secType);

    SSecurityStack *ssecurityStack;
    bool haveSentVersion, haveRecvdMajorVersion, haveRecvdMinorVersion;
    bool haveSentTypes, haveChosenType;
    rdr::U8 majorVersion, minorVersion, numTypes;
    rdr::U32 *subTypes, chosenType;
  };
}
#endif
