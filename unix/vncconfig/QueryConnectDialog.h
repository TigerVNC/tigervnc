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

#ifndef __QUERYCONNECTDIALOG_H__
#define __QUERYCONNECTDIALOG_H__

#include <core/Timer.h>

#include "TXLabel.h"
#include "TXButton.h"
#include "TXDialog.h"

class QueryResultCallback {
 public:
  virtual ~QueryResultCallback() {}
  virtual void queryApproved() = 0;
  virtual void queryRejected() = 0;
  virtual void queryTimedOut() { queryRejected(); };
};

class QueryConnectDialog : public TXDialog, public TXEventHandler,
                           public TXButtonCallback,
                           public core::Timer::Callback
{
 public:
  QueryConnectDialog(Display* dpy, const char* address_,
                     const char* user_, int timeout_,
                     QueryResultCallback* cb);
  void handleEvent(TXWindow*, XEvent* ) override { }
  void deleteWindow(TXWindow*) override;
  void buttonActivate(TXButton* b) override;
  void handleTimeout(core::Timer* t) override;
 private:
  void refreshTimeout();
  TXLabel addressLbl, address, userLbl, user, timeoutLbl, timeout;
  TXButton accept, reject;
  QueryResultCallback* callback;
  int timeUntilReject;
  core::Timer timer;
};

#endif
