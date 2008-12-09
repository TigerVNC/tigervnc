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

// -=- SFileTransferWin32.cxx

#include <rfb/msgTypes.h>
#include <rfb_win32/FolderManager.h>
#include <rfb_win32/SFileTransferWin32.h>

using namespace rfb;
using namespace rfb::win32;

SFileTransferWin32::SFileTransferWin32(network::Socket *sock) : SFileTransfer(sock)
{
}

SFileTransferWin32::~SFileTransferWin32()
{
}

bool 
SFileTransferWin32::initDownloadCallback()
{
  PostThreadMessage(GetCurrentThreadId(), VNCM_FT_DOWNLOAD, (WPARAM) 0, (LPARAM) this);
  return true;
}

bool 
SFileTransferWin32::processDownloadCallback()
{
  return sendFileDownloadPortion();
}

bool 
SFileTransferWin32::convertPathFromNet(char *pszPath)
{
  int len = strlen(pszPath);
  if (pszPath[0] == '/') {
    if (len == 1) { 
      pszPath[0] = '\0'; 
      return true; 
    }
  } else {
    return false;
  }

  for(int i = 0; i < (len - 1); i++) {
    if(pszPath[i+1] == '/') pszPath[i+1] = '\\';
    pszPath[i] = pszPath[i+1];
  }

  pszPath[len-1] = '\0';
  return true;
}

bool 
SFileTransferWin32::makeFileList(char *pszPath, FileInfo *pFI, bool bDirOnly)
{
  FolderManager fm;
  if (fm.getDirInfo(pszPath, pFI, bDirOnly)) 
    return true; 
  else 
    return false;
}

bool 
SFileTransferWin32::deleteIt(char *pszPath)
{
  FolderManager fm;

  return fm.deleteIt(pszPath);
}

bool 
SFileTransferWin32::renameIt(char *pszOldPath, char *pszNewPath)
{
  FolderManager fm;

  return fm.renameIt(pszOldPath, pszNewPath);
}

bool 
SFileTransferWin32::createDir(char *pszPath)
{
  FolderManager fm;

  return fm.createDir(pszPath);
}

bool 
SFileTransferWin32::getDirSize(char *pszName, unsigned short *pHighSize16, 
                               unsigned int *pLowSize32)
{
  FolderManager fm;
  DWORD64 dw64DirSize = 0;

  if (!fm.getDirSize(pszName, &dw64DirSize)) return false;

  if (dw64DirSize & 0xFFFF000000000000LL) return false;

  *pHighSize16 = (unsigned short)((dw64DirSize >> 32) & 0xFFFF);
  *pLowSize32 = (unsigned int)(dw64DirSize & 0xFFFFFFFF);

  return true;
}
