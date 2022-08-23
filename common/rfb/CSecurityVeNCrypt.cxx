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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/Exception.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/CConnection.h>
#include <rfb/CSecurityVeNCrypt.h>
#include <rfb/LogWriter.h>
#include <list>

using namespace rfb;
using namespace rdr;
using namespace std;

static LogWriter vlog("CVeNCrypt");

CSecurityVeNCrypt::CSecurityVeNCrypt(CConnection* cc, SecurityClient* sec)
  : CSecurity(cc), csecurity(NULL), security(sec)
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
}

CSecurityVeNCrypt::~CSecurityVeNCrypt()
{
  delete[] availableTypes;
  delete csecurity;
}

bool CSecurityVeNCrypt::processMsg()
{
  InStream* is = cc->getInStream();
  OutStream* os = cc->getOutStream();

  /* get major, minor versions, send what we can support (or 0.0 for can't support it) */
  if (!haveRecvdMajorVersion) {
    if (!is->hasData(1))
      return false;

    majorVersion = is->readU8();
    haveRecvdMajorVersion = true;
  }

  if (!haveRecvdMinorVersion) {
    if (!is->hasData(1))
      return false;

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
  }

  /* Check that the server is OK */
  if (!haveAgreedVersion) {
    if (!is->hasData(1))
      return false;

    if (is->readU8())
      throw AuthFailureException("The server reported it could not support the "
				 "VeNCrypt version");

    haveAgreedVersion = true;
  }
  
  /* get a number of types */
  if (!haveNumberOfTypes) {
    if (!is->hasData(1))
      return false;

    nAvailableTypes = is->readU8();

    if (!nAvailableTypes)
      throw AuthFailureException("The server reported no VeNCrypt sub-types");

    availableTypes = new rdr::U32[nAvailableTypes];
    haveNumberOfTypes = true;
  }

  if (nAvailableTypes) {
    /* read in the types possible */
    if (!haveListOfTypes) {
      if (!is->hasData(4 * nAvailableTypes))
        return false;

      for (int i = 0;i < nAvailableTypes;i++) {
        availableTypes[i] = is->readU32();
        vlog.debug("Server offers security type %s (%d)",
                   secTypeName(availableTypes[i]),
                   availableTypes[i]);
      }

      haveListOfTypes = true;
    }

    /* make a choice and send it to the server, meanwhile set up the stack */
    if (!haveChosenType) {
      chosenType = secTypeInvalid;
      U8 i;
      list<U32>::iterator j;
      list<U32> secTypes;

      secTypes = security->GetEnabledExtSecTypes();

      /* Honor server's security type order */
      for (i = 0; i < nAvailableTypes; i++) {
        for (j = secTypes.begin(); j != secTypes.end(); j++) {
	  if (*j == availableTypes[i]) {
	    chosenType = *j;
	    break;
	  }
	}

	if (chosenType != secTypeInvalid)
	  break;
      }

      vlog.info("Choosing security type %s (%d)", secTypeName(chosenType),
		 chosenType);

      /* Set up the stack according to the chosen type: */
      if (chosenType == secTypeInvalid || chosenType == secTypeVeNCrypt)
	throw AuthFailureException("No valid VeNCrypt sub-type");

      csecurity = security->GetCSecurity(cc, chosenType);

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

  return csecurity->processMsg();
}

bool CSecurityVeNCrypt::isSecure() const
{
  if (csecurity && csecurity->isSecure())
    return true;
  return false;
}
