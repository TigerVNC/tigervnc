/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

using namespace rfb;

LogWriter vlog("vncconfig");

StringParameter displayname("display", "The X display", "");
BoolParameter noWindow("nowin", "Don't display a window", 0);
BoolParameter iconic("iconic", "Start with window iconified", 0);

#define ACCEPT_CUT_TEXT "AcceptCutText"
#define SEND_CUT_TEXT "SendCutText"

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
                        public TXCheckboxCallback {
public:
  VncConfigWindow(Display* dpy)
    : TXWindow(dpy, 300, 100), clientCutText(0), clientCutTextLen(0),
      acceptClipboard(dpy, "Accept clipboard from viewers", this, false, this),
      sendClipboard(dpy, "Send clipboard to viewers", this, false, this),
      sendPrimary(dpy, "Send primary selection to viewers", this, false, this)
  {
    int y = yPad;
    acceptClipboard.move(xPad, y);
    acceptClipboard.checked(getBoolParam(dpy, ACCEPT_CUT_TEXT));
    y += acceptClipboard.height();
    sendClipboard.move(xPad, y);
    sendClipboard.checked(getBoolParam(dpy, SEND_CUT_TEXT));
    y += sendClipboard.height();
    sendPrimary.move(xPad, y);
    sendPrimary.checked(true);
    sendPrimary.disabled(!sendClipboard.checked());
    y += sendPrimary.height();
    setEventHandler(this);
    toplevel("VNC config", this, 0, 0, 0, iconic);
    XVncExtSelectInput(dpy, win(),
                       VncExtClientCutTextMask|VncExtSelectionChangeMask);
  }

  // handleEvent(). If we get a ClientCutTextNotify event from Xvnc, set the
  // primary and clipboard selections to the clientCutText. If we get a
  // SelectionChangeNotify event from Xvnc, set the serverCutText to the value
  // of the new selection.

  virtual void handleEvent(TXWindow* w, XEvent* ev) {
    if (acceptClipboard.checked()) {
      if (ev->type == vncExtEventBase + VncExtClientCutTextNotify) {
        XVncExtClientCutTextEvent* cutEv = (XVncExtClientCutTextEvent*)ev;
        if (clientCutText)
          XFree(clientCutText);
        clientCutText = 0;
        if (XVncExtGetClientCutText(dpy, &clientCutText, &clientCutTextLen)) {
          vlog.debug("Got client cut text");
          XStoreBytes(dpy, clientCutText, clientCutTextLen);
          ownSelection(XA_PRIMARY, cutEv->time);
          ownSelection(xaCLIPBOARD, cutEv->time);
        }
      }
    }
    if (sendClipboard.checked()) {
      if (ev->type == vncExtEventBase + VncExtSelectionChangeNotify) {
        XVncExtSelectionChangeEvent* selEv = (XVncExtSelectionChangeEvent*)ev;
        if (selEv->selection == xaCLIPBOARD ||
            (selEv->selection == XA_PRIMARY && sendPrimary.checked())) {
          if (!selectionOwner(selEv->selection))
            XConvertSelection(dpy, selEv->selection, XA_STRING,
                              selEv->selection, win(), CurrentTime);
        }
      }
    }
  }
  

  // selectionRequest() is called when we are the selection owner and another X
  // client has requested the selection.  We simply put the server's cut text
  // into the requested property.  TXWindow will handle the rest.
  bool selectionRequest(Window requestor, Atom selection, Atom property)
  {
    if (clientCutText)
      XChangeProperty(dpy, requestor, property, XA_STRING, 8,
                      PropModeReplace, (unsigned char*)clientCutText,
                      clientCutTextLen);
    return clientCutText;
  }

  // selectionNotify() is called when we have requested the selection from the
  // selection owner.
  void selectionNotify(XSelectionEvent* ev, Atom type, int format,
                       int nitems, void* data)
  {
    if (ev->requestor != win() || ev->target != XA_STRING)
      return;

    if (data && format == 8) {
      vlog.debug("setting selection as server cut text");
      XVncExtSetServerCutText(dpy, (char*)data, nitems);
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
    } else if (checkbox == &sendClipboard) {
      XVncExtSetParam(dpy, (sendClipboard.checked()
                            ? SEND_CUT_TEXT "=1" : SEND_CUT_TEXT "=0"));
      sendPrimary.disabled(!sendClipboard.checked());
    }
  }

private:
  char* clientCutText;
  int clientCutTextLen;
  TXCheckbox acceptClipboard, sendClipboard, sendPrimary;
};

static void usage()
{
  fprintf(stderr,"usage: %s [-display <display>] [-nowin] [-iconic]\n",
          programName);
  fprintf(stderr,"       %s [-display <display>] -connect <host>[:<port>]\n",
          programName);
  fprintf(stderr,"       %s [-display <display>] -disconnect\n", programName);
  fprintf(stderr,"       %s [-display <display>] [-set] <param>=<value> ...\n",
          programName);
  fprintf(stderr,"       %s [-display <display>] -list\n", programName);
  fprintf(stderr,"       %s [-display <display>] -get <param>\n", programName);
  fprintf(stderr,"       %s [-display <display>] -desc <param>\n",programName);
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
      TXWindow::handleXEvents(dpy);
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(ConnectionNumber(dpy), &rfds);
      int n = select(FD_SETSIZE, &rfds, 0, 0, 0);
      if (n < 0) throw rdr::SystemException("select",errno);
    }

    XCloseDisplay(dpy);

  } catch (rdr::Exception &e) {
    vlog.error(e.str());
  }

  return 0;
}
