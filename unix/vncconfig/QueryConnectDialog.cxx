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

#include <stdio.h>

#include <algorithm>

#include "QueryConnectDialog.h"
#include "vncExt.h"

QueryConnectDialog::QueryConnectDialog(Display* dpy_,
                                       const char* address_,
                                       const char* user_,
                                       int timeout_,
                                       QueryResultCallback* cb)
  : TXDialog(dpy_, 300, 100, "VNC server : Accept connection?"),
    addressLbl(dpy, "Host:",this),
    address(dpy, address_, this),
    userLbl(dpy, "User:", this),
    user(dpy, user_, this),
    timeoutLbl(dpy, "Seconds until automatic reject:", this),
    timeout(dpy, "0000000000", this),
    accept(dpy, "Accept", this, this, 60),
    reject(dpy, "Reject", this, this, 60),
    callback(cb), timeUntilReject(timeout_), timer(this)
{
  const int pad = 4;
  int y=pad;
  int lblWidth = std::max(addressLbl.width(), userLbl.width());
  userLbl.move(pad+lblWidth-userLbl.width(), y);
  user.move(pad+lblWidth, y);
  addressLbl.move(pad+lblWidth-addressLbl.width(), y+=userLbl.height());
  address.move(pad+lblWidth, y);
  timeoutLbl.move(pad, y+=addressLbl.height());
  timeout.move(pad+timeoutLbl.width(), y);
  accept.move(pad, y+=addressLbl.height());
  int maxWidth = std::max(user.width(), address.width()+pad+lblWidth);
  maxWidth = std::max(maxWidth, accept.width()*3);
  maxWidth = std::max(maxWidth, timeoutLbl.width()+timeout.width()+pad);
  reject.move(maxWidth-reject.width(), y);
  resize(maxWidth + pad, y+reject.height()+pad);
  setBorderWidth(1);
  refreshTimeout();
  timer.start(1000);
}

void QueryConnectDialog::deleteWindow(TXWindow*) {
  unmap();
  callback->queryRejected();
}

void QueryConnectDialog::buttonActivate(TXButton* b) {
  unmap();
  if (b == &accept)
    callback->queryApproved();
  else if (b == &reject)
    callback->queryRejected();
}
  
void QueryConnectDialog::handleTimeout(core::Timer* t)
{
  if (timeUntilReject-- == 0) {
    unmap();
    callback->queryTimedOut();
  } else {
    refreshTimeout();
    t->repeat();
  }
}

void QueryConnectDialog::refreshTimeout() {
  char buf[16];
  sprintf(buf, "%d", timeUntilReject);
  timeout.setText(buf);
}
