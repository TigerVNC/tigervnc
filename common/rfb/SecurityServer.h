/* 
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

#ifndef __RFB_SECURITYSERVER_H__
#define __RFB_SECURITYSERVER_H__

#include <rfb/Configuration.h>
#include <rfb/Security.h>

namespace rfb {

  class SConnection;
  class SSecurity;

  class SecurityServer : public Security {
  public:
    SecurityServer(void) : Security(secTypes) {}

    /* Create server side SSecurity class instance */
    SSecurity* GetSSecurity(SConnection* sc, rdr::U32 secType);

    static StringParameter secTypes;
  };

}

#endif
