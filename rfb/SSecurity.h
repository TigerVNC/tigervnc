/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
// SSecurity - class on the server side for handling security handshaking.  A
// derived class for a particular security type overrides the processMsg()
// method.  processMsg() is called first when the security type has been
// decided on, and will keep being called whenever there is data to read from
// the client until either it returns false, indicating authentication/security
// failure, or it returns with done set to true, to indicate success.
//
// processMsg() must never block (or at least must never block until the client
// has been authenticated) - this is to prevent denial of service attacks.
// Note that the first time processMsg() is called, there is no guarantee that
// there is any data to read from the SConnection's InStream, but subsequent
// calls guarantee there is at least one byte which can be read without
// blocking.
//
// getType() should return the secType value corresponding to the SSecurity
// implementation.
//
// failureMessage_.buf can be set to a string which will be passed to the client
// if processMsg returns false, to indicate the reason for the failure.

#ifndef __RFB_SSECURITY_H__
#define __RFB_SSECURITY_H__

#include <rfb/util.h>

namespace rfb {

  class SConnection;

  class SSecurity {
  public:
    virtual ~SSecurity() {}
    virtual bool processMsg(SConnection* sc, bool* done)=0;
    virtual void destroy() { delete this; }
    virtual int getType() const = 0;

    // getUserName() gets the name of the user attempting authentication.  The
    // storage is owned by the SSecurity object, so a copy must be taken if
    // necessary.  Null may be returned to indicate that there is no user name
    // for this security type.
    virtual const char* getUserName() const = 0;

    virtual const char* failureMessage() {return failureMessage_.buf;}
  protected:
    CharArray failureMessage_;
  };

  // SSecurityFactory creates new SSecurity instances for
  // particular security types.
  // The instances must be destroyed by calling destroy()
  // on them when done.
  class SSecurityFactory {
  public:
    virtual ~SSecurityFactory() {}
    virtual SSecurity* getSSecurity(int secType, bool noAuth=false)=0;
  };

}
#endif
