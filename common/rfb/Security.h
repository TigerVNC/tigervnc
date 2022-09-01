/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
// secTypes.h - constants for the various security types.
//

#ifndef __RFB_SECTYPES_H__
#define __RFB_SECTYPES_H__

#include <rdr/types.h>
#include <rfb/Configuration.h>

#include <list>

namespace rfb {
  const rdr::U8 secTypeInvalid = 0;
  const rdr::U8 secTypeNone    = 1;
  const rdr::U8 secTypeVncAuth = 2;

  const rdr::U8 secTypeRA2     = 5;
  const rdr::U8 secTypeRA2ne   = 6;

  const rdr::U8 secTypeSSPI    = 7;
  const rdr::U8 secTypeSSPIne  = 8;

  const rdr::U8 secTypeTight   = 16;
  const rdr::U8 secTypeUltra   = 17;
  const rdr::U8 secTypeTLS     = 18;
  const rdr::U8 secTypeVeNCrypt= 19;

  const rdr::U8 secTypeRA256   = 129;
  const rdr::U8 secTypeRAne256 = 130;

  /* VeNCrypt subtypes */
  const int secTypePlain       = 256;
  const int secTypeTLSNone     = 257;
  const int secTypeTLSVnc      = 258;
  const int secTypeTLSPlain    = 259;
  const int secTypeX509None    = 260;
  const int secTypeX509Vnc     = 261;
  const int secTypeX509Plain   = 262;

  /* RSA-AES subtypes */
  const int secTypeRA2UserPass = 1;
  const int secTypeRA2Pass     = 2;

  // result types

  const rdr::U32 secResultOK = 0;
  const rdr::U32 secResultFailed = 1;
  const rdr::U32 secResultTooMany = 2; // deprecated

  class Security {
  public:
    /*
     * Create Security instance.
     */
    Security();
    Security(StringParameter &secTypes);

    /*
     * Note about security types.
     *
     * Although RFB protocol specifies security types as U8 values,
     * we map VeNCrypt subtypes (U32) into the standard security types
     * to simplify user configuration. With this mapping user can configure
     * both VeNCrypt subtypes and security types with only one option.
     */

    /* Enable/Disable certain security type */
    void EnableSecType(rdr::U32 secType);
    void DisableSecType(rdr::U32 secType) { enabledSecTypes.remove(secType); }

    void SetSecTypes(std::list<rdr::U32> &secTypes) { enabledSecTypes = secTypes; }

    /* Check if certain type is supported */
    bool IsSupported(rdr::U32 secType);

    /* Get list of enabled security types without VeNCrypt subtypes */
    const std::list<rdr::U8> GetEnabledSecTypes(void);
    /* Get list of enabled VeNCrypt subtypes */
    const std::list<rdr::U32> GetEnabledExtSecTypes(void);

    /* Output char* is stored in static array */
    char *ToString(void);

#ifdef HAVE_GNUTLS
    static StringParameter GnuTLSPriority;
#endif

  private:
    std::list<rdr::U32> enabledSecTypes;
  };

  const char* secTypeName(rdr::U32 num);
  rdr::U32 secTypeNum(const char* name);
  std::list<rdr::U32> parseSecTypes(const char* types);
}

#endif
