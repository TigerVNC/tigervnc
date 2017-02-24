/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2017 Pierre Ossman for Cendio AB
 * Copyright 2014 Brian P. Hinz
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <rfb/ServerCore.h>

#include "XserverDesktop.h"
#include "vncBlockHandler.h"
#include "vncExtInit.h"
#include "vncHooks.h"
#include "vncSelection.h"
#include "XorgGlue.h"
#include "Input.h"

using namespace rfb;
using namespace network;

static LogWriter vlog("XserverDesktop");

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


XserverDesktop::XserverDesktop(int screenIndex_,
                               std::list<network::TcpListener*> listeners_,
                               std::list<network::TcpListener*> httpListeners_,
                               const char* name, const rfb::PixelFormat &pf,
                               int width, int height,
                               void* fbptr, int stride)
  : screenIndex(screenIndex_),
    server(0), httpServer(0),
    listeners(listeners_), httpListeners(httpListeners_),
    directFbptr(true),
    queryConnectId(0)
{
  format = pf;

  server = new VNCServerST(name, this);
  setFramebuffer(width, height, fbptr, stride);
  server->setQueryConnectionHandler(this);

  if (!httpListeners.empty ())
    httpServer = new FileHTTPServer(this);

  for (std::list<TcpListener*>::iterator i = listeners.begin();
       i != listeners.end();
       i++) {
    vncSetNotifyFd((*i)->getFd(), screenIndex, true, false);
  }

  for (std::list<TcpListener*>::iterator i = httpListeners.begin();
       i != httpListeners.end();
       i++) {
    vncSetNotifyFd((*i)->getFd(), screenIndex, true, false);
  }
}

XserverDesktop::~XserverDesktop()
{
  while (!listeners.empty()) {
    vncRemoveNotifyFd(listeners.back()->getFd());
    delete listeners.back();
    listeners.pop_back();
  }
  while (!httpListeners.empty()) {
    vncRemoveNotifyFd(listeners.back()->getFd());
    delete httpListeners.back();
    httpListeners.pop_back();
  }
  if (!directFbptr)
    delete [] data;
  delete httpServer;
  delete server;
}

void XserverDesktop::blockUpdates()
{
  server->blockUpdates();
}

void XserverDesktop::unblockUpdates()
{
  server->unblockUpdates();
}

void XserverDesktop::setFramebuffer(int w, int h, void* fbptr, int stride_)
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
    stride_ = w;
    directFbptr = false;
  }

  data = (rdr::U8*)fbptr;
  stride = stride_;

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
  OutputIdMap newIdMap;

  for (int i = 0;i < vncRandRGetOutputCount(screenIndex);i++) {
      intptr_t outputId;
      int x, y, width, height;

      /* Disabled? */
      if (!vncRandRIsOutputEnabled(screenIndex, i))
          continue;

      outputId = vncRandRGetOutputId(screenIndex, i);

      /* Known output? */
      if (outputIdMap.count(outputId) == 1)
        newIdMap[outputId] = outputIdMap[outputId];
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

        newIdMap[outputId] = id;
      }

      vncRandRGetOutputDimensions(screenIndex, i, &x, &y, &width, &height);

      layout.add_screen(Screen(newIdMap[outputId], x, y, width, height, 0));
  }

  /* Only keep the entries that are currently active */
  outputIdMap = newIdMap;

  /*
   * Make sure we have something to display. Hopefully it's just temporary
   * that we have no active outputs...
   */
  if (layout.num_screens() == 0)
    layout.add_screen(Screen(0, 0, 0, vncGetScreenWidth(screenIndex),
                             vncGetScreenHeight(screenIndex), 0));

  return layout;
}

char* XserverDesktop::substitute(const char* varName)
{
  if (strcmp(varName, "$$") == 0) {
    return rfb::strDup("$");
  }
  if (strcmp(varName, "$PORT") == 0) {
    char* str = new char[10];
    sprintf(str, "%d", listeners.empty () ? 0 : (*listeners.begin ())->getMyPort());
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
    sprintf(str, "%d", height());
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
    strncat(str, vncGetDisplay(), 10);
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
                                char** reason)
{
  int count;

  if (queryConnectId) {
    *reason = strDup("Another connection is currently being queried.");
    return rfb::VNCServerST::REJECT;
  }

  queryConnectAddress.replaceBuf(sock->getPeerAddress());
  if (!userName)
    userName = "(anonymous)";
  queryConnectUsername.replaceBuf(strDup(userName));
  queryConnectId = (uint32_t)(intptr_t)sock;
  queryConnectSocket = sock;

  count = vncNotifyQueryConnect();
  if (count == 0) {
    *reason = strDup("Unable to query the local user to accept the connection.");
    return rfb::VNCServerST::REJECT;
  }

  return rfb::VNCServerST::PENDING;
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

void XserverDesktop::setCursor(int width, int height, int hotX, int hotY,
                               const unsigned char *rgbaData)
{
  rdr::U8* cursorData;

  rdr::U8 *out;
  const unsigned char *in;

  cursorData = new rdr::U8[width * height * 4];

  // Un-premultiply alpha
  in = rgbaData;
  out = cursorData;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      rdr::U8 alpha;

      alpha = in[3];
      if (alpha == 0)
        alpha = 1; // Avoid division by zero

      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = (unsigned)*in++ * 255/alpha;
      *out++ = *in++;
    }
  }

  try {
    server->setCursor(width, height, Point(hotX, hotY), cursorData);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.str());
  }

  delete [] cursorData;
}

void XserverDesktop::add_changed(const rfb::Region &region)
{
  try {
    server->add_changed(region);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_changed: %s",e.str());
  }
}

void XserverDesktop::add_copied(const rfb::Region &dest, const rfb::Point &delta)
{
  try {
    server->add_copied(dest, delta);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::add_copied: %s",e.str());
  }
}

void XserverDesktop::handleSocketEvent(int fd, bool read, bool write)
{
  try {
    if (read) {
      if (handleListenerEvent(fd, &listeners, server))
        return;
      if (handleListenerEvent(fd, &httpListeners, httpServer))
        return;
    }

    if (handleSocketEvent(fd, server, read, write))
      return;
    if (handleSocketEvent(fd, httpServer, read, write))
      return;

    vlog.error("Cannot find file descriptor for socket event");
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::handleSocketEvent: %s",e.str());
  }
}

bool XserverDesktop::handleListenerEvent(int fd,
                                         std::list<TcpListener*>* sockets,
                                         SocketServer* sockserv)
{
  std::list<TcpListener*>::iterator i;

  for (i = sockets->begin(); i != sockets->end(); i++) {
    if ((*i)->getFd() == fd)
      break;
  }

  if (i == sockets->end())
    return false;

  Socket* sock = (*i)->accept();
  sock->outStream().setBlocking(false);
  vlog.debug("new client, sock %d", sock->getFd());
  sockserv->addSocket(sock);
  vncSetNotifyFd(sock->getFd(), screenIndex, true, false);

  return true;
}

bool XserverDesktop::handleSocketEvent(int fd,
                                       SocketServer* sockserv,
                                       bool read, bool write)
{
  std::list<Socket*> sockets;
  std::list<Socket*>::iterator i;

  sockserv->getSockets(&sockets);
  for (i = sockets.begin(); i != sockets.end(); i++) {
    if ((*i)->getFd() == fd)
      break;
  }

  if (i == sockets.end())
    return false;

  if (read)
    sockserv->processSocketReadEvent(*i);

  if (write)
    sockserv->processSocketWriteEvent(*i);

  return true;
}

void XserverDesktop::blockHandler(int* timeout)
{
  // We don't have a good callback for when we can init input devices[1],
  // so we abuse the fact that this routine will be called first thing
  // once the dix is done initialising.
  // [1] Technically Xvnc has InitInput(), but libvnc.so has nothing.
  vncInitInputDevice();

  try {
    std::list<Socket*> sockets;
    std::list<Socket*>::iterator i;
    server->getSockets(&sockets);
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      if ((*i)->isShutdown()) {
        vlog.debug("client gone, sock %d",fd);
        vncRemoveNotifyFd(fd);
        server->removeSocket(*i);
        vncClientGone(fd);
        delete (*i);
      } else {
        /* Update existing NotifyFD to listen for write (or not) */
        vncSetNotifyFd(fd, screenIndex, true, (*i)->outStream().bufferUsage() > 0);
      }
    }
    if (httpServer) {
      httpServer->getSockets(&sockets);
      for (i = sockets.begin(); i != sockets.end(); i++) {
        int fd = (*i)->getFd();
        if ((*i)->isShutdown()) {
          vlog.debug("http client gone, sock %d",fd);
          vncRemoveNotifyFd(fd);
          httpServer->removeSocket(*i);
          delete (*i);
        } else {
          /* Update existing NotifyFD to listen for write (or not) */
          vncSetNotifyFd(fd, screenIndex, true, (*i)->outStream().bufferUsage() > 0);
        }
      }
    }

    // We are responsible for propagating mouse movement between clients
    int cursorX, cursorY;
    vncGetPointerPos(&cursorX, &cursorY);
    cursorX -= vncGetScreenX(screenIndex);
    cursorY -= vncGetScreenY(screenIndex);
    if (oldCursorPos.x != cursorX || oldCursorPos.y != cursorY) {
      oldCursorPos.x = cursorX;
      oldCursorPos.y = cursorY;
      server->setCursorPos(oldCursorPos);
    }

    // Trigger timers and check when the next will expire
    int nextTimeout = server->checkTimeouts();
    if (nextTimeout > 0 && (*timeout == -1 || nextTimeout < *timeout))
      *timeout = nextTimeout;
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::blockHandler: %s",e.str());
  }
}

void XserverDesktop::addClient(Socket* sock, bool reverse)
{
  vlog.debug("new client, sock %d reverse %d",sock->getFd(),reverse);
  sock->outStream().setBlocking(false);
  server->addSocket(sock, reverse);
  vncSetNotifyFd(sock->getFd(), screenIndex, true, false);
}

void XserverDesktop::disconnectClients()
{
  vlog.debug("disconnecting all clients");
  return server->closeClients("Disconnection from server end");
}


void XserverDesktop::getQueryConnect(uint32_t* opaqueId,
                                     const char** address,
                                     const char** username,
                                     int *timeout)
{
  *opaqueId = queryConnectId;

  if (queryConnectId == 0) {
    *address = "";
    *username = "";
    *timeout = 0;
  } else {
    *address = queryConnectAddress.buf;
    *username = queryConnectUsername.buf;
    *timeout = rfb::Server::queryConnectTimeout;
  }
}

void XserverDesktop::approveConnection(uint32_t opaqueId, bool accept,
                                       const char* rejectMsg)
{
  if (queryConnectId == opaqueId) {
    server->approveConnection(queryConnectSocket, accept, rejectMsg);
    queryConnectId = 0;
  }
}

///////////////////////////////////////////////////////////////////////////
//
// SDesktop callbacks


void XserverDesktop::pointerEvent(const Point& pos, int buttonMask)
{
  vncPointerMove(pos.x + vncGetScreenX(screenIndex),
                 pos.y + vncGetScreenY(screenIndex));
  vncPointerButtonAction(buttonMask);
}

void XserverDesktop::clientCutText(const char* str, int len)
{
  vncClientCutText(str, len);
}

unsigned int XserverDesktop::setScreenLayout(int fb_width, int fb_height,
                                             const rfb::ScreenSet& layout)
{
  int ret;
  int availableOutputs;

  // RandR support?
  if (vncRandRGetOutputCount(screenIndex) == 0)
    return rfb::resultProhibited;

  char buffer[2048];
  vlog.debug("Got request for framebuffer resize to %dx%d",
             fb_width, fb_height);
  layout.print(buffer, sizeof(buffer));
  vlog.debug("%s", buffer);

  /*
   * First check that we don't have any active clone modes. That's just
   * too messy to deal with.
   */
  if (vncRandRHasOutputClones(screenIndex)) {
    vlog.error("Clone mode active. Refusing to touch screen layout.");
    return rfb::resultInvalid;
  }

  /* Next count how many useful outputs we have... */
  availableOutputs = vncRandRGetAvailableOutputs(screenIndex);

  /* Try to create more outputs if needed... (only works on Xvnc) */
  if (layout.num_screens() > availableOutputs) {
    vlog.debug("Insufficient screens. Need to create %d more.",
               layout.num_screens() - availableOutputs);
    ret = vncRandRCreateOutputs(screenIndex,
                                layout.num_screens() - availableOutputs);
    if (ret < 0) {
      vlog.error("Unable to create more screens, as needed by the new client layout.");
      return rfb::resultInvalid;
    }
  }

  /* First we might need to resize the screen */
  if ((fb_width != vncGetScreenWidth(screenIndex)) ||
      (fb_height != vncGetScreenHeight(screenIndex))) {
    ret = vncRandRResizeScreen(screenIndex, fb_width, fb_height);
    if (!ret) {
      vlog.error("Failed to resize screen to %dx%d", fb_width, fb_height);
      return rfb::resultInvalid;
    }
  }

  /* Next, reconfigure all known outputs, and turn off the other ones */
  for (int i = 0;i < vncRandRGetOutputCount(screenIndex);i++) {
    intptr_t output;

    ScreenSet::const_iterator iter;

    output = vncRandRGetOutputId(screenIndex, i);

    /* Known? */
    if (outputIdMap.count(output) == 0)
      continue;

    /* Find the corresponding screen... */
    for (iter = layout.begin();iter != layout.end();++iter) {
      if (iter->id == outputIdMap[output])
        break;
    }

    /* Missing? */
    if (iter == layout.end()) {
      /* Disable and move on... */
      ret = vncRandRDisableOutput(screenIndex, i);
      if (!ret) {
        vlog.error("Failed to disable unused output '%s'",
                   vncRandRGetOutputName(screenIndex, i));
        return rfb::resultInvalid;
      }
      outputIdMap.erase(output);
      continue;
    }

    /* Reconfigure new mode and position */
    ret = vncRandRReconfigureOutput(screenIndex, i,
                                    iter->dimensions.tl.x,
                                    iter->dimensions.tl.y,
                                    iter->dimensions.width(),
                                    iter->dimensions.height());
    if (!ret) {
      vlog.error("Failed to reconfigure output '%s' to %dx%d+%d+%d",
                 vncRandRGetOutputName(screenIndex, i),
                 iter->dimensions.width(), iter->dimensions.height(),
                 iter->dimensions.tl.x, iter->dimensions.tl.y);
      return rfb::resultInvalid;
    }
  }

  /* Finally, allocate new outputs for new screens */
  ScreenSet::const_iterator iter;
  for (iter = layout.begin();iter != layout.end();++iter) {
    OutputIdMap::const_iterator oi;
    intptr_t output;
    int i;

    /* Does this screen have an output already? */
    for (oi = outputIdMap.begin();oi != outputIdMap.end();++oi) {
      if (oi->second == iter->id)
        break;
    }

    if (oi != outputIdMap.end())
      continue;

    /* Find an unused output */
    for (i = 0;i < vncRandRGetOutputCount(screenIndex);i++) {
      output = vncRandRGetOutputId(screenIndex, i);

      /* In use? */
      if (outputIdMap.count(output) == 1)
        continue;

      /* Can it be used? */
      if (!vncRandRIsOutputUsable(screenIndex, i))
        continue;

      break;
    }

    /* Shouldn't happen */
    if (i == vncRandRGetOutputCount(screenIndex))
        return rfb::resultInvalid;

    /*
     * Make sure we already have an entry for this, or
     * computeScreenLayout() will think it is a brand new output and
     * assign it a random id.
     */
    outputIdMap[output] = iter->id;

    /* Reconfigure new mode and position */
    ret = vncRandRReconfigureOutput(screenIndex, i,
                                    iter->dimensions.tl.x,
                                    iter->dimensions.tl.y,
                                    iter->dimensions.width(),
                                    iter->dimensions.height());
    if (!ret) {
      vlog.error("Failed to reconfigure output '%s' to %dx%d+%d+%d",
                 vncRandRGetOutputName(screenIndex, i),
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
  vncRandRUpdateSetTime(screenIndex);

  return rfb::resultSuccess;
}

void XserverDesktop::grabRegion(const rfb::Region& region)
{
  if (directFbptr)
    return;

  std::vector<rfb::Rect> rects;
  std::vector<rfb::Rect>::iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    rdr::U8 *buffer;
    int stride;

    buffer = getBufferRW(*i, &stride);
    vncGetScreenImage(screenIndex, i->tl.x, i->tl.y, i->width(), i->height(),
                      (char*)buffer, stride * format.bpp/8);
    commitBufferRW(*i);
  }
}

void XserverDesktop::keyEvent(rdr::U32 keysym, bool down)
{
  vncKeyboardEvent(keysym, down);
}
