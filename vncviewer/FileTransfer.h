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

#include <vncviewer/FTDialog.h>

namespace rfb {
  namespace win32 {
    class FTDialog;

    class FileTransfer
    {
    public:
      FileTransfer();
      ~FileTransfer();

      void createFileTransfer();

    private:
      bool m_bFTDlgShown;

      FTDialog *m_pFTDialog;
      
    };
  }
}

#endif // __RFB_WIN32_FILETRANSFER_H__
