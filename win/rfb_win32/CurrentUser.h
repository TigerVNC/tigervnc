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

// CurrentUser.h

// Helper class providing the session's logged on username, if
// a user is logged on.  Also allows processes running under
// XP/2K3 etc to masquerade as the logged on user for security
// purposes

#ifndef __RFB_WIN32_CURRENT_USER_H__
#define __RFB_WIN32_CURRENT_USER_H__

#include <rfb_win32/Handle.h>
#include <rfb_win32/Security.h>

namespace rfb {

  namespace win32 {

    // CurrentUserToken
    //   CurrentUserToken is a Handle containing the security token
    //   for the currently logged-on user, or null if no user is
    //   logged on.
    //
    //   canImpersonate() tests whether there is a user token that is safe
    //   to impersonate.
    //
    //   noUserLoggedOn() tests whether there is *definitely* no user logged on.

    struct CurrentUserToken : public Handle {
      CurrentUserToken();
      bool canImpersonate() const { return h; }
      bool noUserLoggedOn() const { return !h; }
    };

    // ImpersonateCurrentUser
    //   Throws an exception on failure.
    //   Succeeds (trivially) if process is not running as service.
    //   Fails if CurrentUserToken is not valid.
    //   Fails if cannot impersonate token.
    //   Succeeds otherwise.

    struct ImpersonateCurrentUser {
      ImpersonateCurrentUser();
      ~ImpersonateCurrentUser();
      CurrentUserToken token;
    };

    // UserName
    //   Returns the name of the user the thread is currently running as.
    //   Raises a SystemException in case of error.

    struct UserName : public TCharArray {
      UserName();
    };

    // UserSID
    //   Returns the SID of the currently logged-on user (i.e. the session user)

    struct UserSID : public Sid {
      UserSID();
    };

  }

}

#endif
