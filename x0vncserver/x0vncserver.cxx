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
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rfb/Configuration.h>
#include <rfb/SSecurityFactoryStandard.h>

#include <network/TcpSocket.h>

#include "Image.h"
#include <signal.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>


#include <rfb/Encoder.h>

using namespace rfb;
using namespace rdr;
using namespace network;

LogWriter vlog("main");

StringParameter displayname("display", "The X display", "");
IntParameter rfbport("rfbport", "TCP port to listen for RFB protocol",5900);
VncAuthPasswdFileParameter vncAuthPasswdFile;

static void CleanupSignalHandler(int sig)
{
  // CleanupSignalHandler allows C++ object cleanup to happen because it calls
  // exit() rather than the default which is to abort.
  fprintf(stderr,"CleanupSignalHandler called\n");
  exit(1);
}


class XDesktop : public SDesktop, public rfb::ColourMap
{
public:
  XDesktop(Display* dpy_)
    : dpy(dpy_), pb(0), server(0), oldButtonMask(0), haveXtest(false)
  {
    int xtestEventBase;
    int xtestErrorBase;
    int major, minor;

    if (XTestQueryExtension(dpy, &xtestEventBase,
                            &xtestErrorBase, &major, &minor)) {
      XTestGrabControl(dpy, True);
      vlog.info("XTest extension present - version %d.%d",major,minor);
      haveXtest = true;
    } else {
      vlog.info("XTest extension not present");
      vlog.info("unable to inject events or display while server is grabbed");
    }

    int dpyWidth = DisplayWidth(dpy, DefaultScreen(dpy));
    int dpyHeight = DisplayHeight(dpy, DefaultScreen(dpy));
    Visual* vis = DefaultVisual(dpy, DefaultScreen(dpy));

    image = new Image(dpy, dpyWidth, dpyHeight);
    image->get(DefaultRootWindow(dpy));

    pf.bpp = image->xim->bits_per_pixel;
    pf.depth = image->xim->depth;
    pf.bigEndian = (image->xim->byte_order == MSBFirst);
    pf.trueColour = (vis->c_class == TrueColor);
    pf.redShift   = ffs(vis->red_mask) - 1;
    pf.greenShift = ffs(vis->green_mask) - 1;
    pf.blueShift  = ffs(vis->blue_mask) - 1;
    pf.redMax     = vis->red_mask   >> pf.redShift;
    pf.greenMax   = vis->green_mask >> pf.greenShift;
    pf.blueMax    = vis->blue_mask  >> pf.blueShift;

    pb = new FullFramePixelBuffer(pf, dpyWidth, dpyHeight,
                                  (rdr::U8*)image->xim->data, this);
  }
  virtual ~XDesktop() {
    delete pb;
  }

  void setVNCServer(VNCServer* s) {
    server = s;
    server->setPixelBuffer(pb);
  }

  // -=- SDesktop interface

  virtual void pointerEvent(const Point& pos, rdr::U8 buttonMask) {
    if (!haveXtest) return;
    XTestFakeMotionEvent(dpy, DefaultScreen(dpy), pos.x, pos.y, CurrentTime);
    if (buttonMask != oldButtonMask) {
      for (int i = 0; i < 5; i++) {
	if ((buttonMask ^ oldButtonMask) & (1<<i)) {
          if (buttonMask & (1<<i)) {
            XTestFakeButtonEvent(dpy, i+1, True, CurrentTime);
          } else {
            XTestFakeButtonEvent(dpy, i+1, False, CurrentTime);
          }
        }
      }
    }
    oldButtonMask = buttonMask;
  }

  virtual void keyEvent(rdr::U32 key, bool down) {
    if (!haveXtest) return;
    int keycode = XKeysymToKeycode(dpy, key);
    if (keycode)
      XTestFakeKeyEvent(dpy, keycode, down, CurrentTime);
  }

  virtual void clientCutText(const char* str, int len) {
  }

  virtual Point getFbSize() {
    return Point(pb->width(), pb->height());
  }

  // rfb::ColourMap callbacks
  virtual void lookup(int index, int* r, int* g, int* b) {
    XColor xc;
    xc.pixel = index;
    if (index < DisplayCells(dpy,DefaultScreen(dpy))) {
      XQueryColor(dpy, DefaultColormap(dpy,DefaultScreen(dpy)), &xc);
    } else {
      xc.red = xc.green = xc.blue = 0;
    }
    *r = xc.red;
    *g = xc.green;
    *b = xc.blue;
  }

  virtual void poll() {
    if (server && server->clientsReadyForUpdate()) {
      image->get(DefaultRootWindow(dpy));
      server->add_changed(pb->getRect());
      server->tryUpdate();
    }
  }

protected:
  Display* dpy;
  PixelFormat pf;
  PixelBuffer* pb;
  VNCServer* server;
  Image* image;
  int oldButtonMask;
  bool haveXtest;
};

char* programName;

static void usage()
{
  fprintf(stderr, "\nusage: %s [<parameters>]\n", programName);
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

int main(int argc, char** argv)
{
  initStdIOLoggers();
  LogWriter::setLogParams("*:stderr:30");

  programName = argv[0];
  Display* dpy;

  for (int i = 1; i < argc; i++) {
    if (Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (Configuration::setParam(&argv[i][1], argv[i+1])) {
          i++;
          continue;
        }
      }
      usage();
    }

    usage();
  }

  CharArray dpyStr(displayname.getData());
  if (!(dpy = XOpenDisplay(dpyStr.buf[0] ? dpyStr.buf : 0))) {
    fprintf(stderr,"%s: unable to open display \"%s\"\r\n",
            programName, XDisplayName(displayname.getData()));
    exit(1);
  }

  signal(SIGHUP, CleanupSignalHandler);
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  try {
    XDesktop desktop(dpy);
    VNCServerST server("x0vncserver", &desktop);
    desktop.setVNCServer(&server);

    TcpSocket::initTcpSockets();
    TcpListener listener((int)rfbport);
    vlog.info("Listening on port %d", (int)rfbport);

    while (true) {
      fd_set rfds;
      struct timeval tv;

      tv.tv_sec = 0;
      tv.tv_usec = 50*1000;

      FD_ZERO(&rfds);
      FD_SET(listener.getFd(), &rfds);

      std::list<Socket*> sockets;
      server.getSockets(&sockets);
      std::list<Socket*>::iterator i;
      for (i = sockets.begin(); i != sockets.end(); i++) {
        FD_SET((*i)->getFd(), &rfds);
      }

      int n = select(FD_SETSIZE, &rfds, 0, 0, &tv);
      if (n < 0) throw rdr::SystemException("select",errno);

      if (FD_ISSET(listener.getFd(), &rfds)) {
        Socket* sock = listener.accept();
        server.addClient(sock);
      }

      server.getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        if (FD_ISSET((*i)->getFd(), &rfds)) {
          server.processSocketEvent(*i);
        }
      }

      server.checkTimeouts();
      desktop.poll();
    }

  } catch (rdr::Exception &e) {
    vlog.error(e.str());
  };

  return 0;
}
