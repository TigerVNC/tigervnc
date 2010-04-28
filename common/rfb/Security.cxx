/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#endif
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityVeNCrypt.h>
#include <rfb/CSecurityVncAuth.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/SSecurityNone.h>
#include <rfb/SSecurityVncAuth.h>
#ifdef HAVE_GNUTLS
#include <rfb/SSecurityVeNCrypt.h>
#endif
#include <rfb/util.h>

using namespace rdr;
using namespace rfb;
using namespace std;

static LogWriter vlog("Security");

UserPasswdGetter *CSecurity::upg = NULL;

StringParameter Security::secTypes
("SecurityTypes",
 "Specify which security scheme to use (None, VncAuth)",
 "VncAuth");

Security::Security(void)
{
  char *secTypesStr = secTypes.getData();

  enabledSecTypes = parseSecTypes(secTypesStr);

  delete secTypesStr;
}

void Security::EnableSecType(U8 secType)
{
  list<U8>::iterator i;

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i == secType)
      return;

  enabledSecTypes.push_back(secType);
}

bool Security::IsSupported(U8 secType)
{
  list<U8>::iterator i;

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i == secType)
      return true;

  return false;
}

SSecurity* Security::GetSSecurity(U8 secType)
{
  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new SSecurityNone();
  case secTypeVncAuth: return new SSecurityVncAuth();
#ifdef HAVE_GNUTLS
  case secTypeVeNCrypt: return new SSecurityVeNCrypt();
#endif
  }

bail:
  throw Exception("Security type not supported");
}

CSecurity* Security::GetCSecurity(U8 secType)
{
  assert (CSecurity::upg != NULL); /* (upg == NULL) means bug in the viewer */

  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new CSecurityNone();
  case secTypeVncAuth: return new CSecurityVncAuth();
#ifdef HAVE_GNUTLS
  case secTypeVeNCrypt: return new CSecurityVeNCrypt();
#endif
  }

bail:
  throw Exception("Security type not supported");
}

rdr::U8 rfb::secTypeNum(const char* name)
{
  if (strcasecmp(name, "None") == 0)       return secTypeNone;
  if (strcasecmp(name, "VncAuth") == 0)    return secTypeVncAuth;
  if (strcasecmp(name, "Tight") == 0)      return secTypeTight;
  if (strcasecmp(name, "RA2") == 0)        return secTypeRA2;
  if (strcasecmp(name, "RA2ne") == 0)      return secTypeRA2ne;
  if (strcasecmp(name, "SSPI") == 0)       return secTypeSSPI;
  if (strcasecmp(name, "SSPIne") == 0)     return secTypeSSPIne;
  if (strcasecmp(name, "VeNCrypt") == 0)   return secTypeVeNCrypt;
  return secTypeInvalid;
}

const char* rfb::secTypeName(rdr::U8 num)
{
  switch (num) {
  case secTypeNone:       return "None";
  case secTypeVncAuth:    return "VncAuth";
  case secTypeTight:      return "Tight";
  case secTypeRA2:        return "RA2";
  case secTypeRA2ne:      return "RA2ne";
  case secTypeSSPI:       return "SSPI";
  case secTypeSSPIne:     return "SSPIne";
  case secTypeVeNCrypt:   return "VeNCrypt";
  default:                return "[unknown secType]";
  }
}

std::list<rdr::U8> rfb::parseSecTypes(const char* types_)
{
  std::list<rdr::U8> result;
  CharArray types(strDup(types_)), type;
  while (types.buf) {
    strSplit(types.buf, ',', &type.buf, &types.buf);
    rdr::U8 typeNum = secTypeNum(type.buf);
    if (typeNum != secTypeInvalid)
      result.push_back(typeNum);
  }
  return result;
}
