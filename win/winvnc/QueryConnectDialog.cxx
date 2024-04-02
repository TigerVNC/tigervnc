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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <winvnc/VNCServerWin32.h>
#include <winvnc/QueryConnectDialog.h>
#include <winvnc/resource.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/Service.h>
#include <rfb/LogWriter.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;

static LogWriter vlog("QueryConnectDialog");

static IntParameter timeout("QueryConnectTimeout",
                            "Number of seconds to show the Accept Connection dialog before "
                            "rejecting the connection",
                            10);


// - Visible methods

QueryConnectDialog::QueryConnectDialog(network::Socket* sock_,
                                       const char* userName_,
                                       VNCServerWin32* s)
: Dialog(GetModuleHandle(nullptr)),
  sock(sock_), peerIp(sock->getPeerAddress()), userName(userName_),
  approve(false), server(s) {
}

void QueryConnectDialog::startDialog() {
  start();
}


// - Thread overrides

void QueryConnectDialog::worker() {
  countdown = timeout;
  try {
    if (desktopChangeRequired() && !changeDesktop())
      throw rdr::Exception("changeDesktop failed");
    approve = Dialog::showDialog(MAKEINTRESOURCE(IDD_QUERY_CONNECT));
    server->queryConnectionComplete();
  } catch (...) {
    server->queryConnectionComplete();
    throw;
  }
}


// - Dialog overrides

void QueryConnectDialog::initDialog() {
  if (!SetTimer(handle, 1, 1000, nullptr))
    throw rdr::SystemException("SetTimer", GetLastError());
  setItemString(IDC_QUERY_HOST, peerIp.c_str());
  if (userName.empty())
    userName = "(anonymous)";
  setItemString(IDC_QUERY_USER, userName.c_str());
  setCountdownLabel();
}

void QueryConnectDialog::setCountdownLabel() {
  char buf[16];
  sprintf(buf, "%d", countdown);
  setItemString(IDC_QUERY_COUNTDOWN, buf);
}

BOOL QueryConnectDialog::dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_TIMER) {
    if (--countdown == 0 || desktopChangeRequired()) {
      DestroyWindow(hwnd);
    } else {
      setCountdownLabel();
    }
    return TRUE;
  } else {
    return Dialog::dialogProc(hwnd, msg, wParam, lParam);
  }
}
