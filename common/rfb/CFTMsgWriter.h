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

// -=- CFTMsgWriter.h

#ifndef __RFB_CFTMSGWRITER_H__
#define __RFB_CFTMSGWRITER_H__

#include <rdr/types.h>
#include <rdr/OutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/fttypes.h>

namespace rfb {
  class CFTMsgWriter
  {
  public:
    CFTMsgWriter(rdr::OutStream *pOS);
    ~CFTMsgWriter();
    
    bool writeFileListRqst(unsigned short dirnameLen, char *pDirName, bool bDirOnly);
    
    bool writeFileDownloadCancel(unsigned short reasonLen, char *pReason);
    bool writeFileDownloadRqst(unsigned short filenameLen, char *pFilename, 
                               unsigned int position);
    
    bool writeFileUploadData(unsigned short dataSize, char *pData);
    bool writeFileUploadData(unsigned int modTime);
    bool writeFileUploadFailed(unsigned short reasonLen, char *pReason);
    bool writeFileUploadRqst(unsigned short filenameLen, char *pFilename, 
                             unsigned int position);
    
    bool writeFileCreateDirRqst(unsigned short dirNameLen, char *pDirName);
    bool writeFileDirSizeRqst(unsigned short dirNameLen, char *pDirName);
    
    bool writeFileRenameRqst(unsigned short oldNameLen, unsigned short newNameLen,
                             char *pOldName, char *pNewName);
    bool writeFileDeleteRqst(unsigned short nameLen, char *pName);
    
  private:
    rdr::OutStream *m_pOutStream;
    
    bool writeU8U16StringMsg(unsigned short strLength, char *pString);
  };
}

#endif // __RFB_CFTMSGWRITER_H__
