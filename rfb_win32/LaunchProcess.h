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

// -=- LaunchProcess.h

// Helper class to launch a names process from the same directory as
// the current process executable resides in.

#ifndef __RFB_WIN32_LAUNCHPROCESS_H__
#define __RFB_WIN32_LAUNCHPROCESS_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    class LaunchProcess {
    public:
      LaunchProcess(const TCHAR* exeName_, const TCHAR* params);
      ~LaunchProcess();

      // If userToken is 0 then starts as current user, otherwise
      // starts as the specified user.  userToken must be a primary token.
      void start(HANDLE userToken);

      // Wait for the process to quit, and close the handles to it.
      void await();

      PROCESS_INFORMATION procInfo;
    protected:
      TCharArray exeName;
      TCharArray params;
    };


  };

};

#endif
