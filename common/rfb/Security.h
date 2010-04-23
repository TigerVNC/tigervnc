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
#include <rfb/CSecurity.h>
#include <rfb/SSecurity.h>
#include <rfb/UserPasswdGetter.h>

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

  // result types

  const rdr::U32 secResultOK = 0;
  const rdr::U32 secResultFailed = 1;
  const rdr::U32 secResultTooMany = 2; // deprecated

  class Security {
  public:
    /*
     * Create Security instance.
     */
    Security(void);

    /* Enable/Disable certain security type */
    void EnableSecType(rdr::U8 secType);
    void DisableSecType(rdr::U8 secType) { enabledSecTypes.remove(secType); }

    /* Check if certain type is supported */
    bool IsSupported(rdr::U8 secType);

    /* Get list of enabled security types */
    const std::list<rdr::U8>& GetEnabledSecTypes(void)
      { return enabledSecTypes; }

    /* Create server side SSecurity class instance */
    SSecurity* GetSSecurity(rdr::U8 secType);

    /* Create client side CSecurity class instance */
    CSecurity* GetCSecurity(rdr::U8 secType);

    static StringParameter secTypes;

    /*
     * Use variable directly instead of dumb get/set methods. It is used
     * only in viewer-side code and MUST be set by viewer.
     */
    UserPasswdGetter *upg;
  private:
    std::list<rdr::U8> enabledSecTypes;
  };

  const char* secTypeName(rdr::U8 num);
  rdr::U8 secTypeNum(const char* name);
  std::list<rdr::U8> parseSecTypes(const char* types);
}

#endif
