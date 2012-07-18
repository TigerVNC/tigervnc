/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
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
// XserverDesktop.cxx
//

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <network/TcpSocket.h>
#include <rfb/Exception.h>
#include <rfb/VNCServerST.h>
#include <rfb/HTTPServer.h>
#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include "XserverDesktop.h"
#include "vncExtInit.h"
#include "xorg-version.h"
#include "Input.h"

extern "C" {
#define public c_public
#define class c_class

extern char *display;

#include "colormapst.h"
#ifdef RANDR
#include "randrstr.h"
#endif
#include "cursorstr.h"
#undef public
#undef class
}

using namespace rfb;
using namespace network;

static LogWriter vlog("XserverDesktop");

IntParameter queryConnectTimeout("QueryConnectTimeout",
                                 "Number of seconds to show the Accept Connection dialog before "
                                 "rejecting the connection",
                                 10);

static rdr::U8 reverseBits[] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0,
  0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4,
  0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
  0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca,
  0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6,
  0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1,
  0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
  0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
  0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3,
  0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7,
  0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf,
  0x3f, 0xbf, 0x7f, 0xff
};


class FileHTTPServer : public rfb::HTTPServer {
public:
  FileHTTPServer(XserverDesktop* d) : desktop(d) {}
  virtual ~FileHTTPServer() {}

  virtual rdr::InStream* getFile(const char* name, const char** contentType,
                                 int* contentLength, time_t* lastModified)
  {
    if (name[0] != '/' || strstr(name, "..") != 0) {
      vlog.info("http request was for invalid file name");
      return 0;
    }

    if (strcmp(name, "/") == 0) name = "/index.vnc";

    CharArray httpDirStr(httpDir.getData());
    CharArray fname(strlen(httpDirStr.buf)+strlen(name)+1);
    sprintf(fname.buf, "%s%s", httpDirStr.buf, name);
    int fd = open(fname.buf, O_RDONLY);
    if (fd < 0) return 0;
    rdr::InStream* is = new rdr::FdInStream(fd, -1, 0, true);
    *contentType = guessContentType(name, *contentType);
    if (strlen(name) > 4 && strcasecmp(&name[strlen(name)-4], ".vnc") == 0) {
      is = new rdr::SubstitutingInStream(is, desktop, 20);
      *contentType = "text/html";
    } else {
      struct stat st;
      if (fstat(fd, &st) == 0) {
        *contentLength = st.st_size;
        *lastModified = st.st_mtime;
      }
    }
    return is;
  }

  XserverDesktop* desktop;
};


XserverDesktop::XserverDesktop(ScreenPtr pScreen_,
                               network::TcpListener* listener_,
                               network::TcpListener* httpListener_,
                               const char* name, const rfb::PixelFormat &pf,
                               void* fbptr, int stride)
  : pScreen(pScreen_),
    server(0), httpServer(0),
    listener(listener_), httpListener(httpListener_),
    cmap(0), deferredUpdateTimerSet(false),
    grabbing(false), ignoreHooks_(false), directFbptr(true),
    queryConnectId(0)
{
  format = pf;
  colourmap = this;

  serverReset(pScreen);

  server = new VNCServerST(name, this);
  setFramebuffer(pScreen->width, pScreen->height, fbptr, stride);
  server->setQueryConnectionHandler(this);

  if (httpListener)
    httpServer = new FileHTTPServer(this);

  inputDevice = new InputDevice(server);
}

XserverDesktop::~XserverDesktop()
{
  if (!directFbptr)
    delete [] data;
  delete inputDevice;
  delete httpServer;
  delete server;
}

void XserverDesktop::serverReset(ScreenPtr pScreen_)
{
  pScreen = pScreen_;
  int i;
  pointer retval;

#if XORG >= 17
#define dixLookupResource dixLookupResourceByType
#endif
  i = dixLookupResource(&retval, pScreen->defColormap, RT_COLORMAP, NullClient,
			DixReadAccess);

  /* Handle suspicious conditions */
  assert(i == Success);

  cmap = (ColormapPtr) retval;
}

void XserverDesktop::blockUpdates()
{
  server->blockUpdates();
}

void XserverDesktop::unblockUpdates()
{
  server->unblockUpdates();
}

void XserverDesktop::setFramebuffer(int w, int h, void* fbptr, int stride)
{
  ScreenSet layout;

  width_ = w;
  height_ = h;

  if (!directFbptr) {
    delete [] data;
    directFbptr = true;
  }

  if (!fbptr) {
    fbptr = new rdr::U8[w * h * (format.bpp/8)];
    stride = w;
    directFbptr = false;
  }

  data = (rdr::U8*)fbptr;
  stride_ = stride;

  layout = computeScreenLayout();

  server->setPixelBuffer(this, layout);
}

void XserverDesktop::refreshScreenLayout()
{
  server->setScreenLayout(computeScreenLayout());
}

ScreenSet XserverDesktop::computeScreenLayout()
{
  ScreenSet layout;

#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(pScreen);
  OutputIdMap newIdMap;

  for (int i = 0;i < rp->numOutputs;i++) {
      RROutputPtr output;
      RRCrtcPtr crtc;

      output = rp->outputs[i];
      crtc = output->crtc;

      /* Disabled? */
      if ((crtc == NULL) || (crtc->mode == NULL))
          continue;

      /* Known output? */
      if (outputIdMap.count(output) == 1)
        newIdMap[output] = outputIdMap[output];
      else {
        rdr::U32 id;
        OutputIdMap::const_iterator iter;

        while (true) {
          id = rand();
          for (iter = outputIdMap.begin();iter != outputIdMap.end();++iter) {
            if (iter->second == id)
              break;
          }
          if (iter == outputIdMap.end())
            break;
        }

        newIdMap[output] = id;
      }

      layout.add_screen(Screen(newIdMap[output], crtc->x, crtc->y,
                               crtc->mode->mode.width,
                               crtc->mode->mode.height,
                               0));
  }

  /* Only keep the entries that are currently active */
  outputIdMap = newIdMap;
#endif

  /*
   * Make sure we have something to display. Hopefully it's just temporary
   * that we have no active outputs...
   */
  if (layout.num_screens() == 0)
    layout.add_screen(Screen(0, 0, 0, pScreen->width, pScreen->height, 0));

  return layout;
}

#ifdef RANDR

extern RRModePtr vncRandRModeGet(int width, int height);

RRModePtr XserverDesktop::findRandRMode(RROutputPtr output, int width, int height)
{
  RRModePtr mode;

  for (int i = 0;i < output->numModes;i++) {
    if ((output->modes[i]->mode.width == width) &&
        (output->modes[i]->mode.height == height))
      return output->modes[i];
  }

  mode = vncRandRModeGet(width, height);
  if (mode != NULL)
    return mode;

  return NULL;
}

#endif

char* XserverDesktop::substitute(const char* varName)
{
  if (strcmp(varName, "$$") == 0) {
    return rfb::strDup("$");
  }
  if (strcmp(varName, "$PORT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", listener ? listener->getMyPort() : 0);
    return str;
  }
  if (strcmp(varName, "$WIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", width());
    return str;
  }
  if (strcmp(varName, "$HEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", height());
    return str;
  }
  if (strcmp(varName, "$APPLETWIDTH") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", width());
    return str;
  }
  if (strcmp(varName, "$APPLETHEIGHT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", height() + 32);
    return str;
  }
  if (strcmp(varName, "$DESKTOP") == 0) {
    return rfb::strDup(server->getName());
  }
  if (strcmp(varName, "$DISPLAY") == 0) {
    struct utsname uts;
    uname(&uts);
    char* str = new char[256];
    strncpy(str, uts.nodename, 240);
    str[239] = '\0'; /* Ensure string is zero-terminated */
    strcat(str, ":");
    strncat(str, display, 10);
    return str;
  }
  if (strcmp(varName, "$USER") == 0) {
    struct passwd* user = getpwuid(getuid());
    return rfb::strDup(user ? user->pw_name : "?");
  }
  return 0;
}

rfb::VNCServerST::queryResult
XserverDesktop::queryConnection(network::Socket* sock,
                                const char* userName,
                                char** reason) {
  if (queryConnectId) {
    *reason = strDup("Another connection is currently being queried.");
    return rfb::VNCServerST::REJECT;
  }
  queryConnectAddress.replaceBuf(sock->getPeerAddress());
  if (!userName)
    userName = "(anonymous)";
  queryConnectUsername.replaceBuf(strDup(userName));
  queryConnectId = sock;
  vncQueryConnect(this, sock);
  return rfb::VNCServerST::PENDING;
}


void XserverDesktop::setColormap(ColormapPtr cmap_)
{
  if (cmap != cmap_) {
    cmap = cmap_;
    setColourMapEntries(0, 0);
  }
}

void XserverDesktop::setColourMapEntries(ColormapPtr pColormap, int ndef,
                                         xColorItem* pdef)
{
  if (cmap != pColormap || ndef <= 0) return;

  unsigned int first = pdef[0].pixel;
  unsigned int n = 1;

  for (int i = 1; i < ndef; i++) {
    if (first + n == pdef[i].pixel) {
      n++;
    } else {
      setColourMapEntries(first, n);
      first = pdef[i].pixel;
      n = 1;
    }
  }
  setColourMapEntries(first, n);
}

void XserverDesktop::setColourMapEntries(int firstColour, int nColours)
{
  try {
    server->setColourMapEntries(firstColour, nColours);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setColourMapEntries: %s",e.str());
  }
}

void XserverDesktop::bell()
{
  server->bell();
}

void XserverDesktop::serverCutText(const char* str, int len)
{
  try {
    server->serverCutText(str, len);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::serverCutText: %s",e.str());
  }
}

void XserverDesktop::setDesktopName(const char* name)
{
  try {
    server->setName(name);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setDesktopName: %s",e.str());
  }
}

void XserverDesktop::setCursor(CursorPtr cursor)
{
  try {
    int w = cursor->bits->width;
    int h = cursor->bits->height;

    rdr::U8* cursorData = new rdr::U8[w * h * (getPF().bpp / 8)];

    int rfbMaskBytesPerRow = (w + 7) / 8;

    rdr::U8* cursorMask = new rdr::U8[rfbMaskBytesPerRow * h];

#ifdef ARGB_CURSOR
    if (cursor->bits->argb) {
      rdr::U8 *out;
      CARD32 *in;

      rdr::U8 rgb[3];

      memset(cursorMask, 0, rfbMaskBytesPerRow * h);

      in = cursor->bits->argb;
      out = cursorData;
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          rgb[0] = (*in >> 16) & 0xff;
          rgb[1] = (*in >>  8) & 0xff;
          rgb[2] = (*in >>  0) & 0xff;

          getPF().bufferFromRGB(out, rgb, 1, this);

          if (((*in >> 24) & 0xff) > 127)
            cursorMask[y * rfbMaskBytesPerRow + x/8] |= 0x80>>(x%8);

          in++;
          out += getPF().bpp/8;
        }
      }
    } else {
#endif
      xColorItem fg, bg;
      fg.red   = cursor->foreRed;
      fg.green = cursor->foreGreen;
      fg.blue  = cursor->foreBlue;
      FakeAllocColor(cmap, &fg);
      bg.red   = cursor->backRed;
      bg.green = cursor->backGreen;
      bg.blue  = cursor->backBlue;
      FakeAllocColor(cmap, &bg);
      FakeFreeColor(cmap, fg.pixel);
      FakeFreeColor(cmap, bg.pixel);

      int xMaskBytesPerRow = BitmapBytePad(w);

      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          int byte = y * xMaskBytesPerRow + x / 8;
#if (BITMAP_BIT_ORDER == MSBFirst)
          int bit = 7 - x % 8;
#else
          int bit = x % 8;
#endif
          switch (getPF().bpp) {
          case 8:
            ((rdr::U8*)cursorData)[y * w + x]
              = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
            break;
          case 16:
            ((rdr::U16*)cursorData)[y * w + x]
              = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
            break;
          case 32:
            ((rdr::U32*)cursorData)[y * w + x]
              = (cursor->bits->source[byte] & (1 << bit)) ? fg.pixel : bg.pixel;
            break;
          }
        }
      }

      for (int j = 0; j < h; j++) {
        for (int i = 0; i < rfbMaskBytesPerRow; i++)
#if (BITMAP_BIT_ORDER == MSBFirst)
          cursorMask[j * rfbMaskBytesPerRow + i]
            = cursor->bits->mask[j * xMaskBytesPerRow + i];
#else
          cursorMask[j * rfbMaskBytesPerRow + i]
            = reverseBits[cursor->bits->mask[j * xMaskBytesPerRow + i]];
#endif
      }
#ifdef ARGB_CURSOR
    }
#endif

    server->setCursor(cursor->bits->width, cursor->bits->height,
                      Point(cursor->bits->xhot, cursor->bits->yhot),
                      cursorData, cursorMask);
    delete [] cursorData;
    delete [] cursorMask;
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.str());
  }
}

void XserverDesktop::add_changed(RegionPtr reg)
{
  if (ignoreHooks_) return;
  if (grabbing) return;
  try {
    rfb::Region rfbReg;
    rfbReg.setExtentsAndOrderedRects((ShortRect*)REGION_EXTENTS(pScreen, reg),
                                     REGION_NUM_RECTS(reg),
                                     (ShortRect*)REGION_RECTS(reg));
    server->add_changed(rfbReg);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_changed: %s",e.str());
  }
}

void XserverDesktop::add_copied(RegionPtr dst, int dx, int dy)
{
  if (ignoreHooks_) return;
  if (grabbing) return;
  try {
    rfb::Region rfbReg;
    rfbReg.setExtentsAndOrderedRects((ShortRect*)REGION_EXTENTS(pScreen, dst),
                                     REGION_NUM_RECTS(dst),
                                     (ShortRect*)REGION_RECTS(dst));
    server->add_copied(rfbReg, rfb::Point(dx, dy));
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_copied: %s",e.str());
  }
}

static struct timeval XserverDesktopTimeout;

void XserverDesktop::blockHandler(fd_set* fds, OSTimePtr timeout)
{
  try {
    int nextTimeout;

    // Add all sockets we want read events for, after purging
    // any closed sockets.
    if (listener)
      FD_SET(listener->getFd(), fds);
    if (httpListener)
      FD_SET(httpListener->getFd(), fds);

    std::list<Socket*> sockets;
    server->getSockets(&sockets);
    std::list<Socket*>::iterator i;
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      if ((*i)->isShutdown()) {
        vlog.debug("client gone, sock %d",fd);
        server->removeSocket(*i);
        vncClientGone(fd);
        delete (*i);
      } else {
        FD_SET(fd, fds);
      }
    }
    if (httpServer) {
      httpServer->getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if ((*i)->isShutdown()) {
          vlog.debug("http client gone, sock %d",fd);
          httpServer->removeSocket(*i);
          delete (*i);
        } else {
          FD_SET(fd, fds);
        }
      }
    }

    // Then check when the next timer will expire.
    // (this unfortunately also triggers any already expired timers)
    nextTimeout = server->checkTimeouts();
    if (nextTimeout > 0) {
      // No timeout specified? Or later timeout than we need?
      if ((*timeout == NULL) ||
          ((*timeout)->tv_sec > (nextTimeout/1000)) ||
          (((*timeout)->tv_sec == (nextTimeout/1000)) &&
           ((*timeout)->tv_usec > ((nextTimeout%1000)*1000)))) {
        XserverDesktopTimeout.tv_sec = nextTimeout/1000;
        XserverDesktopTimeout.tv_usec = (nextTimeout%1000)*1000;
        *timeout = &XserverDesktopTimeout;
      }
    }

  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::blockHandler: %s",e.str());
  }
}

void XserverDesktop::wakeupHandler(fd_set* fds, int nfds)
{
  try {
    // First check for file descriptors with something to do
    if (nfds >= 1) {

      if (listener) {
        if (FD_ISSET(listener->getFd(), fds)) {
          FD_CLR(listener->getFd(), fds);
          Socket* sock = listener->accept();
          sock->outStream().setBlocking(false);
          server->addSocket(sock);
          vlog.debug("new client, sock %d",sock->getFd());
        }
      }

      if (httpListener) {
        if (FD_ISSET(httpListener->getFd(), fds)) {
          FD_CLR(httpListener->getFd(), fds);
          Socket* sock = httpListener->accept();
          sock->outStream().setBlocking(false);
          httpServer->addSocket(sock);
          vlog.debug("new http client, sock %d",sock->getFd());
        }
      }

      std::list<Socket*> sockets;
      server->getSockets(&sockets);
      std::list<Socket*>::iterator i;
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if (FD_ISSET(fd, fds)) {
          FD_CLR(fd, fds);
          server->processSocketEvent(*i);
        }
      }

      if (httpServer) {
        httpServer->getSockets(&sockets);
        for (i = sockets.begin(); i != sockets.end(); i++) {
          int fd = (*i)->getFd();
          if (FD_ISSET(fd, fds)) {
            FD_CLR(fd, fds);
            httpServer->processSocketEvent(*i);
          }
        }
      }

      inputDevice->PointerSync();
    }

    // Then let the timers do some processing. Rescheduling is done in
    // blockHandler().
    server->checkTimeouts();
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::wakeupHandler: %s",e.str());
  }
}

void XserverDesktop::writeBlockHandler(fd_set* fds)
{
  try {
    std::list<Socket*> sockets;
    std::list<Socket*>::iterator i;

    server->getSockets(&sockets);
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      if ((*i)->isShutdown()) {
        vlog.debug("client gone, sock %d",fd);
        server->removeSocket(*i);
        vncClientGone(fd);
        delete (*i);
      } else {
        if ((*i)->outStream().bufferUsage() > 0)
          FD_SET(fd, fds);
      }
    }

    if (httpServer) {
      httpServer->getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if ((*i)->isShutdown()) {
          vlog.debug("http client gone, sock %d",fd);
          httpServer->removeSocket(*i);
          delete (*i);
        } else {
          if ((*i)->outStream().bufferUsage() > 0)
            FD_SET(fd, fds);
        }
      }
    }
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::writeBlockHandler: %s",e.str());
  }
}

void XserverDesktop::writeWakeupHandler(fd_set* fds, int nfds)
{
  if (nfds < 1)
    return;

  try {
    std::list<Socket*> sockets;
    std::list<Socket*>::iterator i;

    server->getSockets(&sockets);
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      if (FD_ISSET(fd, fds)) {
        FD_CLR(fd, fds);
        (*i)->outStream().flush();
      }
    }

    if (httpServer) {
      httpServer->getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if (FD_ISSET(fd, fds)) {
          FD_CLR(fd, fds);
          (*i)->outStream().flush();
        }
      }
    }
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::writeWakeupHandler: %s",e.str());
  }
}

void XserverDesktop::addClient(Socket* sock, bool reverse)
{
  vlog.debug("new client, sock %d reverse %d",sock->getFd(),reverse);
  server->addSocket(sock, reverse);
}

void XserverDesktop::disconnectClients()
{
  vlog.debug("disconnecting all clients");
  return server->closeClients("Disconnection from server end");
}


int XserverDesktop::getQueryTimeout(void* opaqueId,
                                    const char** address,
                                    const char** username)
{
  if (opaqueId && queryConnectId == opaqueId) {
    vlog.info("address=%s, username=%s, timeout=%d",
              queryConnectAddress.buf, queryConnectUsername.buf,
              (int)queryConnectTimeout);
    if (address) *address = queryConnectAddress.buf;
    if (username) *username = queryConnectUsername.buf;
    return queryConnectTimeout;
  }
  return 0;
}

void XserverDesktop::approveConnection(void* opaqueId, bool accept,
                                       const char* rejectMsg)
{
  if (queryConnectId == opaqueId) {
    server->approveConnection((network::Socket*)opaqueId, accept, rejectMsg);
    queryConnectId = 0;
  }
}

///////////////////////////////////////////////////////////////////////////
//
// SDesktop callbacks


void XserverDesktop::pointerEvent(const Point& pos, int buttonMask)
{
  inputDevice->PointerMove(pos);
  inputDevice->PointerButtonAction(buttonMask);
}

void XserverDesktop::clientCutText(const char* str, int len)
{
  vncClientCutText(str, len);
}

extern RROutputPtr vncRandROutputCreate(ScreenPtr pScreen);

unsigned int XserverDesktop::setScreenLayout(int fb_width, int fb_height,
                                             const rfb::ScreenSet& layout)
{
#ifndef RANDR
  return rfb::resultProhibited;
#else
  int availableOutputs;
  Bool ret;

  rrScrPrivPtr rp = rrGetScrPriv(pScreen);

  /*
   * First check that we don't have any active clone modes. That's just
   * too messy to deal with.
   */
  for (int i = 0;i < rp->numCrtcs;i++) {
    if (rp->crtcs[i]->numOutputs > 1) {
      vlog.error("Clone mode active. Refusing to touch screen layout.");
      return rfb::resultInvalid;
    }
  }

  /*
   * Next count how many useful outputs we have...
   *
   * This gets slightly complicated because we might need to hook a CRTC
   * up to the output, but also check that we don't try to use the same
   * CRTC for multiple outputs.
   */
  std::set<RRCrtcPtr> usedCrtcs;
  availableOutputs = 0;
  for (int i = 0;i < rp->numOutputs;i++) {
    RROutputPtr output;

    output = rp->outputs[i];

    if (output->crtc != NULL)
      availableOutputs++;
    else {
      for (int j = 0;j < output->numCrtcs;j++) {
        if (output->crtcs[j]->numOutputs != 0)
          continue;
        if (usedCrtcs.count(output->crtcs[j]) != 0)
          continue;

        availableOutputs++;
        usedCrtcs.insert(output->crtcs[j]);

        break;
      }
    }
  }

  /* Try to create more outputs if needed... (only works on Xvnc) */
  if (layout.num_screens() > availableOutputs) {
    for (int i = 0;i < (layout.num_screens() - availableOutputs);i++) {
      RROutputPtr output;
      output = vncRandROutputCreate(pScreen);
      if (output == NULL) {
        vlog.error("Unable to create more screens, as needed by the new client layout.");
        return rfb::resultInvalid;
      }
    }
  }

  /* First we might need to resize the screen */
  if ((fb_width != pScreen->width) || (fb_height == pScreen->height)) {
    /* Try to retain DPI when we resize */
    ret = RRScreenSizeSet(pScreen, fb_width, fb_height,
                          pScreen->mmWidth * fb_width / pScreen->width,
                          pScreen->mmHeight * fb_height / pScreen->height);
    if (!ret) {
      vlog.error("Failed to resize screen to %dx%d", fb_width, fb_height);
      return rfb::resultInvalid;
    }
  }

  /* Next, reconfigure all known outputs, and turn off the other ones */
  for (int i = 0;i < rp->numOutputs;i++) {
    RROutputPtr output;
    RRCrtcPtr crtc;
    RRModePtr mode;

    ScreenSet::const_iterator iter;

    output = rp->outputs[i];
    crtc = output->crtc;

    /* Known? */
    if (outputIdMap.count(output) == 0)
      continue;

    /* A known output should have a CRTC, but double check... */
    if (crtc == NULL) {
      vlog.error("Existing output '%s' has unexpectedly been disabled",
                 output->name);
      continue;
    }

    /* Find the corresponding screen... */
    for (iter = layout.begin();iter != layout.end();++iter) {
      if (iter->id == outputIdMap[output])
        break;
    }

    /* Missing? */
    if (iter == layout.end()) {
      /* Disable and move on... */
      ret = RRCrtcSet(crtc, NULL, crtc->x, crtc->y, crtc->rotation,
                      crtc->numOutputs, crtc->outputs);
      if (!ret) {
        vlog.error("Failed to disable unused CRTC for output '%s'",
                   output->name);
        return rfb::resultInvalid;
      }
      outputIdMap.erase(output);
      continue;
    }

    /* Need to switch mode? */
    if ((crtc->mode->mode.width == iter->dimensions.width()) &&
        (crtc->mode->mode.height == iter->dimensions.height()))
      mode = crtc->mode;
    else {
      mode = findRandRMode(output, iter->dimensions.width(),
                           iter->dimensions.height());
      if (mode == NULL) {
        vlog.error("Failed to find a suitable mode for %dx%d for output '%s'",
                   iter->dimensions.width(), iter->dimensions.height(),
                   output->name);
        return rfb::resultInvalid;
      }
    }

    /* Reconfigure new mode and position */
    ret = RRCrtcSet(crtc, mode, iter->dimensions.tl.x, iter->dimensions.tl.y,
                    crtc->rotation, crtc->numOutputs, crtc->outputs);
    if (!ret) {
      vlog.error("Failed to reconfigure output '%s' to %dx%d+%d+%d",
                 output->name,
                 iter->dimensions.width(), iter->dimensions.height(),
                 iter->dimensions.tl.x, iter->dimensions.tl.y);
      return rfb::resultInvalid;
    }
  }

  /* Finally, allocate new outputs for new screens */
  ScreenSet::const_iterator iter;
  for (iter = layout.begin();iter != layout.end();++iter) {
    OutputIdMap::const_iterator oi;

    RROutputPtr output;
    RRCrtcPtr crtc;
    RRModePtr mode;

    int i;

    /* Does this screen have an output already? */
    for (oi = outputIdMap.begin();oi != outputIdMap.end();++oi) {
      if (oi->second == iter->id)
        break;
    }

    if (oi != outputIdMap.end())
      continue;

    /* Find an unused output */
    for (i = 0;i < rp->numOutputs;i++) {
      output = rp->outputs[i];
      crtc = output->crtc;

      /* In use? */
      if (outputIdMap.count(output) == 1)
        continue;

      /* Need a CRTC? */
      if (crtc == NULL) {
        for (int j = 0;j < output->numCrtcs;j++) {
          if (output->crtcs[j]->numOutputs != 0)
            continue;

          crtc = output->crtcs[j];
          break;
        }

        /* Couldn't find one... */
        if (crtc == NULL)
          continue;

        ret = RRCrtcSet(crtc, NULL, 0, 0, RR_Rotate_0,
                        1, &output);
        if (!ret) {
          vlog.error("Failed to associate a CRTC with output '%s'",
                     output->name);
          return rfb::resultInvalid;
        }
      }

      break;
    }

    /* Shouldn't happen */
    if (i == rp->numOutputs)
        return rfb::resultInvalid;

    mode = findRandRMode(output, iter->dimensions.width(),
                         iter->dimensions.height());
    if (mode == NULL) {
      vlog.error("Failed to find a suitable mode for %dx%d for output '%s'",
                 iter->dimensions.width(), iter->dimensions.height(),
                 output->name);
      return rfb::resultInvalid;
    }

    /*
     * Make sure we already have an entry for this, or
     * computeScreenLayout() will think it is a brand new output and
     * assign it a random id.
     */
    outputIdMap[output] = iter->id;

    /* Reconfigure new mode and position */
    ret = RRCrtcSet(crtc, mode,  iter->dimensions.tl.x, iter->dimensions.tl.y,
                    crtc->rotation, crtc->numOutputs, crtc->outputs);
    if (!ret) {
      vlog.error("Failed to reconfigure output '%s' to %dx%d+%d+%d",
                 output->name,
                 iter->dimensions.width(), iter->dimensions.height(),
                 iter->dimensions.tl.x, iter->dimensions.tl.y);
      return rfb::resultInvalid;
    }
  }

  /*
   * Update timestamp for when screen layout was last changed.
   * This is normally done in the X11 request handlers, which is
   * why we have to deal with it manually here.
   */
  rp->lastSetTime = currentTime;

  return rfb::resultSuccess;
#endif
}

void XserverDesktop::grabRegion(const rfb::Region& region)
{
  if (directFbptr) return;
  if (!pScreen->GetImage) {
    vlog.error("VNC error: pScreen->GetImage == 0");
    return;
  }

  grabbing = true;

  int bytesPerPixel = format.bpp/8;
  int bytesPerRow = pScreen->width * bytesPerPixel;

  std::vector<rfb::Rect> rects;
  std::vector<rfb::Rect>::iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    for (int y = i->tl.y; y < i->br.y; y++) {
      DrawablePtr pDrawable;
#if XORG < 19
      pDrawable = (DrawablePtr) WindowTable[pScreen->myNum];
#else
      pDrawable = (DrawablePtr) pScreen->root;
#endif

      (*pScreen->GetImage) (pDrawable, i->tl.x, y, i->width(), 1,
                            ZPixmap, (unsigned long)~0L,
                            ((char*)data
                             + y * bytesPerRow + i->tl.x * bytesPerPixel));
    }
  }
  grabbing = false;
}

int XserverDesktop::getStride() const
{
  return stride_;
}

void XserverDesktop::lookup(int index, int* r, int* g, int* b)
{
  if ((cmap->c_class | DynamicClass) == DirectColor) {
    VisualPtr v = cmap->pVisual;
    *r = cmap->red  [(index & v->redMask  ) >> v->offsetRed  ].co.local.red;
    *g = cmap->green[(index & v->greenMask) >> v->offsetGreen].co.local.green;
    *b = cmap->blue [(index & v->blueMask ) >> v->offsetBlue ].co.local.blue;
  } else {
    EntryPtr pent;
    pent = (EntryPtr)&cmap->red[index];
    if (pent->fShared) {
      *r = pent->co.shco.red->color;
      *g = pent->co.shco.green->color;
      *b = pent->co.shco.blue->color;
    } else {
      *r = pent->co.local.red;
      *g = pent->co.local.green;
      *b = pent->co.local.blue;
    }
  }
}

void XserverDesktop::keyEvent(rdr::U32 keysym, bool down)
{
	if (down)
		inputDevice->KeyboardPress(keysym);
	else
		inputDevice->KeyboardRelease(keysym);
}
