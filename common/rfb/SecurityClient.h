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

#ifndef __RFB_SECURITYCLIENT_H__
#define __RFB_SECURITYCLIENT_H__

#include <rfb/Configuration.h>
#include <rfb/Security.h>
#include <rfb/CSecurity.h>

namespace rfb {

  class SecurityClient : public Security {
  public:
    SecurityClient(void) : Security(secTypes) {}

    /* Create client side CSecurity class instance */
    CSecurity* GetCSecurity(CConnection* cc, rdr::U32 secType);

    static void setDefaults(void);

    static StringParameter secTypes;
  };

}

#endif
