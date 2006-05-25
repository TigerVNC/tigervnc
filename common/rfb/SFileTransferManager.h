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

// -=- SFileTransferManager.h

#ifndef __RFB_SFILETRANSFERMANAGER_H__
#define __RFB_SFILETRANSFERMANAGER_H__

#include <list>

#include <rfb/SFileTransfer.h>
#include <network/Socket.h>

namespace rfb {
  class SFileTransferManager
  {
  public:
    SFileTransferManager();
    virtual ~SFileTransferManager();

    virtual SFileTransfer *createObject(network::Socket *sock) = 0;
    void destroyObject(SFileTransfer *pFT);

  protected:
    std::list<SFileTransfer*> m_lstFTObjects;

    void destroy();
  };
}

#endif // __RFB_SFILETRANSFERMANAGER_H__
