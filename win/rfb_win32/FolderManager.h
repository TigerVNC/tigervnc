/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- FolderManager.h

#ifndef __RFB_WIN32_FOLDERMANAGER_H__
#define __RFB_WIN32_FOLDERMANAGER_H__

#include <windows.h>

#include <rfb/FileInfo.h>
#include <rfb/DirManager.h>

namespace rfb {
  namespace win32{
    class FolderManager : public DirManager {
    public:
      FolderManager();
      ~FolderManager();
      
      bool createDir(char *pFullPath);

      bool renameIt(char *pOldName, char *pNewName);
      bool renameIt(char *pPath, char *pOldName, char *pNewName);
      
      bool deleteIt(char *pPrefix, FileInfo *pFI);
      bool deleteIt(char *pFullPath);
      bool deleteIt(FileInfo *pFI);

      bool getInfo(char *pFullPath, FILEINFO *pFIStruct);
      
      bool getDirInfo(char *pPath, FileInfo *pFileInfo, unsigned int dirOnly);
      bool getDrivesInfo(FileInfo *pFI);

      unsigned int getTime70(FILETIME ftime);
      void getFiletime(unsigned int time70, FILETIME *pftime);

      bool getDirSize(char *pFullPath, DWORD64 *dirSize);

    private:
      bool getFolderInfoWithPrefix(char *pPrefix, FileInfo *pFileInfo);
    };
  }
}

#endif // __RFB_WIN32_FOLDERMANAGER_H__
