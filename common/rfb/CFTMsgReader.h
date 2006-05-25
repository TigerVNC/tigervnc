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

// -=- CFTMsgReader.h

#ifndef __RFB_CFTMSGREADER_H__
#define __RFB_CFTMSGREADER_H__

#include <rdr/InStream.h>
#include <rfb/FileInfo.h>

namespace rfb {
  class CFTMsgReader
  {
  public:
    CFTMsgReader(rdr::InStream *pIS);
    ~CFTMsgReader();
    
    int readFileListData(FileInfo *pFileInfo);
    int readFileDirSizeData(unsigned short *pDirSizeLow16, unsigned int *pDirSizeHigh32);
    
    char *readFileUploadCancel(unsigned int *pReasonSize);
    char *readFileDownloadFailed(unsigned int *pReasonSize);
    char *readFileLastRqstFailed(int *pTypeOfRequest, unsigned int *pReasonSize);
    void *readFileDownloadData(unsigned int *pSize, unsigned int *pModTime);
    
  private:
    rdr::InStream *m_pInStream;
    
    bool createFileInfo(unsigned int numFiles, FileInfo *fi, 
                        SIZEDATAINFO *pSDInfo, char *pFilenames);
    char *readReasonMsg(unsigned int *pReasonSize);
  };
}

#endif // __RFB_CFTMSGREADER_H__
