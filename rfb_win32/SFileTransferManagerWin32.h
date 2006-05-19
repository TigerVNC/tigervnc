/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky.
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- SFileTransferManagerWin32.h

#ifndef __RFB_WIN32_SFILETRANSFERMANAGERWIN32_H__
#define __RFB_WIN32_SFILETRANSFERMANAGERWIN32_H__

#include <rfb/SFileTransfer.h>
#include <rfb/SFileTransferManager.h>
#include <rfb_win32/SFileTransferWin32.h>

namespace rfb {
  namespace win32 {
    class SFileTransferManagerWin32 : public rfb::SFileTransferManager
    {
    public:
      SFileTransferManagerWin32();
      virtual ~SFileTransferManagerWin32();

      void processDownloadMsg(MSG msg);

      virtual SFileTransfer *createObject(network::Socket *sock);
    };
  };
}

#endif // __RFB_WIN32_SFILETRANSFERMANAGERWIN32_H__
