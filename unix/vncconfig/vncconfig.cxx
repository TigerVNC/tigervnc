/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2012-2016 Pierre Ossman for Cendio AB
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
//
// VNC server configuration utility
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "vncExt.h"
#include <rdr/Exception.h>
#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include "TXWindow.h"
#include "TXCheckbox.h"
#include "TXLabel.h"
#include "QueryConnectDialog.h"

using namespace rfb;

static LogWriter vlog("vncconfig");

StringParameter displayname("display", "The X display", "");
BoolParameter noWindow("nowin", "Don't display a window", 0);
BoolParameter iconic("iconic", "Start with window iconified", 0);

#define ACCEPT_CUT_TEXT "AcceptCutText"
#define SEND_CUT_TEXT "SendCutText"

#define SET_PRIMARY "SetPrimary"
#define SEND_PRIMARY "SendPrimary"

char* programName = 0;
Display* dpy;
int vncExtEventBase, vncExtErrorBase;

static bool getBoolParam(Display* dpy, const char* param) {
  char* data;
  int len;
  if (XVncExtGetParam(dpy, param, &data, &len)) {
    if (strcmp(data,"1") == 0) return true;
  }
  return false;
}

class VncConfigWindow : public TXWindow, public TXEventHandler,
                        public TXDeleteWindowCallback,
                        public TXCheckboxCallback,
                        public QueryResultCallback {
public:
  VncConfigWindow(Display* dpy)
    : TXWindow(dpy, 300, 100),
      acceptClipboard(dpy, "Accept clipboard from viewers", this, false, this),
      setPrimaryCB(dpy, "Also set primary selection", this, false, this),
      sendClipboard(dpy, "Send clipboard to viewers", this, false, this),
      sendPrimaryCB(dpy, "Send primary selection to viewers", this,false,this),
      queryConnectDialog(0)
  {
    int y = yPad;
    acceptClipboard.move(xPad, y);
    acceptClipboard.checked(getBoolParam(dpy, ACCEPT_CUT_TEXT));
    y += acceptClipboard.height();
    setPrimaryCB.move(xPad + 10, y);
    setPrimaryCB.checked(getBoolParam(dpy, SET_PRIMARY));
    setPrimaryCB.disabled(!acceptClipboard.checked());
    y += setPrimaryCB.height();
    sendClipboard.move(xPad, y);
    sendClipboard.checked(getBoolParam(dpy, SEND_CUT_TEXT));
    y += sendClipboard.height();
    sendPrimaryCB.move(xPad + 10, y);
    sendPrimaryCB.checked(getBoolParam(dpy, SEND_PRIMARY));
    sendPrimaryCB.disabled(!sendClipboard.checked());
    y += sendPrimaryCB.height();
    setEventHandler(this);
    toplevel("VNC config", this, 0, 0, 0, iconic);
    XVncExtSelectInput(dpy, win(), VncExtQueryConnectMask);
  }

  // handleEvent()

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    if (ev->type == vncExtEventBase + VncExtQueryConnectNotify) {
       vlog.debug("query connection event");
       if (queryConnectDialog)
         delete queryConnectDialog;
       queryConnectDialog = 0;
       char* qcAddress;
       char* qcUser;
       int qcTimeout;
       if (XVncExtGetQueryConnect(dpy, &qcAddress, &qcUser,
                                  &qcTimeout, &queryConnectId)) {
         if (qcTimeout)
           queryConnectDialog = new QueryConnectDialog(dpy, qcAddress,
                                                       qcUser, qcTimeout,
                                                       this);
         if (queryConnectDialog)
           queryConnectDialog->map();
         XFree(qcAddress);
         XFree(qcUser);
       }
    }
  }

  // TXDeleteWindowCallback method
  virtual void deleteWindow(TXWindow* w) {
    exit(1);
  }

  // TXCheckboxCallback method
  virtual void checkboxSelect(TXCheckbox* checkbox) {
    if (checkbox == &acceptClipboard) {
      XVncExtSetParam(dpy, (acceptClipboard.checked()
                            ? ACCEPT_CUT_TEXT "=1" : ACCEPT_CUT_TEXT "=0"));
      setPrimaryCB.disabled(!acceptClipboard.checked());
    } else if (checkbox == &sendClipboard) {
      XVncExtSetParam(dpy, (sendClipboard.checked()
                            ? SEND_CUT_TEXT "=1" : SEND_CUT_TEXT "=0"));
      sendPrimaryCB.disabled(!sendClipboard.checked());
    } else if (checkbox == &setPrimaryCB) {
      XVncExtSetParam(dpy, (setPrimaryCB.checked()
                            ? SET_PRIMARY "=1" : SET_PRIMARY "=0"));
    } else if (checkbox == &sendPrimaryCB) {
      XVncExtSetParam(dpy, (sendPrimaryCB.checked()
                            ? SEND_PRIMARY "=1" : SEND_PRIMARY "=0"));
    }
  }

  // QueryResultCallback interface
  virtual void queryApproved() {
    XVncExtApproveConnect(dpy, queryConnectId, 1);
  }
  virtual void queryRejected() {
    XVncExtApproveConnect(dpy, queryConnectId, 0);
  }

private:
  TXCheckbox acceptClipboard, setPrimaryCB;
  TXCheckbox sendClipboard, sendPrimaryCB;

  QueryConnectDialog* queryConnectDialog;
  void* queryConnectId;
};

static void usage()
{
  fprintf(stderr,"usage: %s [parameters]\n",
          programName);
  fprintf(stderr,"       %s [parameters] -connect <host>[:<port>]\n",
          programName);
  fprintf(stderr,"       %s [parameters] -disconnect\n", programName);
  fprintf(stderr,"       %s [parameters] [-set] <Xvnc-param>=<value> ...\n",
          programName);
  fprintf(stderr,"       %s [parameters] -list\n", programName);
  fprintf(stderr,"       %s [parameters] -get <param>\n", programName);
  fprintf(stderr,"       %s [parameters] -desc <param>\n",programName);
  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  Configuration::listParams(79, 14);
  exit(1);
}

void removeArgs(int* argc, char** argv, int first, int n)
{
  if (first + n > *argc) return;
  for (int i = first + n; i < *argc; i++)
    argv[i-n] = argv[i];
  *argc -= n;
}

int main(int argc, char** argv)
{
  programName = argv[0];
  rfb::initStdIOLoggers();
  rfb::LogWriter::setLogParams("*:stderr:30");

  // Process vncconfig's own parameters first, then we process the
  // other arguments when we have the X display.
  int i;
  for (i = 1; i < argc; i++) {
    if (Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-' && i+1 < argc &&
        Configuration::setParam(&argv[i][1], argv[i+1])) {
      i++;
      continue;
    }
    break;
  }

  CharArray displaynameStr(displayname.getData());
  if (!(dpy = XOpenDisplay(displaynameStr.buf))) {
    fprintf(stderr,"%s: unable to open display \"%s\"\n",
            programName, XDisplayName(displaynameStr.buf));
    exit(1);
  }

  if (!XVncExtQueryExtension(dpy, &vncExtEventBase, &vncExtErrorBase)) {
    fprintf(stderr,"No VNC extension on display %s\n",
            XDisplayName(displaynameStr.buf));
    exit(1);
  }

  if (i < argc) {
    for (; i < argc; i++) {
      if (strcmp(argv[i], "-connect") == 0) {
        i++;
        if (i >= argc) usage();
        if (!XVncExtConnect(dpy, argv[i])) {
          fprintf(stderr,"connecting to %s failed\n",argv[i]);
        }
      } else if (strcmp(argv[i], "-disconnect") == 0) {
        if (!XVncExtConnect(dpy, "")) {
          fprintf(stderr,"disconnecting all clients failed\n");
        }
      } else if (strcmp(argv[i], "-get") == 0) {
        i++;
        if (i >= argc) usage();
        char* data;
        int len;
        if (XVncExtGetParam(dpy, argv[i], &data, &len)) {
          printf("%.*s\n",len,data);
        } else {
          fprintf(stderr,"getting param %s failed\n",argv[i]);
        }
        XFree(data);
      } else if (strcmp(argv[i], "-desc") == 0) {
        i++;
        if (i >= argc) usage();
        char* desc = XVncExtGetParamDesc(dpy, argv[i]);
        if (desc) {
          printf("%s\n",desc);
        } else {
          fprintf(stderr,"getting description for param %s failed\n",argv[i]);
        }
        XFree(desc);
      } else if (strcmp(argv[i], "-list") == 0) {
        int nParams;
        char** list = XVncExtListParams(dpy, &nParams);
        for (int i = 0; i < nParams; i++) {
          printf("%s\n",list[i]);
        }
        XVncExtFreeParamList(list);
      } else if (strcmp(argv[i], "-set") == 0) {
        i++;
        if (i >= argc) usage();
        if (!XVncExtSetParam(dpy, argv[i])) {
          fprintf(stderr,"setting param %s failed\n",argv[i]);
        }
      } else if (XVncExtSetParam(dpy, argv[i])) {
        fprintf(stderr,"set parameter %s\n",argv[i]);
      } else {
        usage();
      }
    }

    return 0;
  }

  try {
    TXWindow::init(dpy,"Vncconfig");

    VncConfigWindow w(dpy);
    if (!noWindow) w.map();

    while (true) {
      struct timeval tv;
      struct timeval* tvp = 0;

      // Process any incoming X events
      TXWindow::handleXEvents(dpy);
      
      // Process expired timers and get the time until the next one
      int timeoutMs = Timer::checkTimeouts();
      if (timeoutMs) {
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        tvp = &tv;
      }
      
      // If there are X requests pending then poll, don't wait!
      if (XPending(dpy)) {
        tv.tv_usec = tv.tv_sec = 0;
        tvp = &tv;
      }
      
      // Wait for X events, VNC traffic, or the next timer expiry
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(ConnectionNumber(dpy), &rfds);
      int n = select(FD_SETSIZE, &rfds, 0, 0, tvp);
      if (n < 0) throw rdr::SystemException("select",errno);
    }

    XCloseDisplay(dpy);

  } catch (rdr::Exception &e) {
    vlog.error("%s", e.str());
  }

  return 0;
}
