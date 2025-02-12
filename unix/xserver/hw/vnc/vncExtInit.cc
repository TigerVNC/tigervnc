/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2024 Pierre Ossman for Cendio AB
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
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <string>

#include <core/Configuration.h>
#include <core/Logger_stdio.h>
#include <core/LogWriter.h>
#include <core/Region.h>

#include <rfb/ServerCore.h>
#include <rdr/HexOutStream.h>
#include <rfb/ledStates.h>

#include <network/TcpSocket.h>
#include <network/UnixSocket.h>

#include "XserverDesktop.h"
#include "vncExtInit.h"
#include "vncHooks.h"
#include "vncBlockHandler.h"
#include "vncSelection.h"
#include "XorgGlue.h"
#include "RandrGlue.h"
#include "xorg-version.h"

extern "C" {
void vncSetGlueContext(int screenIndex);
}

static core::LogWriter vlog("vncext");

// We can't safely get this from Xorg
#define MAXSCREENS 16

static unsigned long vncExtGeneration = 0;
static bool initialised = false;
static XserverDesktop* desktop[MAXSCREENS] = { 0, };
void* vncFbptr[MAXSCREENS] = { 0, };
int vncFbstride[MAXSCREENS];

int vncInetdSock = -1;

static const char* defaultDesktopName();

core::IntParameter
  rfbport("rfbport",
          "TCP port to listen for RFB protocol", 0, -1, 65535);
core::StringParameter
  rfbunixpath("rfbunixpath",
              "Unix socket to listen for RFB protocol", "");
core::IntParameter
  rfbunixmode("rfbunixmode",
              "Unix socket access mode", 0600, 0000, 0777);
core::StringParameter
  desktopName("desktop", "Name of VNC desktop", defaultDesktopName());
core::BoolParameter
  localhostOnly("localhost",
                "Only allow connections from localhost", false);
core::StringParameter
  interface("interface",
            "Listen on the specified network address", "all");
core::BoolParameter
  avoidShiftNumLock("AvoidShiftNumLock",
                    "Avoid fake Shift presses for keys affected by "
                    "NumLock.", true);
core::StringListParameter
  allowOverride("AllowOverride",
                "Comma separated list of parameters that can be "
                "modified using VNC extension.",
                {"desktop", "AcceptPointerEvents", "SendCutText",
                 "AcceptCutText", "SendPrimary", "SetPrimary"});
core::BoolParameter
  setPrimary("SetPrimary",
             "Set the PRIMARY as well as the CLIPBOARD selection",
             true);
core::BoolParameter
  sendPrimary("SendPrimary",
              "Send the PRIMARY as well as the CLIPBOARD selection",
              true);

static const char* defaultDesktopName()
{
  size_t host_max = sysconf(_SC_HOST_NAME_MAX);
  if (host_max < 0)
    return "";

  std::vector<char> hostname(host_max + 1);
  if (gethostname(hostname.data(), hostname.size()) == -1)
    return "";

  struct passwd* pwent = getpwuid(getuid());
  if (pwent == nullptr)
    return "";

  size_t len = snprintf(nullptr, 0, "%s@%s", pwent->pw_name, hostname.data());
  if (len < 0)
    return "";

  char* name = new char[len + 1];

  snprintf(name, len + 1, "%s@%s", pwent->pw_name, hostname.data());

  return name;
}

static rfb::PixelFormat vncGetPixelFormat(int scrIdx)
{
  int depth, bpp;
  int trueColour, bigEndian;
  int redMask, greenMask, blueMask;

  int redShift, greenShift, blueShift;
  int redMax, greenMax, blueMax;

  vncGetScreenFormat(scrIdx, &depth, &bpp, &trueColour, &bigEndian,
                     &redMask, &greenMask, &blueMask);

  if (!trueColour) {
    vlog.error("Pseudocolour not supported");
    abort();
  }

  redShift   = ffs(redMask) - 1;
  greenShift = ffs(greenMask) - 1;
  blueShift  = ffs(blueMask) - 1;
  redMax     = redMask   >> redShift;
  greenMax   = greenMask >> greenShift;
  blueMax    = blueMask  >> blueShift;

  return rfb::PixelFormat(bpp, depth, bigEndian, trueColour,
                          redMax, greenMax, blueMax,
                          redShift, greenShift, blueShift);
}

void vncExtensionInit(void)
{
  if (vncExtGeneration == vncGetServerGeneration()) {
    vlog.error("vncExtensionInit: Called twice in same generation?");
    return;
  }
  vncExtGeneration = vncGetServerGeneration();

  if (vncGetScreenCount() > MAXSCREENS)
    vncFatalError("vncExtensionInit: Too many screens\n");

  vncAddExtension();

  vncSelectionInit();

  vlog.info("VNC extension running!");

  try {
    if (!initialised) {
      core::initStdIOLoggers();

      allowOverride.setImmutable();

      initialised = true;
    }

    for (int scr = 0; scr < vncGetScreenCount(); scr++) {

      if (!desktop[scr]) {
        std::list<network::SocketListener*> listeners;
        bool inetd = false;
        if (scr == 0 && vncInetdSock != -1) {
          inetd = true;
          if (network::isSocketListening(vncInetdSock))
          {
            listeners.push_back(new network::TcpListener(vncInetdSock));
            vlog.info("inetd wait");
          }
        }

        if (!inetd && ((const char*)rfbunixpath)[0] != '\0') {
          char path[PATH_MAX];
          int mode = (int)rfbunixmode;

          if (scr == 0)
            strncpy(path, rfbunixpath, sizeof(path));
          else
            snprintf(path, sizeof(path), "%s.%d",
                     (const char*)rfbunixpath, scr);
          path[sizeof(path)-1] = '\0';

          listeners.push_back(new network::UnixListener(path, mode));

          vlog.info("Listening for VNC connections on %s (mode %04o)",
                    path, mode);
        }

        if (!inetd && rfbport != -1) {
          std::list<network::SocketListener*> tcp_listeners;
          const char *addr = interface;
          int port = rfbport;
          if (port == 0) port = 5900 + atoi(vncGetDisplay());
          port += 1000 * scr;
          if (strcasecmp(addr, "all") == 0)
            addr = 0;
          if (localhostOnly)
            network::createLocalTcpListeners(&tcp_listeners, port);
          else
            network::createTcpListeners(&tcp_listeners, addr, port);

          if (!tcp_listeners.empty()) {
            listeners.splice (listeners.end(), tcp_listeners);
            vlog.info("Listening for VNC connections on %s interface(s), port %d",
                      localhostOnly ? "local" : (const char*)interface,
                      port);
          }
        }

        if (!inetd && listeners.empty())
          throw std::runtime_error("No path or port configured for incoming connections");

        rfb::PixelFormat pf = vncGetPixelFormat(scr);

        vncSetGlueContext(scr);
        desktop[scr] = new XserverDesktop(scr,
                                          listeners,
                                          desktopName,
                                          pf,
                                          vncGetScreenWidth(),
                                          vncGetScreenHeight(),
                                          vncFbptr[scr],
                                          vncFbstride[scr]);
        vlog.info("Created VNC server for screen %d", scr);

        if (scr == 0 && vncInetdSock != -1 && listeners.empty()) {
          network::Socket* sock = new network::TcpSocket(vncInetdSock);
          desktop[scr]->addClient(sock, false, false);
          vlog.info("Added inetd sock");
        }
      }

      vncHooksInit(scr);
    }
  } catch (std::exception& e) {
    vncFatalError("vncExtInit: %s\n",e.what());
  }

  vncRegisterBlockHandlers();
}

void vncExtensionClose(void)
{
  try {
    for (int scr = 0; scr < vncGetScreenCount(); scr++) {
      delete desktop[scr];
      desktop[scr] = nullptr;
    }
  } catch (std::exception& e) {
    vncFatalError("vncExtInit: %s\n",e.what());
  }
}

void vncHandleSocketEvent(int fd, int scrIdx, int read, int write)
{
  desktop[scrIdx]->handleSocketEvent(fd, read, write);
}

void vncCallBlockHandlers(int* timeout)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
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
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->setDesktopName(desktopName);
}

void vncRequestClipboard(void)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->requestClipboard();
}

void vncAnnounceClipboard(int available)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->announceClipboard(available);
}

void vncSendClipboardData(const char* data)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->sendClipboardData(data);
}

int vncConnectClient(const char *addr, int viewOnly)
{
  if (strlen(addr) == 0) {
    try {
      desktop[0]->disconnectClients();
    } catch (std::exception& e) {
      vlog.error("Disconnecting all clients: %s", e.what());
      return -1;
    }
    return 0;
  }

  std::string host;
  int port;

  network::getHostAndPort(addr, &host, &port, 5500);

  try {
    network::Socket* sock = new network::TcpSocket(host.c_str(), port);
    vlog.info("Reverse connection: %s:%d%s", host.c_str(), port,
              viewOnly ? " (view only)" : "");
    desktop[0]->addClient(sock, true, (bool)viewOnly);
  } catch (std::exception& e) {
    vlog.error("Reverse connection: %s", e.what());
    return -1;
  }

  return 0;
}

void vncGetQueryConnect(uint32_t *opaqueId, const char**username,
                        const char **address, int *timeout)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    desktop[scr]->getQueryConnect(opaqueId, username, address, timeout);
    if (opaqueId != 0)
      break;
  }
}

void vncApproveConnection(uint32_t opaqueId, int approve)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++) {
    desktop[scr]->approveConnection(opaqueId, approve,
                                    "Connection rejected by local user");
  }
}

void vncBell()
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->bell();
}

void vncSetLEDState(unsigned long leds)
{
  unsigned int state;

  state = 0;
  if (leds & (1 << 0))
    state |= rfb::ledCapsLock;
  if (leds & (1 << 1))
    state |= rfb::ledNumLock;
  if (leds & (1 << 2))
    state |= rfb::ledScrollLock;

  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->setLEDState(state);
}

void vncAddChanged(int scrIdx, int nRects,
                   const struct UpdateRect *rects)
{
  for (int i = 0;i < nRects;i++) {
    desktop[scrIdx]->add_changed({{rects[i].x1, rects[i].y1,
                                   rects[i].x2, rects[i].y2}});
  }
}

void vncAddCopied(int scrIdx, int nRects,
                  const struct UpdateRect *rects,
                  int dx, int dy)
{
  for (int i = 0;i < nRects;i++) {
    desktop[scrIdx]->add_copied({{rects[i].x1, rects[i].y1,
                                  rects[i].x2, rects[i].y2}},
                                {dx, dy});
  }
}

void vncSetCursorSprite(int width, int height, int hotX, int hotY,
                        const unsigned char *rgbaData)
{
  for (int scr = 0; scr < vncGetScreenCount(); scr++)
    desktop[scr]->setCursor(width, height, hotX, hotY, rgbaData);
}

void vncSetCursorPos(int scrIdx, int x, int y)
{
  desktop[scrIdx]->setCursorPos(x, y, true);
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
    try {
      desktop[scrIdx]->setFramebuffer(width, height,
                                      vncFbptr[scrIdx],
                                      vncFbstride[scrIdx]);
    } catch (std::exception& e) {
      vncFatalError("vncPostScreenResize: %s\n", e.what());
    }
  }

  desktop[scrIdx]->unblockUpdates();

  if (success) {
    // Mark entire screen as changed
    desktop[scrIdx]->add_changed({{0, 0, width, height}});
  }
}

void vncRefreshScreenLayout(int scrIdx)
{
  try {
    desktop[scrIdx]->refreshScreenLayout();
  } catch (std::exception& e) {
    vncFatalError("vncRefreshScreenLayout: %s\n", e.what());
  }
}

uint64_t vncGetMsc(int scrIdx)
{
  try {
    return desktop[scrIdx]->getMsc();
  } catch (std::exception& e) {
    vncFatalError("vncGetMsc: %s\n", e.what());
  }
}

void vncQueueMsc(int scrIdx, uint64_t id, uint64_t msc)
{
  try {
    desktop[scrIdx]->queueMsc(id, msc);
  } catch (std::exception& e) {
    vncFatalError("vncQueueMsc: %s\n", e.what());
  }
}

void vncAbortMsc(int scrIdx, uint64_t id)
{
  try {
    desktop[scrIdx]->abortMsc(id);
  } catch (std::exception& e) {
    vncFatalError("vncAbortMsc: %s\n", e.what());
  }
}

int vncOverrideParam(const char *param, const char *value)
{
  for (const char* allowed : allowOverride) {
    if (strcasecmp(allowed, param) == 0)
      return core::Configuration::setParam(param, value);
  }

  return 0;
}
