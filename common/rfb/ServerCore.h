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

// -=- ServerCore.h

// This header will define the Server interface, from which ServerMT and
// ServerST will be derived.

#ifndef __RFB_SERVER_CORE_H__
#define __RFB_SERVER_CORE_H__

#include <core/Configuration.h>

namespace rfb {

  class Server {
  public:

    static core::IntParameter idleTimeout;
    static core::IntParameter maxDisconnectionTime;
    static core::IntParameter maxConnectionTime;
    static core::IntParameter maxIdleTime;
    static core::IntParameter compareFB;
    static core::IntParameter frameRate;
    static core::BoolParameter protocol3_3;
    static core::BoolParameter alwaysShared;
    static core::BoolParameter neverShared;
    static core::BoolParameter disconnectClients;
    static core::BoolParameter acceptKeyEvents;
    static core::BoolParameter acceptPointerEvents;
    static core::BoolParameter acceptCutText;
    static core::BoolParameter sendCutText;
    static core::BoolParameter acceptSetDesktopSize;
    static core::BoolParameter queryConnect;

  };

};

#endif // __RFB_SERVER_CORE_H__

