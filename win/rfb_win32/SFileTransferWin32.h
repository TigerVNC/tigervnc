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

// -=- SFileTransferWin32.h

#ifndef __RFB_SFILETRANSFERWIN32_H__
#define __RFB_SFILETRANSFERWIN32_H__

#include <windows.h>

#include <rfb/SFileTransfer.h>

const UINT VNCM_FT_DOWNLOAD = WM_USER + 2;

namespace rfb {
  namespace win32 {
    class SFileTransferWin32 : public rfb::SFileTransfer
    {
    public:
      SFileTransferWin32(network::Socket *sock);
      virtual ~SFileTransferWin32();

      bool processDownloadCallback();
      virtual bool initDownloadCallback();

      virtual bool convertPathFromNet(char *pszPath);
      virtual bool makeFileList(char *pszPath, FileInfo *pFI, bool bDirOnly);
  
      virtual bool deleteIt(char *pszPath);
      virtual bool renameIt(char *pszOldPath, char *pszNewPath);
      virtual bool createDir(char *pszPath);

      virtual bool getDirSize(char *pszName, unsigned short *pHighSize16, unsigned int *pLowSize32);

    };
  };
}

#endif // __RFB_SFILETRANSFERWIN32_H__
