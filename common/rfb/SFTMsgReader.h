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

// -=- SFTMsgReader.h

#ifndef __RFB_SFTMSGREADER_H__
#define __RFB_SFTMSGREADER_H__

#include <rdr/InStream.h>
#include <rfb/fttypes.h>

namespace rfb {
  class SFTMsgReader
  {
  public:
    SFTMsgReader(rdr::InStream *pIS);
    ~SFTMsgReader();
    
    bool readFileListRqst(unsigned int *pDirNameSize, char *pDirName, 
                          unsigned int *pFlags);
    
    
    bool readFileDownloadRqst(unsigned int *pFilenameSize, char *pFilename, 
                              unsigned int *pPosition);

    bool readFileUploadRqst(unsigned int *pFilenameSize, char *pFilename, 
                            unsigned int *pPosition);
    
    char *readFileUploadData(unsigned int *pDataSize, unsigned int *pModTime);

    
    bool readFileCreateDirRqst(unsigned int *pDirNameSize, char *pDirName);
    bool readFileDirSizeRqst(unsigned int *pDirNameSize, char *pDirName);
    bool readFileDeleteRqst(unsigned int *pNameSize, char *pName);
    
    bool readFileRenameRqst(unsigned int *pOldNameSize, unsigned int *pNewNameSize,
                            char *pOldName, char *pNewName);

    bool readFileDownloadCancel(unsigned int *pReasonSize, char *pReason);
    bool readFileUploadFailed(unsigned int *pReasonSize, char *pReason);

  private:
    rdr::InStream *m_pIS;

    bool readU8U16StringMsg(unsigned int *pReasonSize, char *pReason);
    bool readU8U16U32StringMsg(unsigned char *pU8, unsigned int *pU16, 
                               unsigned int *pU32, char *pString);
  };
}

#endif // __RFB_SFTMSGREADER_H__
