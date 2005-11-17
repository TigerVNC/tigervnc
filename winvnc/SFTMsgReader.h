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

// -=- SFTMsgReader.h

#ifndef __RFB_WIN32_SFTMSGREADER_H__
#define __RFB_WIN32_SFTMSGREADER_H__

namespace winvnc {
  class SFTMsgReader
  {
  public:
    SFTMsgReader();
    ~SFTMsgReader();
    
    int readFileListRqst(unsigned char *pFlags, char *pDirName, bool bDirOnly);
    
    int readFileDownloadCancel(char *pReason);
    int readFileDownloadRqst(char *pFilename, unsigned int *pPosition);
    
    int readFileUploadData(char *pData);
    int readFileUploadData(unsigned int *pModTime);

    int readFileUploadFailed(char *pReason);
    int readFileUploadRqst(char *pFilename, unsigned int *pPosition);
    
    int readFileCreateDirRqst(char *pDirName);
    int readFileDirSizeRqst(char *pDirName);
    
    int readFileRenameRqst(unsigned short *pOldNameLen, unsigned short *pNewNameLen,
                           char *pOldName, char *pNewName);

    int readFileDeleteRqst(char *pName);

  private:
    bool readBytes(unsigned int dataSize, void *pData);
  };
}

#endif // __RFB_WIN32_SFTMSGREADER_H__
