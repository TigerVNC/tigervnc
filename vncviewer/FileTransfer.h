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

// -=- FileTransfer.h

#ifndef __RFB_WIN32_FILETRANSFER_H__
#define __RFB_WIN32_FILETRANSFER_H__

#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <vncviewer/FTDialog.h>

namespace rfb {
  namespace win32 {
    class FTDialog;

    class FileTransfer
    {
    public:
      FileTransfer();
      ~FileTransfer();

      bool initialize(rdr::InStream *pIS, rdr::OutStream *pOS);
      bool show();

    private:
      bool m_bFTDlgShown;
      bool m_bInitialized;

      FTDialog *m_pFTDialog;

      rdr::InStream *m_pInStream;
      rdr::OutStream *m_pOutStream;

      char m_szLocalPath[FT_FILENAME_SIZE];
      char m_szRemotePath[FT_FILENAME_SIZE];
      char m_szLocalPathTmp[FT_FILENAME_SIZE];
      char m_szRemotePathTmp[FT_FILENAME_SIZE];
    };
  }
}

#endif // __RFB_WIN32_FILETRANSFER_H__
