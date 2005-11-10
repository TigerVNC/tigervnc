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

// -=- FTMsgWriter.h

#ifndef __RFB_WIN32_FTMSGWRITER_H__
#define __RFB_WIN32_FTMSGWRITER_H__

#include <rdr/types.h>
#include <rdr/OutStream.h>
#include <vncviewer/FileTransfer.h>

namespace rfb {
  namespace win32 {
    class FTMsgWriter
    {
    public:
      FTMsgWriter(rdr::OutStream *pOS);
      ~FTMsgWriter();

      bool writeFileListRqst(unsigned short dirNameSize, char *pDirName,
							 int dest, unsigned char flags);
      
      bool writeFileDownloadCancel(unsigned short reasonLen, char *pReason);
      bool writeFileDownloadRqst(unsigned short filenameLen, char *pFilename, 
                                 unsigned int position);

      bool writeFileUploadData(unsigned short dataSize, char *pData);
      bool writeFileUploadData(unsigned int modTime);
      bool writeFileUploadFailed(unsigned short reasonLen, char *pReason);
      bool writeFileUploadRqst(unsigned short filenameLen, char *pFilename, 
                               unsigned int position);

      bool writeFileCreateDirRqst(unsigned short dirNameLen, char *pDirName);
      bool writeFileDirSizeRqst(unsigned short dirNameLen, char *pDirName, int dest);
      
      bool writeFileRenameRqst(unsigned short oldNameLen, unsigned short newNameLen,
                               char *pOldName, char *pNewName);
      bool writeFileDeleteRqst(unsigned short nameLen, char *pName);

    private:
      rdr::OutStream *m_pOutStream;
    };
  }
}

#endif // __RFB_WIN32_FTMSGWRITER_H__
