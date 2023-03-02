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

#include <stdint.h>

#include <rfb/Configuration.h>

#include <list>

namespace rfb {
  const uint8_t secTypeInvalid    = 0;
  const uint8_t secTypeNone       = 1;
  const uint8_t secTypeVncAuth    = 2;

  const uint8_t secTypeRA2        = 5;
  const uint8_t secTypeRA2ne      = 6;

  const uint8_t secTypeSSPI       = 7;
  const uint8_t secTypeSSPIne     = 8;

  const uint8_t secTypeTight      = 16;
  const uint8_t secTypeUltra      = 17;
  const uint8_t secTypeTLS        = 18;
  const uint8_t secTypeVeNCrypt   = 19;

  const uint8_t secTypeDH         = 30;

  const uint8_t secTypeMSLogonII  = 113;

  const uint8_t secTypeRA256      = 129;
  const uint8_t secTypeRAne256    = 130;

  /* VeNCrypt subtypes */
  const int secTypePlain          = 256;
  const int secTypeTLSNone        = 257;
  const int secTypeTLSVnc         = 258;
  const int secTypeTLSPlain       = 259;
  const int secTypeX509None       = 260;
  const int secTypeX509Vnc        = 261;
  const int secTypeX509Plain      = 262;

  /* RSA-AES subtypes */
  const int secTypeRA2UserPass    = 1;
  const int secTypeRA2Pass        = 2;

  // result types

  const uint32_t secResultOK = 0;
  const uint32_t secResultFailed = 1;
  const uint32_t secResultTooMany = 2; // deprecated

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
    void EnableSecType(uint32_t secType);
    void DisableSecType(uint32_t secType) { enabledSecTypes.remove(secType); }

    void SetSecTypes(std::list<uint32_t> &secTypes) { enabledSecTypes = secTypes; }

    /* Check if certain type is supported */
    bool IsSupported(uint32_t secType);

    /* Get list of enabled security types without VeNCrypt subtypes */
    const std::list<uint8_t> GetEnabledSecTypes(void);
    /* Get list of enabled VeNCrypt subtypes */
    const std::list<uint32_t> GetEnabledExtSecTypes(void);

    /* Output char* is stored in static array */
    char *ToString(void);

#ifdef HAVE_GNUTLS
    static StringParameter GnuTLSPriority;
#endif

  private:
    std::list<uint32_t> enabledSecTypes;
  };

  const char* secTypeName(uint32_t num);
  uint32_t secTypeNum(const char* name);
  std::list<uint32_t> parseSecTypes(const char* types);
}

#endif
