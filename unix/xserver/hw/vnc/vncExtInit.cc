/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2015 Pierre Ossman for Cendio AB
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

#include <stdio.h>
#include <errno.h>

#include <set>
#include <string>

#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/ServerCore.h>
#include <rdr/HexOutStream.h>
#include <rfb/LogWriter.h>
#include <rfb/Hostname.h>
#include <rfb/Region.h>
#include <network/TcpSocket.h>

#include "XserverDesktop.h"
#include "vncExtInit.h"
#include "vncHooks.h"
#include "vncBlockHandler.h"
#include "vncSelection.h"
#include "XorgGlue.h"
#include "xorg-version.h"

using namespace rfb;

static rfb::LogWriter vlog("vncext");

// We can't safely get this from Xorg
#define MAXSCREENS 16

static unsigned long vncExtGeneration = 0;
static bool initialised = false;
static XserverDesktop* desktop[MAXSCREENS] = { 0, };
void* vncFbptr[MAXSCREENS] = { 0, };
int vncFbstride[MAXSCREENS];

int vncInetdSock = -1;

struct CaseInsensitiveCompare {
  bool operator() (const std::string &a, const std::string &b) const {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  }
};

typedef std::set<std::string, CaseInsensitiveCompare> ParamSet;
static ParamSet allowOverrideSet;

rfb::StringParameter httpDir("httpd",
                             "Directory containing files to serve via HTTP",
                             "");
rfb::IntParameter httpPort("httpPort", "TCP port to listen for HTTP",0);
rfb::AliasParameter rfbwait("rfbwait", "Alias for ClientWaitTimeMillis",
                            &rfb::Server::clientWaitTimeMillis);
rfb::IntParameter rfbport("rfbport", "TCP port to listen for RFB protocol",0);
rfb::StringParameter desktopName("desktop", "Name of VNC desktop","x11");
rfb::BoolParameter localhostOnly("localhost",
                                 "Only allow connections from localhost",
                                 false);
rfb::StringParameter interface("interface",
                               "listen on the specified network address",
                               "all");
rfb::BoolParameter avoidShiftNumLock("AvoidShiftNumLock",
                                     "Avoid fake Shift presses for keys affected by NumLock.",
                                     true);
rfb::StringParameter allowOverride("AllowOverride",
                                   "Comma separated list of parameters that can be modified using VNC extension.",
                                   "desktop,AcceptPointerEvents,SendCutText,AcceptCutText,SendPrimary,SetPrimary");
rfb::BoolParameter setPrimary("SetPrimary", "Set the PRIMARY as well "
                              "as the CLIPBOARD selection", true);
rfb::BoolParameter sendPrimary("SendPrimary",
                               "Send the PRIMARY as well as the CLIPBOARD selection",
                               true);

static PixelFormat vncGetPixelFormat(int scrIdx)
{
  int depth, bpp;
  int trueColour, bigEndian;
  int redMask, greenMask, blueMask;

  int redShift, greenShift, blueShift;
  int redMax, greenMax, blueMax;

  vncGetScreenFormat(scrIdx, &depth, &bpp, &trueColour, &bigEndian,
                     &redMask, &greenMask, &blueMask);

  if (!trueColour) {
    vlog.error("pseudocolour not supported");
    abort();
  }

  redShift   = ffs(redMask) - 1;
  greenShift = ffs(greenMask) - 1;
  blueShift  = ffs(blueMask) - 1;
  redMax     = redMask   >> redShift;
  greenMax   = greenMask >> greenShift;
  blueMax    = blueMask  >> blueShift;

  return PixelFormat(bpp, depth, bigEndian, trueColour,
                     redMax, greenMax, blueMax,
                     redShift, greenShift, blueShift);
}

static void parseOverrideList(const char *text, ParamSet &out)
{
  for (const char* iter = text; ; ++iter) {
    if (*iter == ',' || *iter == '\0') {
      out.insert(std::string(text, iter));
      text = iter + 1;

      if (*iter == '\0')
        break;
    }
  }
}

void vncExtensionInit(void)
{
  int ret;

  if (vncExtGeneration == vncGetServerGeneration()) {
    vlog.error("vncExtensionInit: called twice in same generation?");
    return;
  }
  vncExtGeneration = vncGetServerGeneration();

  if (vncGetScreenCount() > MAXSCREENS) {
    vlog.error("vncExtensionInit: too many screens");
    return;
  }

  if (sizeof(ShortRect) != sizeof(struct UpdateRect)) {
    vlog.error("vncExtensionInit: Incompatible ShortRect size");
    return;
  }

  ret = vncAddExtension();
  if (ret == -1)
    return;

  vncSelectionInit();

  vlog.info("VNC extension running!");

  try {
    if (!initialised) {
      rfb::initStdIOLoggers();

      parseOverrideList(allowOverride, allowOverrideSet);
      allowOverride.setImmutable();

      initialised = true;
    }

    for (int scr = 0; scr < vncGetScreenCount(); scr++) {

      if (!desktop[scr]) {
        std::list<network::TcpListener*> listeners;
        std::list<network::TcpListener*> httpListeners;
        if (scr == 0 && vncInetdSock != -1) {
          if (network::TcpSocket::isListening(vncInetdSock))
          {
            listeners.push_back(new network::TcpListener(vncInetdSock));
            vlog.info("inetd wait");
          }
        } else {
          const char *addr = interface;
          int port = rfbport;
          if (port == 0) port = 5900 + atoi(vncGetDisplay());
          port += 1000 * scr;
          if (strcasecmp(addr, "all") == 0)
            addr = 0;
          if (localhostOnly)
            network::createLocalTcpListeners(&listeners, port);
          else
            network::createTcpListeners(&listeners, addr, port);

          vlog.info("Listening for VNC connections on %s interface(s), port %d",
                    localhostOnly ? "local" : (const char*)interface,
                    port);

          CharArray httpDirStr(httpDir.getData());
          if (httpDirStr.buf[0]) {
            port = httpPort;
            if (port == 0) port = 5800 + atoi(vncGetDisplay());
            port += 1000 * scr;
            if (localhostOnly)
              network::createLocalTcpListeners(&httpListeners, port);
            else
              network::createTcpListeners(&httpListeners, addr, port);

            vlog.info("Listening for HTTP connections on %s interface(s), port %d",
                      localhostOnly ? "local" : (const char*)interface,
                      port);
          }
        }

        CharArray desktopNameStr(desktopName.getData());
        PixelFormat pf = vncGetPixelFormat(scr);

        desktop[scr] = new XserverDesktop(scr,
                                          listeners,
                                          httpListeners,
                                          desktopNameStr.buf,
                                          pf,
                                          vncGetScreenWidth(scr),
                                          vncGetScreenHeight(scr),
                                          vncFbptr[scr],
                                          vncFbstride[scr]);
        vlog.info("created VNC server for screen %d", scr);

        if (scr == 0 && vncInetdSock != -1 && listeners.empty()) {
          network::Socket* sock = new network::TcpSocket(vncInetdSock);
          desktop[scr]->addClient(sock, false);
          vlog.info("added inetd sock");
        }
      }

      vncHooksInit(scr);
    }
  } catch (rdr::Exception& e) {
    vlog.error("vncExtInit: %s",e.str());
  }

  vncRegisterBlockHandlers();
}

int vncExtensionIsActive(int scrIdx)
{
  return (desktop[scrIdx] != NULL);
}

void vncHandleSocketEvent(int fd, int scrIdx, int read, int write)
{
  desktop[scrIdx]->handleSocketEvent(fd, read, write);
}

void vncCallBlockHandlers(int* timeout)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    if (desktop[scr])
      desktop[scr]->blockHandler(timeout);
}

int vncGetAvoidShiftNumLock(void)
{
  return (bool)avoidShiftNumLock;
}

int vncGetSetPrimary(void)
{
  return (bool)setPrimary;
}

int vncGetSendPrimary(void)
{
  return (bool)sendPrimary;
}

void vncUpdateDesktopName(void)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->setDesktopName(desktopName);
  }
}

void vncServerCutText(const char *text, size_t len)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->serverCutText(text, len);
  }
}

int vncConnectClient(const char *addr)
{
  if (desktop[0] == NULL)
    return -1;

  if (strlen(addr) == 0) {
    try {
      desktop[0]->disconnectClients();
    } catch (rdr::Exception& e) {
      vlog.error("Disconnecting all clients: %s",e.str());
      return -1;
    }
    return 0;
  }

  char *host;
  int port;

  getHostAndPort(addr, &host, &port, 5500);

  try {
    network::Socket* sock = new network::TcpSocket(host, port);
    delete [] host;
    desktop[0]->addClient(sock, true);
  } catch (rdr::Exception& e) {
    vlog.error("Reverse connection: %s",e.str());
    return -1;
  }

  return 0;
}

void vncGetQueryConnect(uint32_t *opaqueId, const char**username,
                        const char **address, int *timeout)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->getQueryConnect(opaqueId, username, address, timeout);
    if (opaqueId != 0)
      break;
  }
}

void vncApproveConnection(uint32_t opaqueId, int approve)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->approveConnection(opaqueId, approve,
                                    "Connection rejected by local user");
  }
}

void vncBell()
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->bell();
  }
}

void vncAddChanged(int scrIdx, const struct UpdateRect *extents,
                   int nRects, const struct UpdateRect *rects)
{
  Region reg;

  reg.setExtentsAndOrderedRects((const ShortRect*)extents,
                                nRects, (const ShortRect*)rects);
  desktop[scrIdx]->add_changed(reg);
}

void vncAddCopied(int scrIdx, const struct UpdateRect *extents,
                  int nRects, const struct UpdateRect *rects,
                  int dx, int dy)
{
  Region reg;

  reg.setExtentsAndOrderedRects((const ShortRect*)extents,
                                nRects, (const ShortRect*)rects);
  desktop[scrIdx]->add_copied(reg, rfb::Point(dx, dy));
}

void vncSetCursor(int width, int height, int hotX, int hotY,
                  const unsigned char *rgbaData)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    if (desktop[scr] == NULL)
      continue;
    desktop[scr]->setCursor(width, height, hotX, hotY, rgbaData);
  }
}

void vncPreScreenResize(int scrIdx)
{
  // We need to prevent the RFB core from accessing the framebuffer
  // for a while as there might be updates thrown our way inside
  // the routines that change the screen (i.e. before we have a
  // pointer to the new framebuffer).
  desktop[scrIdx]->blockUpdates();
}

void vncPostScreenResize(int scrIdx, int success, int width, int height)
{
  if (success) {
    // Let the RFB core know of the new dimensions and framebuffer
    desktop[scrIdx]->setFramebuffer(width, height,
                                    vncFbptr[scrIdx], vncFbstride[scrIdx]);
  }

  desktop[scrIdx]->unblockUpdates();

  if (success) {
    // Mark entire screen as changed
    desktop[scrIdx]->add_changed(Region(Rect(0, 0, width, height)));
  }
}

void vncRefreshScreenLayout(int scrIdx)
{
  desktop[scrIdx]->refreshScreenLayout();
}

int vncOverrideParam(const char *nameAndValue)
{
  const char* equalSign = strchr(nameAndValue, '=');
  if (!equalSign)
    return 0;

  std::string key(nameAndValue, equalSign);
  if (allowOverrideSet.find(key) == allowOverrideSet.end())
    return 0;

  return rfb::Configuration::setParam(nameAndValue);
}
