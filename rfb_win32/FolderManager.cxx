/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- FolderManager.cxx

#include <rfb_win32/FolderManager.h>

using namespace rfb;
using namespace rfb::win32;

FolderManager::FolderManager()
{

}

FolderManager::~FolderManager()
{

}
      
bool 
FolderManager::createDir(char *pFullPath)
{
  if (CreateDirectory(pFullPath, NULL) == 0) return false;

  return true;
}

bool 
FolderManager::renameDir(char *pOldName, char *pNewName)
{
  if (MoveFile(pOldName, pNewName)) return true;

  return false;
}

bool 
FolderManager::deleteDir(char *pFullPath)
{
  return false;
}
    
bool 
FolderManager::getDirInfo(char *pPath, FileInfo *pFileInfo, unsigned int dirOnly)
{
  return false;
}
