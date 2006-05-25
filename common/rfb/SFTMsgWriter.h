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

// -=- SFTMsgWriter.h

#ifndef __RFB_SFTMSGWRITER_H__
#define __RFB_SFTMSGWRITER_H__

#include <rdr/OutStream.h>
#include <rfb/FileInfo.h>
#include <rfb/msgTypes.h>

namespace rfb {
  class SFTMsgWriter
  {
  public:
    SFTMsgWriter(rdr::OutStream *pOS);
    ~SFTMsgWriter();
    
    bool writeFileListData(unsigned char flags, rfb::FileInfo *pFileInfo);
    bool writeFileDownloadData(unsigned int dataSize, void *pData);
    bool writeFileDownloadData(unsigned int modTime);
    bool writeFileUploadCancel(unsigned int reasonLen, char *pReason);
    bool writeFileDownloadFailed(unsigned int reasonLen, char *pReason);
    bool writeFileDirSizeData(unsigned int dirSizeLow, unsigned short dirSizeHigh);
    bool writeFileLastRqstFailed(unsigned char lastRequest, unsigned short reasonLen, 
                                 char *pReason);

  private:
    rdr::OutStream *m_pOS;

    bool writeU8U16StringMsg(unsigned char p1, unsigned short p2, char *pP3);
  };
}

#endif // __RFB_SFTMSGWRITER_H__
