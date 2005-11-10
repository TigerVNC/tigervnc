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

// -=- FTMsgReader.cxx

#include <vncviewer/FTMsgReader.h>

using namespace rfb;
using namespace rfb::win32;

FTMsgReader::FTMsgReader(rdr::InStream *pIS)
{
  m_pInStream = pIS;
}

FTMsgReader::~FTMsgReader()
{

}

int 
FTMsgReader::readFileListData(FileInfo *pFileInfo)
{
  return 0;  
}

int 
FTMsgReader::readFileDownloadData(char *pFile, unsigned int *pModTime)
{
  return 0;  
}

int 
FTMsgReader::readFileUploadCancel(char *pReason)
{
  return 0;
}

int 
FTMsgReader::readFileDownloadFailed(char *pReason)
{
  return 0;
}

int 
FTMsgReader::readFileDirSizeData(DWORD64 *pdw64DirSize)
{
  return 0;
}

int 
FTMsgReader::readFileLastRqstFailed(int *pTypeOfRequest, char *pReason)
{
  return 0;
}
