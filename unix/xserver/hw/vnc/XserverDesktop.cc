/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2024 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>

#include <core/Configuration.h>
#include <core/LogWriter.h>

#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>

#include <network/Socket.h>

#include <rfb/VNCServerST.h>
#include <rfb/ServerCore.h>

#include "XserverDesktop.h"
#include "vncBlockHandler.h"
#include "vncExtInit.h"
#include "vncHooks.h"
#include "vncSelection.h"
#include "XorgGlue.h"
#include "vncInput.h"

extern "C" {
void vncSetGlueContext(int screenIndex);
void vncPresentMscEvent(uint64_t id, uint64_t msc);
}

static core::LogWriter vlog("XserverDesktop");

core::BoolParameter
  rawKeyboard("RawKeyboard",
              "Send keyboard events straight through and avoid mapping "
              "them to the current keyboard layout", false);
core::IntParameter
  queryConnectTimeout("QueryConnectTimeout",
                      "Number of seconds to show the 'Accept "
                      "connection' dialog before rejecting the "
                      "connection", 10, 0, INT_MAX);


XserverDesktop::XserverDesktop(int screenIndex_,
                               std::list<network::SocketListener*> listeners_,
                               const char* name, const rfb::PixelFormat &pf,
                               int width, int height,
                               void* fbptr, int stride_)
  : screenIndex(screenIndex_),
    server(0), listeners(listeners_),
    shadowFramebuffer(nullptr),
    queryConnectId(0), queryConnectTimer(this)
{
  format = pf;

  server = new rfb::VNCServerST(name, this);
  setFramebuffer(width, height, fbptr, stride_);

  for (network::SocketListener* listener : listeners)
    vncSetNotifyFd(listener->getFd(), screenIndex, true, false);
}

XserverDesktop::~XserverDesktop()
{
  while (!listeners.empty()) {
    vncRemoveNotifyFd(listeners.back()->getFd());
    delete listeners.back();
    listeners.pop_back();
  }
  if (shadowFramebuffer)
    delete [] shadowFramebuffer;
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
  rfb::ScreenSet layout;

  if (shadowFramebuffer) {
    delete [] shadowFramebuffer;
    shadowFramebuffer = nullptr;
  }

  if (!fbptr) {
    shadowFramebuffer = new uint8_t[w * h * (format.bpp/8)];
    fbptr = shadowFramebuffer;
    stride_ = w;
  }

  setBuffer(w, h, (uint8_t*)fbptr, stride_);

  vncSetGlueContext(screenIndex);
  layout = ::computeScreenLayout(&outputIdMap);

  server->setPixelBuffer(this, layout);
}

void XserverDesktop::refreshScreenLayout()
{
  vncSetGlueContext(screenIndex);
  server->setScreenLayout(::computeScreenLayout(&outputIdMap));
}

uint64_t XserverDesktop::getMsc()
{
  return server->getMsc();
}

void XserverDesktop::queueMsc(uint64_t id, uint64_t msc)
{
  pendingMsc[id] = msc;
  server->queueMsc(msc);
}

void XserverDesktop::abortMsc(uint64_t id)
{
  pendingMsc.erase(id);
}

void XserverDesktop::init(rfb::VNCServer* vs)
{
  // We already own the server object, and we always keep it in a
  // ready state
}

void XserverDesktop::queryConnection(network::Socket* sock,
                                     const char* userName)
{
  int count;

  if (queryConnectTimer.isStarted()) {
    server->approveConnection(sock, false, "Another connection is currently being queried.");
    return;
  }

  count = vncNotifyQueryConnect();
  if (count == 0) {
    server->approveConnection(sock, false, "Unable to query the local user to accept the connection.");
    return;
  }

  queryConnectAddress = sock->getPeerAddress();
  if (!userName)
    userName = "(anonymous)";
  queryConnectUsername = userName;
  queryConnectId = (uint32_t)(intptr_t)sock;
  queryConnectSocket = sock;

  queryConnectTimer.start(queryConnectTimeout * 1000);
}

void XserverDesktop::requestClipboard()
{
  try {
    server->requestClipboard();
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::requestClipboard: %s",e.what());
  }
}

void XserverDesktop::announceClipboard(bool available)
{
  try {
    server->announceClipboard(available);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::announceClipboard: %s",e.what());
  }
}

void XserverDesktop::sendClipboardData(const char* data_)
{
  try {
    server->sendClipboardData(data_);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::sendClipboardData: %s",e.what());
  }
}

void XserverDesktop::bell()
{
  server->bell();
}

void XserverDesktop::setLEDState(unsigned int state)
{
  server->setLEDState(state);
}

void XserverDesktop::setDesktopName(const char* name)
{
  try {
    server->setName(name);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::setDesktopName: %s",e.what());
  }
}

void XserverDesktop::setCursor(int width, int height, int hotX, int hotY,
                               const unsigned char *rgbaData)
{
  uint8_t* cursorData;

  uint8_t *out;
  const unsigned char *in;

  cursorData = new uint8_t[width * height * 4];

  // Un-premultiply alpha
  in = rgbaData;
  out = cursorData;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t alpha;

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
    server->setCursor(width, height, {hotX, hotY}, cursorData);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.what());
  }

  delete [] cursorData;
}

void XserverDesktop::setCursorPos(int x, int y, bool warped)
{
  try {
    server->setCursorPos({x, y}, warped);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::setCursorPos: %s",e.what());
  }
}

void XserverDesktop::add_changed(const core::Region& region)
{
  try {
    server->add_changed(region);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::add_changed: %s",e.what());
  }
}

void XserverDesktop::add_copied(const core::Region& dest,
                                const core::Point& delta)
{
  try {
    server->add_copied(dest, delta);
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::add_copied: %s",e.what());
  }
}

void XserverDesktop::handleSocketEvent(int fd, bool read, bool write)
{
  try {
    if (read) {
      if (handleListenerEvent(fd, &listeners, server))
        return;
    }

    if (handleSocketEvent(fd, server, read, write))
      return;

    vlog.error("Cannot find file descriptor for socket event");
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::handleSocketEvent: %s",e.what());
  }
}

bool XserverDesktop::handleListenerEvent(int fd,
                                         std::list<network::SocketListener*>* sockets,
                                         rfb::VNCServer* sockserv)
{
  std::list<network::SocketListener*>::iterator i;

  for (i = sockets->begin(); i != sockets->end(); i++) {
    if ((*i)->getFd() == fd)
      break;
  }

  if (i == sockets->end())
    return false;

  network::Socket* sock = (*i)->accept();
  vlog.debug("New client, sock %d", sock->getFd());
  sockserv->addSocket(sock);
  vncSetNotifyFd(sock->getFd(), screenIndex, true, false);

  return true;
}

bool XserverDesktop::handleSocketEvent(int fd,
                                       rfb::VNCServer* sockserv,
                                       bool read, bool write)
{
  std::list<network::Socket*> sockets;
  std::list<network::Socket*>::iterator i;

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

  // Do a graceful close by waiting for the peer to close their end
  if ((*i)->isShutdown()) {
    bool done;

    done = false;
    while (true) {
      try {
        (*i)->inStream().skip((*i)->inStream().avail());
        if (!(*i)->inStream().hasData(1))
          break;
      } catch (std::exception&) {
        done = true;
        break;
      }
    }

    if (done) {
      vlog.debug("Client gone, sock %d",fd);
      vncRemoveNotifyFd(fd);
      sockserv->removeSocket(*i);
      vncClientGone(fd);
      delete (*i);
    }
  }

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
    std::list<network::Socket*> sockets;
    std::list<network::Socket*>::iterator i;
    server->getSockets(&sockets);
    for (i = sockets.begin(); i != sockets.end(); i++) {
      int fd = (*i)->getFd();
      /* Update existing NotifyFD to listen for write (or not) */
      vncSetNotifyFd(fd, screenIndex, true, (*i)->outStream().hasBufferedData());
    }

    // We are responsible for propagating mouse movement between clients
    int cursorX, cursorY;
    vncGetPointerPos(&cursorX, &cursorY);
    cursorX -= vncGetScreenX(screenIndex);
    cursorY -= vncGetScreenY(screenIndex);
    if (oldCursorPos.x != cursorX || oldCursorPos.y != cursorY) {
      oldCursorPos.x = cursorX;
      oldCursorPos.y = cursorY;
      server->setCursorPos(oldCursorPos, false);
    }

    // Trigger timers and check when the next will expire
    int nextTimeout = core::Timer::checkTimeouts();
    if (nextTimeout >= 0 && (*timeout == -1 || nextTimeout < *timeout))
      *timeout = nextTimeout;
  } catch (std::exception& e) {
    vlog.error("XserverDesktop::blockHandler: %s", e.what());
  }
}

void XserverDesktop::addClient(network::Socket* sock,
                               bool reverse, bool viewOnly)
{
  vlog.debug("New client, sock %d reverse %d",sock->getFd(),reverse);
  server->addSocket(sock, reverse,
                    viewOnly ? rfb::AccessView : rfb::AccessDefault);
  vncSetNotifyFd(sock->getFd(), screenIndex, true, false);
}

void XserverDesktop::disconnectClients()
{
  vlog.debug("Disconnecting all clients");
  return server->closeClients("Disconnection from server end");
}


void XserverDesktop::getQueryConnect(uint32_t* opaqueId,
                                     const char** address,
                                     const char** username,
                                     int *timeout)
{
  *opaqueId = queryConnectId;

  if (!queryConnectTimer.isStarted()) {
    *address = "";
    *username = "";
    *timeout = 0;
  } else {
    *address = queryConnectAddress.c_str();
    *username = queryConnectUsername.c_str();
    *timeout = queryConnectTimeout;
  }
}

void XserverDesktop::approveConnection(uint32_t opaqueId, bool accept,
                                       const char* rejectMsg)
{
  if (queryConnectId == opaqueId) {
    server->approveConnection(queryConnectSocket, accept, rejectMsg);
    queryConnectId = 0;
    queryConnectTimer.stop();
  }
}

///////////////////////////////////////////////////////////////////////////
//
// SDesktop callbacks


void XserverDesktop::terminate()
{
  kill(getpid(), SIGTERM);
}

void XserverDesktop::pointerEvent(const core::Point& pos,
                                  uint16_t buttonMask)
{
  vncPointerMove(pos.x + vncGetScreenX(screenIndex),
                 pos.y + vncGetScreenY(screenIndex));
  vncPointerButtonAction(buttonMask);
}

unsigned int XserverDesktop::setScreenLayout(int fb_width, int fb_height,
                                             const rfb::ScreenSet& layout)
{
  unsigned int result;

  vncSetGlueContext(screenIndex);
  result = ::setScreenLayout(fb_width, fb_height, layout, &outputIdMap);

  // Explicitly update the server state with the result as there
  // can be corner cases where we don't get feedback from the X core
  refreshScreenLayout();

  return result;
}

void XserverDesktop::frameTick(uint64_t msc)
{
  std::map<uint64_t, uint64_t>::iterator iter, next;

  for (iter = pendingMsc.begin(); iter != pendingMsc.end();) {
    next = iter; next++;

    if (iter->second <= msc) {
      pendingMsc.erase(iter->first);
      vncPresentMscEvent(iter->first, msc);
    }

    iter = next;
  }
}

void XserverDesktop::handleClipboardRequest()
{
  vncHandleClipboardRequest();
}

void XserverDesktop::handleClipboardAnnounce(bool available)
{
  vncHandleClipboardAnnounce(available);
}

void XserverDesktop::handleClipboardData(const char* data_)
{
  vncHandleClipboardData(data_);
}

void XserverDesktop::grabRegion(const core::Region& region)
{
  if (shadowFramebuffer == nullptr)
    return;

  std::vector<core::Rect> rects;
  std::vector<core::Rect>::iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    uint8_t *buffer;
    int bufStride;

    buffer = getBufferRW(*i, &bufStride);
    vncGetScreenImage(screenIndex, i->tl.x, i->tl.y, i->width(), i->height(),
                      (char*)buffer, bufStride * format.bpp/8);
    commitBufferRW(*i);
  }
}

void XserverDesktop::keyEvent(uint32_t keysym, uint32_t keycode, bool down)
{
  if (!rawKeyboard)
    keycode = 0;

  vncKeyboardEvent(keysym, keycode, down);
}

void XserverDesktop::handleTimeout(core::Timer* t)
{
  if (t == &queryConnectTimer) {
    server->approveConnection(queryConnectSocket, false,
                              "The attempt to prompt the user to "
                              "accept the connection failed");
  }
}
