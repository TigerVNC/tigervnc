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

// -=- FileTransfer.cxx

#include <vncviewer/FileTransfer.h>

using namespace rfb;
using namespace rfb::win32;

FileTransfer::FileTransfer()
{
  m_bFTDlgShown = false;
  m_bInitialized = false;

  m_pFTDialog = new FTDialog(GetModuleHandle(0), this);
  m_pInStream = NULL;
  m_pOutStream = NULL;

  m_szLocalPath[0] = '\0';
  m_szRemotePath[0] = '\0';
  m_szLocalPathTmp[0] = '\0';
  m_szRemotePathTmp[0] = '\0';
}

FileTransfer::~FileTransfer()
{
  if (m_pFTDialog != NULL) {
    delete m_pFTDialog;
    m_pFTDialog = NULL;
  }
}

bool 
FileTransfer::initialize(rdr::InStream *pIS, rdr::OutStream *pOS)
{
  if (m_bInitialized) return false;

  m_pInStream = pIS;
  m_pOutStream = pOS;

  m_bInitialized = true;
  return true;
}

bool 
FileTransfer::show()
{
  if (!m_bInitialized) return false;

  m_bFTDlgShown = m_pFTDialog->createFTDialog();
  return m_bFTDlgShown;
}
