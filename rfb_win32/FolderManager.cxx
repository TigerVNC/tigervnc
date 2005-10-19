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
FolderManager::getFolderInfo(char *pPath, FileInfo *pFileInfo, unsigned int dirOnly)
{
	if (strlen(pPath) == 0) return getDrivesInfo(pFileInfo);

	char path[FT_FILENAME_SIZE];
	sprintf(path, "%s\\*", pPath);

	WIN32_FIND_DATA FindFileData;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	HANDLE handle = FindFirstFile(path, &FindFileData);
	DWORD lastError = GetLastError();
	SetErrorMode(0);

	if (handle != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(FindFileData.cFileName, ".") != 0 &&
				strcmp(FindFileData.cFileName, "..") != 0) {
				LARGE_INTEGER li;
				li.LowPart = FindFileData.ftLastWriteTime.dwLowDateTime;
				li.HighPart = FindFileData.ftLastWriteTime.dwHighDateTime;							
				li.QuadPart = (li.QuadPart - 116444736000000000) / 10000000;
				if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {	
					pFileInfo->add(FindFileData.cFileName, 0, li.LowPart, FT_ATTR_FOLDER);
				} else {
					if (!dirOnly)
						pFileInfo->add(FindFileData.cFileName, FindFileData.nFileSizeLow, li.LowPart, FT_ATTR_FILE);
				}
			}
			
		} while (FindNextFile(handle, &FindFileData));
	} else {
		return false;
	}
	FindClose(handle);
	return true;
}

bool 
FolderManager::getDrivesInfo(FileInfo *pFileInfo)
{
	TCHAR szDrivesList[256];
	if (GetLogicalDriveStrings(255, szDrivesList) == 0)
		return false;

	int i = 0;
	while (szDrivesList[i] != '\0') {
		char *drive = strdup(&szDrivesList[i]);
		char *backslash = strrchr(drive, '\\');
		if (backslash != NULL)
			*backslash = '\0';
		pFileInfo->add(drive, 0, 0, FT_ATTR_FOLDER);
		free(drive);
		i += strcspn(&szDrivesList[i], "\0") + 1;
	}
	return true;
}
