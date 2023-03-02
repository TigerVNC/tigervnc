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
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityStack.h>
#include <rfb/CSecurityVeNCrypt.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/CSecurityPlain.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/SSecurityNone.h>
#include <rfb/SSecurityStack.h>
#include <rfb/SSecurityPlain.h>
#include <rfb/SSecurityVncAuth.h>
#include <rfb/SSecurityVeNCrypt.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#include <rfb/SSecurityTLS.h>
#endif
#include <rfb/util.h>

using namespace rdr;
using namespace rfb;
using namespace std;

static LogWriter vlog("Security");

#ifdef HAVE_GNUTLS
StringParameter Security::GnuTLSPriority("GnuTLSPriority",
  "GnuTLS priority string that controls the TLS session’s handshake algorithms",
  "");
#endif

Security::Security()
{
}

Security::Security(StringParameter &secTypes)
{
  enabledSecTypes = parseSecTypes(secTypes);
}

const std::list<uint8_t> Security::GetEnabledSecTypes(void)
{
  list<uint8_t> result;
  list<uint32_t>::iterator i;

  /* Partial workaround for Vino's stupid behaviour. It doesn't allow
   * the basic authentication types as part of the VeNCrypt handshake,
   * making it impossible for a client to do opportunistic encryption.
   * At least make it possible to connect when encryption is explicitly
   * disabled. */
  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++) {
    if (*i >= 0x100) {
      result.push_back(secTypeVeNCrypt);
      break;
    }
  }

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i < 0x100)
      result.push_back(*i);

  return result;
}

const std::list<uint32_t> Security::GetEnabledExtSecTypes(void)
{
  list<uint32_t> result;
  list<uint32_t>::iterator i;

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i != secTypeVeNCrypt) /* Do not include VeNCrypt type to avoid loops */
      result.push_back(*i);

  return result;
}

void Security::EnableSecType(uint32_t secType)
{
  list<uint32_t>::iterator i;

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i == secType)
      return;

  enabledSecTypes.push_back(secType);
}

bool Security::IsSupported(uint32_t secType)
{
  list<uint32_t>::iterator i;

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++)
    if (*i == secType)
      return true;
  if (secType == secTypeVeNCrypt)
    return true;

  return false;
}

char *Security::ToString(void)
{
  list<uint32_t>::iterator i;
  static char out[128]; /* Should be enough */
  bool firstpass = true;
  const char *name;

  memset(out, 0, sizeof(out));

  for (i = enabledSecTypes.begin(); i != enabledSecTypes.end(); i++) {
    name = secTypeName(*i);
    if (name[0] == '[') /* Unknown security type */
      continue;

    if (!firstpass)
      strncat(out, ",", sizeof(out) - 1);
    else
      firstpass = false;
    strncat(out, name, sizeof(out) - 1);
  }

  return out;
}

uint32_t rfb::secTypeNum(const char* name)
{
  if (strcasecmp(name, "None") == 0)       return secTypeNone;
  if (strcasecmp(name, "VncAuth") == 0)    return secTypeVncAuth;
  if (strcasecmp(name, "Tight") == 0)      return secTypeTight;
  if (strcasecmp(name, "RA2") == 0)        return secTypeRA2;
  if (strcasecmp(name, "RA2ne") == 0)      return secTypeRA2ne;
  if (strcasecmp(name, "RA2_256") == 0)    return secTypeRA256;
  if (strcasecmp(name, "RA2ne_256") == 0)  return secTypeRAne256;
  if (strcasecmp(name, "SSPI") == 0)       return secTypeSSPI;
  if (strcasecmp(name, "SSPIne") == 0)     return secTypeSSPIne;
  if (strcasecmp(name, "VeNCrypt") == 0)   return secTypeVeNCrypt;
  if (strcasecmp(name, "DH") == 0)         return secTypeDH;
  if (strcasecmp(name, "MSLogonII") == 0)  return secTypeMSLogonII;

  /* VeNCrypt subtypes */
  if (strcasecmp(name, "Plain") == 0)      return secTypePlain;
  if (strcasecmp(name, "TLSNone") == 0)    return secTypeTLSNone;
  if (strcasecmp(name, "TLSVnc") == 0)     return secTypeTLSVnc;
  if (strcasecmp(name, "TLSPlain") == 0)   return secTypeTLSPlain;
  if (strcasecmp(name, "X509None") == 0)   return secTypeX509None;
  if (strcasecmp(name, "X509Vnc") == 0)    return secTypeX509Vnc;
  if (strcasecmp(name, "X509Plain") == 0)  return secTypeX509Plain;

  return secTypeInvalid;
}

const char* rfb::secTypeName(uint32_t num)
{
  switch (num) {
  case secTypeNone:       return "None";
  case secTypeVncAuth:    return "VncAuth";
  case secTypeTight:      return "Tight";
  case secTypeRA2:        return "RA2";
  case secTypeRA2ne:      return "RA2ne";
  case secTypeRA256:      return "RA2_256";
  case secTypeRAne256:    return "RA2ne_256";
  case secTypeSSPI:       return "SSPI";
  case secTypeSSPIne:     return "SSPIne";
  case secTypeVeNCrypt:   return "VeNCrypt";
  case secTypeDH:         return "DH";
  case secTypeMSLogonII:  return "MSLogonII";

  /* VeNCrypt subtypes */
  case secTypePlain:      return "Plain";
  case secTypeTLSNone:    return "TLSNone";
  case secTypeTLSVnc:     return "TLSVnc";
  case secTypeTLSPlain:   return "TLSPlain";
  case secTypeX509None:   return "X509None";
  case secTypeX509Vnc:    return "X509Vnc";
  case secTypeX509Plain:  return "X509Plain";
  default:                return "[unknown secType]";
  }
}

std::list<uint32_t> rfb::parseSecTypes(const char* types_)
{
  std::list<uint32_t> result;
  std::vector<std::string> types;
  types = split(types_, ',');
  for (size_t i = 0; i < types.size(); i++) {
    uint32_t typeNum = secTypeNum(types[i].c_str());
    if (typeNum != secTypeInvalid)
      result.push_back(typeNum);
  }
  return result;
}
