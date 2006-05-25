/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
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

// -=- SFileTransferManagerWin32.cxx

#include <rfb_win32/SFileTransferManagerWin32.h>

using namespace rfb;
using namespace win32;

SFileTransferManagerWin32::SFileTransferManagerWin32()
{

}

SFileTransferManagerWin32::~SFileTransferManagerWin32()
{

}

SFileTransfer *
SFileTransferManagerWin32::createObject(network::Socket *sock)
{
  rfb::SFileTransfer *pFT = 0;
  rfb::win32::SFileTransferWin32 *pFTWin32 = 0;

  pFTWin32 = new SFileTransferWin32(sock);
  if (pFTWin32 == NULL) return NULL;

  pFT = (SFileTransfer *) pFTWin32;

  m_lstFTObjects.push_front(pFT);

  return pFT;
}

void 
SFileTransferManagerWin32::processDownloadMsg(MSG msg)
{
  SFileTransfer *pFT = (SFileTransfer *)msg.lParam;

  if (pFT != NULL) {
    std::list<SFileTransfer*>::iterator i;
    for (i=m_lstFTObjects.begin(); i!=m_lstFTObjects.end(); i++) {
      if ((*i) == pFT) {
        (*i)->sendFileDownloadPortion();
        return;
      }
    }
  }
}
