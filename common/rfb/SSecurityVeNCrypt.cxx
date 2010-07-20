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

#ifndef HAVE_GNUTLS
#error "This source should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/SSecurityVeNCrypt.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/SSecurityVncAuth.h>
#include <rfb/SSecurityTLS.h>

using namespace rfb;
using namespace rdr;
using namespace std;

static LogWriter vlog("SVeNCrypt");

StringParameter SSecurityVeNCrypt::X509_CertFile
("x509cert",
 "specifies path to the x509 certificate in PEM format",
 "", ConfServer);

StringParameter SSecurityVeNCrypt::X509_KeyFile
("x509key",
 "specifies path to the key of the x509 certificate in PEM format",
 "", ConfServer);

StringParameter SSecurityVeNCrypt::secTypesStr
("VeNCryptTypes",
 "Specify which security scheme to use for VeNCrypt connections (TLSNone, "
 "TLSVnc, TLSPlain, X509None, X509Vnc, X509Plain)",
 "TLSVnc,TLSPlain,X509Vnc,X509Plain");

SSecurityVeNCrypt::SSecurityVeNCrypt(void)
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
  if (subTypes) {
    delete [] subTypes;
    subTypes = NULL;
  }
}

bool SSecurityVeNCrypt::processMsg(SConnection* sc)
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

    return false;
  }

  /* Receive back highest version that client can support (up to and including ours) */
  if (!haveRecvdMajorVersion) {
    majorVersion = is->readU8();
    haveRecvdMajorVersion = true;

    return false;
  }

  if (!haveRecvdMinorVersion) {
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
    SSecurityVeNCrypt::getSecTypes(&listSubTypes);

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
      return false;
    } else
      throw AuthFailureException("There are no VeNCrypt sub-types to send to the client");
  }

  /* get type back from client (must be one of the ones we sent) */
  if (!haveChosenType) {
    is->check(4);
    chosenType = is->readU32();

    for (i = 0; i < numTypes; i++) {
      if (chosenType == subTypes[i]) {
	haveChosenType = true;
	break;
      }
    }

    if (!haveChosenType)
      chosenType = secTypeInvalid;

    /* Set up the stack according to the chosen type */
    switch(chosenType) {
    case secTypeTLSNone:
    case secTypeTLSVnc:
    case secTypeTLSPlain:
    case secTypeX509None:
    case secTypeX509Vnc:
    case secTypeX509Plain:
      ssecurity = SSecurityVeNCrypt::getSSecurityStack(chosenType);
	break;  
    case secTypeInvalid:
    case secTypeVeNCrypt: /* This would cause looping */
    default:
      throw AuthFailureException("No valid VeNCrypt sub-type");
    }

  }

  /* continue processing the messages */
  return ssecurity->processMsg(sc);
}

SSecurityStack* SSecurityVeNCrypt::getSSecurityStack(int secType)
{
  switch (secType) {
  case secTypeTLSNone:
    return new SSecurityStack(secTypeTLSNone, new SSecurityTLS());
  case secTypeTLSVnc:
    return new SSecurityStack(secTypeTLSVnc, new SSecurityTLS(), new SSecurityVncAuth());
#if 0
  /* Following types are not implemented, yet */
  case secTypeTLSPlain:
  case secTypeX509None:
  case secTypeX509Vnc:
  case secTypeX509Plain:
#endif
  default:
    throw Exception("Bug in the SSecurityVeNCrypt::getSSecurityStack");
  }
}

void SSecurityVeNCrypt::getSecTypes(list<U32>* secTypes)
{
  CharArray types;

  types.buf = SSecurityVeNCrypt::secTypesStr.getData();
  list<U32> configured = SSecurityVeNCrypt::parseSecTypes(types.buf);
  list<U32>::iterator i;
  for (i = configured.begin(); i != configured.end(); i++)
    secTypes->push_back(*i);
}

U32 SSecurityVeNCrypt::secTypeNum(const char *name)
{
  if (strcasecmp(name, "TLSNone") == 0)
    return secTypeTLSNone;
  if (strcasecmp(name, "TLSVnc") == 0)
    return secTypeTLSVnc;
  if (strcasecmp(name, "TLSPlain") == 0)
    return secTypeTLSPlain;
  if (strcasecmp(name, "X509None") == 0)
    return secTypeX509None;
  if (strcasecmp(name, "X509Vnc") == 0)
    return secTypeX509Vnc;
  if (strcasecmp(name, "X509Plain") == 0)
    return secTypeX509Plain;

  return secTypeInvalid;
}

char* SSecurityVeNCrypt::secTypeName(U32 num)
{
  switch (num) {
  case secTypePlain:
    return "Plain";
  case secTypeTLSNone:
    return "TLSNone";
  case secTypeTLSVnc:
    return "TLSVnc";
  case secTypeTLSPlain:
    return "TLSPlain";
  case secTypeX509None:
    return "X509None";
  case secTypeX509Vnc:
    return "X509Vnc";
  case secTypeX509Plain:
    return "X509Plain";
  default:
    return "[unknown secType]";
  }
}

list<U32> SSecurityVeNCrypt::parseSecTypes(const char *secTypes)
{
  list<U32> result;
  CharArray types(strDup(secTypes)), type;
  while (types.buf) {
    strSplit(types.buf, ',', &type.buf, &types.buf);
    int typeNum = SSecurityVeNCrypt::secTypeNum(type.buf);
    if (typeNum != secTypeInvalid)
      result.push_back(typeNum);
  }
  return result;
}


