/* 
 * Copyright (C) 2006 OCCAM Financial Technology
 * Copyright (C) 2005-2006 Martin Koegler
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

#include <rfb/Exception.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/CConnection.h>
#include <rfb/CSecurityTLS.h>
#include <rfb/CSecurityVeNCrypt.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/LogWriter.h>
#include <rfb/SSecurityVeNCrypt.h>
#include <list>

using namespace rfb;
using namespace rdr;
using namespace std;

static LogWriter vlog("CVeNCrypt");

CSecurityVeNCrypt::CSecurityVeNCrypt(Security* sec) : csecurity(NULL), security(sec)
{
  haveRecvdMajorVersion = false;
  haveRecvdMinorVersion = false;
  haveSentVersion = false;
  haveAgreedVersion = false;
  haveListOfTypes = false;
  haveNumberOfTypes = false;
  haveChosenType = false;
  majorVersion = 0;
  minorVersion = 0;
  chosenType = secTypeVeNCrypt;
  nAvailableTypes = 0;
  availableTypes = NULL;
  iAvailableType = 0;
}

CSecurityVeNCrypt::~CSecurityVeNCrypt()
{
  if (availableTypes)
	delete[] availableTypes;
}

bool CSecurityVeNCrypt::processMsg(CConnection* cc)
{
  InStream* is = cc->getInStream();
  OutStream* os = cc->getOutStream();
	
  /* get major, minor versions, send what we can support (or 0.0 for can't support it) */
  if (!haveRecvdMajorVersion) {
    majorVersion = is->readU8();
    haveRecvdMajorVersion = true;

    return false;
  }

  if (!haveRecvdMinorVersion) {
    minorVersion = is->readU8();
    haveRecvdMinorVersion = true;
  }

  /* major version in upper 8 bits and minor version in lower 8 bits */
  U16 Version = (((U16) majorVersion) << 8) | ((U16) minorVersion);
  
  if (!haveSentVersion) {
    /* Currently we don't support former VeNCrypt 0.1 */
    if (Version >= 0x0002) {
      majorVersion = 0;
      minorVersion = 2;
      os->writeU8(majorVersion);
      os->writeU8(minorVersion);
      os->flush();
    } else {
      /* Send 0.0 to indicate no support */
      majorVersion = 0;
      minorVersion = 0;
      os->writeU8(0);
      os->writeU8(0);
      os->flush();
      throw AuthFailureException("The server reported an unsupported VeNCrypt version");
     }

     haveSentVersion = true;
     return false;
  }

  /* Check that the server is OK */
  if (!haveAgreedVersion) {
    if (is->readU8())
      throw AuthFailureException("The server reported it could not support the "
				 "VeNCrypt version");

    haveAgreedVersion = true;
    return false;
  }
  
  /* get a number of types */
  if (!haveNumberOfTypes) {
    nAvailableTypes = is->readU8();
    iAvailableType = 0;

    if (!nAvailableTypes)
      throw AuthFailureException("The server reported no VeNCrypt sub-types");

    availableTypes = new rdr::U32[nAvailableTypes];
    haveNumberOfTypes = true;
    return false;
  }

  if (nAvailableTypes) {
    /* read in the types possible */
    if (!haveListOfTypes) {
      if (is->checkNoWait(4)) {
	availableTypes[iAvailableType++] = is->readU32();
	haveListOfTypes = (iAvailableType >= nAvailableTypes);
	vlog.debug("Server offers security type %s (%d)",
		   secTypeName(availableTypes[iAvailableType - 1]),
		   availableTypes[iAvailableType - 1]);

	if (!haveListOfTypes)
	  return false;

      } else
	return false;
    }

    /* make a choice and send it to the server, meanwhile set up the stack */
    if (!haveChosenType) {
      chosenType = 0;
      U8 i;
      list<U32>::iterator j;
      list<U32> preferredList;

      /* Try preferred choice */
      SSecurityVeNCrypt::getSecTypes(&preferredList);
	  
      for (j = preferredList.begin(); j != preferredList.end(); j++) {
	for (i = 0; i < nAvailableTypes; i++) {
	  if (*j == availableTypes[i]) {
	    chosenType = *j;
	    break;
	  }
	}

	if (chosenType)
	  break;
      }

      vlog.debug("Choosing security type %s (%d)", secTypeName(chosenType),
		 chosenType);
      /* Set up the stack according to the chosen type: */
      switch (chosenType) {
	case secTypeTLSNone:
	case secTypeTLSVnc:
	case secTypeTLSPlain:
	case secTypeX509None:
	case secTypeX509Vnc:
	case secTypeX509Plain:
	  csecurity = CSecurityVeNCrypt::getCSecurityStack(chosenType);
	  break;

	case secTypeInvalid:
	case secTypeVeNCrypt: /* would cause looping */
	default:
	  throw AuthFailureException("No valid VeNCrypt sub-type");
      }
      
      /* send chosen type to server */
      os->writeU32(chosenType);
      os->flush();

      haveChosenType = true;
    }
  } else {
    /*
     * Server told us that there are 0 types it can support - this should not
     * happen, since if the server supports 0 sub-types, it doesn't support
     * this security type
     */
    throw AuthFailureException("The server reported 0 VeNCrypt sub-types");
  }

  return csecurity->processMsg(cc);
}

CSecurityStack* CSecurityVeNCrypt::getCSecurityStack(int secType)
{
  switch (secType) {
  case secTypeTLSNone:
    return new CSecurityStack(secTypeTLSNone, "TLS with no password",
			      new CSecurityTLS());
  case secTypeTLSVnc:
    return new CSecurityStack(secTypeTLSVnc, "TLS with VNCAuth",
			      new CSecurityTLS(), new CSecurityVncAuth());
#if 0
  /* Following subtypes are not implemented, yet */
  case secTypeTLSPlain:
  case secTypeX509None:
  case secTypeX509Vnc:
  case secTypeX509Plain:
#endif
  default:
    throw Exception("Unsupported VeNCrypt subtype");
  }

  return NULL; /* not reached */
}
