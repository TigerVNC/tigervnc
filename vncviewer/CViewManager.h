/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
 */

// -=- CViewManager.h

// Creates and manages threads to run CView instances.

#ifndef __RFB_WIN32_CVIEW_MANAGER_H__
#define __RFB_WIN32_CVIEW_MANAGER_H__

#include <list>
#include <network/Socket.h>
#include <rfb/Threading.h>
#include <rfb_win32/MsgWindow.h>

namespace rfb {

  namespace win32 {

    class CViewManager : public MsgWindow {
    public:
      CViewManager();
      ~CViewManager();

      void awaitEmpty();

      void addThread(Thread* t);
      void remThread(Thread* t);

      bool addClient(const char* hostinfo, bool isConfigFile=false);

      bool addListener(network::SocketListener* sock);
      bool addDefaultTCPListener(int port);

      LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
      std::list<network::SocketListener*> socks;
      std::list<Thread*> threads;
      Mutex threadsMutex;
      Condition threadsSig;
      Thread* mainThread;
    };

  };

};

#endif
