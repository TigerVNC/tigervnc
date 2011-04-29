/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <assert.h>
#include <unistd.h>

#include <rfb/CMsgWriter.h>
#include <rfb/encodings.h>
#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/screenTypes.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "CConn.h"
#include "i18n.h"
#include "parameters.h"

using namespace rdr;
using namespace rfb;
using namespace std;

extern void exit_vncviewer();

static rfb::LogWriter vlog("CConn");

CConn::CConn(const char* vncServerName)
  : serverHost(0), serverPort(0), sock(NULL), desktop(NULL),
    currentEncoding(encodingTight), lastServerEncoding((unsigned int)-1),
    formatChange(false), encodingChange(false),
    firstUpdate(true), pendingUpdate(false),
    forceNonincremental(false)
{
  setShared(::shared);

  int encNum = encodingNum(preferredEncoding);
  if (encNum != -1)
    currentEncoding = encNum;

  cp.supportsDesktopResize = true;
  cp.supportsExtendedDesktopSize = true;
  cp.supportsDesktopRename = true;
  cp.supportsLocalCursor = useLocalCursor;

  cp.customCompressLevel = customCompressLevel;
  cp.compressLevel = compressLevel;

  cp.noJpeg = noJpeg;
  cp.qualityLevel = qualityLevel;

  try {
    getHostAndPort(vncServerName, &serverHost, &serverPort);

    sock = new network::TcpSocket(serverHost, serverPort);
    vlog.info(_("connected to host %s port %d"), serverHost, serverPort);
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
    fl_alert(e.str());
    exit_vncviewer();
    return;
  }

  Fl::add_fd(sock->getFd(), FL_READ | FL_EXCEPT, socketEvent, this);

  // See callback below
  sock->inStream().setBlockCallback(this);

  setServerName(serverHost);
  setStreams(&sock->inStream(), &sock->outStream());

  initialiseProtocol();
}

CConn::~CConn()
{
  free(serverHost);
  if (sock)
    Fl::remove_fd(sock->getFd());
  delete sock;
}

void CConn::refreshFramebuffer()
{
  // FIXME: We cannot safely trigger an update request directly but must
  //        wait for the next update to arrive.
  if (!formatChange)
    forceNonincremental = true;
}

// The RFB core is not properly asynchronous, so it calls this callback
// whenever it needs to block to wait for more data. Since FLTK is
// monitoring the socket, we just make sure FLTK gets to run.

void CConn::blockCallback()
{
  int next_timer;

  next_timer = Timer::checkTimeouts();
  if (next_timer == 0)
    next_timer = INT_MAX;

  Fl::wait((double)next_timer / 1000.0);
}

void CConn::socketEvent(int fd, void *data)
{
  CConn *cc;
  static bool recursing = false;

  assert(data);
  cc = (CConn*)data;

  // I don't think processMsg() is recursion safe, so add this check
  if (recursing)
    return;

  recursing = true;

  try {
    // processMsg() only processes one message, so we need to loop
    // until the buffers are empty or things will stall.
    do {
      cc->processMsg();
    } while (cc->sock->inStream().checkNoWait(1));
  } catch (rdr::EndOfStream& e) {
    vlog.info(e.str());
    exit_vncviewer();
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
    fl_alert(e.str());
    exit_vncviewer();
  }

  recursing = false;
}

////////////////////// CConnection callback methods //////////////////////

// serverInit() is called when the serverInit message has been received.  At
// this point we create the desktop window and display it.  We also tell the
// server the pixel format and encodings to use and request the first update.
void CConn::serverInit()
{
  CConnection::serverInit();

  // If using AutoSelect with old servers, start in FullColor
  // mode. See comment in autoSelectFormatAndEncoding. 
  if (cp.beforeVersion(3, 8) && autoSelect)
    fullColour.setParam(true);

  serverPF = cp.pf();

  desktop = new DesktopWindow(cp.width, cp.height, cp.name(), serverPF, this);
  fullColourPF = desktop->getPreferredPF();

  formatChange = encodingChange = true;
  requestNewUpdate();
}

// setDesktopSize() is called when the desktop size changes (including when
// it is set initially).
void CConn::setDesktopSize(int w, int h)
{
  CConnection::setDesktopSize(w,h);
  resizeFramebuffer();
}

// setExtendedDesktopSize() is a more advanced version of setDesktopSize()
void CConn::setExtendedDesktopSize(int reason, int result, int w, int h,
                                   const rfb::ScreenSet& layout)
{
  CConnection::setExtendedDesktopSize(reason, result, w, h, layout);

  if ((reason == reasonClient) && (result != resultSuccess)) {
    vlog.error(_("SetDesktopSize failed: %d"), result);
    return;
  }

  resizeFramebuffer();
}

// setName() is called when the desktop name changes
void CConn::setName(const char* name)
{
  CConnection::setName(name);
  if (desktop)
    desktop->setName(name);
}

// framebufferUpdateStart() is called at the beginning of an update.
// Here we try to send out a new framebuffer update request so that the
// next update can be sent out in parallel with us decoding the current
// one. We cannot do this if we're in the middle of a format change
// though.
void CConn::framebufferUpdateStart()
{
  if (!formatChange) {
    pendingUpdate = true;
    requestNewUpdate();
  } else
    pendingUpdate = false;
}

// framebufferUpdateEnd() is called at the end of an update.
// For each rectangle, the FdInStream will have timed the speed
// of the connection, allowing us to select format and encoding
// appropriately, and then request another incremental update.
void CConn::framebufferUpdateEnd()
{
  desktop->updateWindow();

  if (firstUpdate) {
    int width, height;

    if (cp.supportsSetDesktopSize &&
        sscanf(desktopSize.getValueStr(), "%dx%d", &width, &height) == 2) {
      ScreenSet layout;

      layout = cp.screenLayout;

      if (layout.num_screens() == 0)
        layout.add_screen(rfb::Screen());
      else if (layout.num_screens() != 1) {
        ScreenSet::iterator iter;

        while (true) {
          iter = layout.begin();
          ++iter;

          if (iter == layout.end())
            break;

          layout.remove_screen(iter->id);
        }
      }

      layout.begin()->dimensions.tl.x = 0;
      layout.begin()->dimensions.tl.y = 0;
      layout.begin()->dimensions.br.x = width;
      layout.begin()->dimensions.br.y = height;

      writer()->writeSetDesktopSize(width, height, layout);
    }

    firstUpdate = false;
  }

  // A format change prevented us from sending this before the update,
  // so make sure to send it now.
  if (formatChange && !pendingUpdate)
    requestNewUpdate();

  // Compute new settings based on updated bandwidth values
  if (autoSelect)
    autoSelectFormatAndEncoding();

  // Make sure that the FLTK handling and the timers gets some CPU time
  // in case of back to back framebuffer updates.
  Fl::check();
  Timer::checkTimeouts();
}

// The rest of the callbacks are fairly self-explanatory...

void CConn::setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs)
{
  desktop->setColourMapEntries(firstColour, nColours, rgbs);
}

void CConn::bell()
{
  fl_beep();
}

void CConn::serverCutText(const char* str, rdr::U32 len)
{
//  desktop->serverCutText(str,len);
}

// We start timing on beginRect and stop timing on endRect, to
// avoid skewing the bandwidth estimation as a result of the server
// being slow or the network having high latency
void CConn::beginRect(const Rect& r, int encoding)
{
  sock->inStream().startTiming();
  if (encoding != encodingCopyRect) {
    lastServerEncoding = encoding;
  }
}

void CConn::endRect(const Rect& r, int encoding)
{
  sock->inStream().stopTiming();
}

void CConn::fillRect(const rfb::Rect& r, rfb::Pixel p)
{
  desktop->fillRect(r,p);
}
void CConn::imageRect(const rfb::Rect& r, void* p)
{
  desktop->imageRect(r,p);
}
void CConn::copyRect(const rfb::Rect& r, int sx, int sy)
{
  desktop->copyRect(r,sx,sy);
}
void CConn::setCursor(int width, int height, const Point& hotspot,
                      void* data, void* mask)
{
//  desktop->setCursor(width, height, hotspot, data, mask);
}

////////////////////// Internal methods //////////////////////

void CConn::resizeFramebuffer()
{
/*
  if (!desktop)
    return;
  if ((desktop->width() == cp.width) && (desktop->height() == cp.height))
    return;

  desktop->resize(cp.width, cp.height);
*/
}

// autoSelectFormatAndEncoding() chooses the format and encoding appropriate
// to the connection speed:
//
//   First we wait for at least one second of bandwidth measurement.
//
//   Above 16Mbps (i.e. LAN), we choose the second highest JPEG quality,
//   which should be perceptually lossless.
//
//   If the bandwidth is below that, we choose a more lossy JPEG quality.
//
//   If the bandwidth drops below 256 Kbps, we switch to palette mode.
//
//   Note: The system here is fairly arbitrary and should be replaced
//         with something more intelligent at the server end.
//
void CConn::autoSelectFormatAndEncoding()
{
  int kbitsPerSecond = sock->inStream().kbitsPerSecond();
  unsigned int timeWaited = sock->inStream().timeWaited();
  bool newFullColour = fullColour;
  int newQualityLevel = qualityLevel;

  // Always use Tight
  if (currentEncoding != encodingTight) {
    currentEncoding = encodingTight;
    encodingChange = true;
  }

  // Check that we have a decent bandwidth measurement
  if ((kbitsPerSecond == 0) || (timeWaited < 10000))
    return;

  // Select appropriate quality level
  if (!noJpeg) {
    if (kbitsPerSecond > 16000)
      newQualityLevel = 8;
    else
      newQualityLevel = 6;

    if (newQualityLevel != qualityLevel) {
      vlog.info(_("Throughput %d kbit/s - changing to quality %d"),
                kbitsPerSecond, newQualityLevel);
      cp.qualityLevel = newQualityLevel;
      qualityLevel.setParam(newQualityLevel);
      encodingChange = true;
    }
  }

  if (cp.beforeVersion(3, 8)) {
    // Xvnc from TightVNC 1.2.9 sends out FramebufferUpdates with
    // cursors "asynchronously". If this happens in the middle of a
    // pixel format change, the server will encode the cursor with
    // the old format, but the client will try to decode it
    // according to the new format. This will lead to a
    // crash. Therefore, we do not allow automatic format change for
    // old servers.
    return;
  }
  
  // Select best color level
  newFullColour = (kbitsPerSecond > 256);
  if (newFullColour != fullColour) {
    vlog.info(_("Throughput %d kbit/s - full color is now %s"), 
              kbitsPerSecond,
              newFullColour ? _("enabled") : _("disabled"));
    fullColour.setParam(newFullColour);
    formatChange = true;
  } 
}

// checkEncodings() sends a setEncodings message if one is needed.
void CConn::checkEncodings()
{
  if (encodingChange && writer()) {
    vlog.info(_("Using %s encoding"),encodingName(currentEncoding));
    writer()->writeSetEncodings(currentEncoding, true);
    encodingChange = false;
  }
}

// requestNewUpdate() requests an update from the server, having set the
// format and encoding appropriately.
void CConn::requestNewUpdate()
{
  if (formatChange) {
    PixelFormat pf;

    /* Catch incorrect requestNewUpdate calls */
    assert(pendingUpdate == false);

    if (fullColour) {
      pf = fullColourPF;
    } else {
      if (lowColourLevel == 0)
        pf = PixelFormat(8,3,0,1,1,1,1,2,1,0);
      else if (lowColourLevel == 1)
        pf = PixelFormat(8,6,0,1,3,3,3,4,2,0);
      else
        pf = PixelFormat(8,8,0,0);
    }
    char str[256];
    pf.print(str, 256);
    vlog.info(_("Using pixel format %s"),str);
    desktop->setServerPF(pf);
    cp.setPF(pf);
    writer()->writeSetPixelFormat(pf);

    forceNonincremental = true;

    formatChange = false;
  }

  checkEncodings();

  writer()->writeFramebufferUpdateRequest(Rect(0, 0, cp.width, cp.height),
                                          !forceNonincremental);
 
  forceNonincremental = false;
}
