/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

#include <rfb_win32/Service.h>
#include <rfb_win32/OSVersion.h>
#include <rfb_win32/Win32Util.h>

namespace rfb {

  namespace win32 {

    // CurrentUserToken
    //   h == 0
    //     if platform is not NT *or* unable to get token
    //     *or* if process is hosting service.
    //   h != 0
    //     if process is hosting service *and*
    //     if platform is NT *and* able to get token.
    //   isValid() == true
    //     if platform is not NT *or* token is valid.

    struct CurrentUserToken : public Handle {
      CurrentUserToken();
      bool isValid() const {return isValid_;};
    private:
      bool isValid_;
    };

    // ImpersonateCurrentUser
    //   Throws an exception on failure.
    //   Succeeds (trivially) if process is not running as service.
    //   Fails if CurrentUserToken is not valid.
    //   Fails if platform is NT AND cannot impersonate token.
    //   Succeeds otherwise.

    struct ImpersonateCurrentUser {
      ImpersonateCurrentUser();
      ~ImpersonateCurrentUser();
      CurrentUserToken token;
    };

    // UserName
    //   Returns the name of the user the thread is currently running as.

    struct UserName : public TCharArray {
      UserName();
    };

  }

}

#endif
