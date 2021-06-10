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
/*
 * SSecurityVeNCrypt
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/SSecurityVeNCrypt.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>

using namespace rfb;
using namespace rdr;
using namespace std;

static LogWriter vlog("SVeNCrypt");

SSecurityVeNCrypt::SSecurityVeNCrypt(SConnection* sc, SecurityServer *sec)
  : SSecurity(sc), security(sec)
{
  ssecurity = NULL;
  haveSentVersion = false;
  haveRecvdMajorVersion = false;
  haveRecvdMinorVersion = false;
  majorVersion = 0;
  minorVersion = 0;
  haveSentTypes = false;
  haveChosenType = false;
  chosenType = secTypeVeNCrypt;
  numTypes = 0;
  subTypes = NULL;
}

SSecurityVeNCrypt::~SSecurityVeNCrypt()
{
  delete ssecurity;
  delete [] subTypes;
}

bool SSecurityVeNCrypt::processMsg()
{
  rdr::InStream* is = sc->getInStream();
  rdr::OutStream* os = sc->getOutStream();
  rdr::U8 i;

  /* VeNCrypt initialization */

  /* Send the highest version we can support */
  if (!haveSentVersion) {
    os->writeU8(0);
    os->writeU8(2);
    haveSentVersion = true;
    os->flush();
  }

  /* Receive back highest version that client can support (up to and including ours) */
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

    /* WORD value with major version in upper 8 bits and minor version in lower 8 bits */
    U16 Version = (((U16)majorVersion) << 8) | ((U16)minorVersion);

    switch (Version) {
    case 0x0000: /* 0.0 - The client cannot support us! */
    case 0x0001: /* 0.1 Legacy VeNCrypt, not supported */
      os->writeU8(0xFF); /* This is not OK */
      os->flush();
      throw AuthFailureException("The client cannot support the server's "
				 "VeNCrypt version");

    case 0x0002: /* 0.2 */
      os->writeU8(0); /* OK */
      break;

    default:
      os->writeU8(0xFF); /* Not OK */
      os->flush();
      throw AuthFailureException("The client returned an unsupported VeNCrypt version");
    }
  }

  /*
   * send number of supported VeNCrypt authentication types (U8) followed
   * by authentication types (U32s)
   */
  if (!haveSentTypes) {
    list<U32> listSubTypes;

    listSubTypes = security->GetEnabledExtSecTypes();

    numTypes = listSubTypes.size();
    subTypes = new U32[numTypes];

    for (i = 0; i < numTypes; i++) {
      subTypes[i] = listSubTypes.front();
      listSubTypes.pop_front();
    }

    if (numTypes) { 
      os->writeU8(numTypes);
      for (i = 0; i < numTypes; i++)
	os->writeU32(subTypes[i]);

      os->flush(); 
      haveSentTypes = true;
    } else
      throw AuthFailureException("There are no VeNCrypt sub-types to send to the client");
  }

  /* get type back from client (must be one of the ones we sent) */
  if (!haveChosenType) {
    if (!is->hasData(4))
      return false;

    chosenType = is->readU32();

    for (i = 0; i < numTypes; i++) {
      if (chosenType == subTypes[i]) {
	haveChosenType = true;
	break;
      }
    }

    if (!haveChosenType)
      chosenType = secTypeInvalid;

    vlog.info("Client requests security type %s (%d)", secTypeName(chosenType),
	       chosenType);

    /* Set up the stack according to the chosen type */
    if (chosenType == secTypeInvalid || chosenType == secTypeVeNCrypt)
      throw AuthFailureException("No valid VeNCrypt sub-type");

    ssecurity = security->GetSSecurity(sc, chosenType);
  }

  /* continue processing the messages */
  return ssecurity->processMsg();
}

const char* SSecurityVeNCrypt::getUserName() const
{
  if (ssecurity == NULL)
    return NULL;
  return ssecurity->getUserName();
}

SConnection::AccessRights SSecurityVeNCrypt::getAccessRights() const
{
  if (ssecurity == NULL)
    return SSecurity::getAccessRights();
  return ssecurity->getAccessRights();
}
