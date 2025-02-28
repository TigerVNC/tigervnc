/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

// -=- QueryConnectDialog.h

#ifndef __WINVNC_QUERY_CONNECT_DIALOG_H__
#define __WINVNC_QUERY_CONNECT_DIALOG_H__

#include <core/Thread.h>

#include <rfb_win32/Dialog.h>

namespace network { class Socket; }

namespace winvnc {

  class VNCServerWin32;

  class QueryConnectDialog : public core::Thread, rfb::win32::Dialog {
  public:
    QueryConnectDialog(network::Socket* sock, const char* userName, VNCServerWin32* s);
    virtual void startDialog();
    network::Socket* getSock() {return sock;}
    bool isAccepted() const {return approve;}
  protected:
    // Thread methods
    void worker() override;

    // Dialog methods (protected)
    void initDialog() override;
    BOOL dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

    // Custom internal methods
    void setCountdownLabel();

    int countdown;
    network::Socket* sock;
    std::string peerIp;
    std::string userName;
    bool approve;
    VNCServerWin32* server;
  };

};

#endif
