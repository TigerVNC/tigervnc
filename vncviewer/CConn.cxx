/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
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

#include <assert.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <rfb/CMsgWriter.h>
#include <rfb/CSecurity.h>
#include <rfb/Hostname.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/util.h>
#include <rfb/screenTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>
#ifndef WIN32
#include <network/UnixSocket.h>
#endif

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "CConn.h"
#include "OptionsDialog.h"
#include "DesktopWindow.h"
#include "PlatformPixelBuffer.h"
#include "i18n.h"
#include "parameters.h"
#include "vncviewer.h"

#ifdef WIN32
#include "win32.h"
#endif

using namespace rdr;
using namespace rfb;
using namespace std;

static rfb::LogWriter vlog("CConn");

// 8 colours (1 bit per component)
static const PixelFormat verylowColourPF(8, 3,false, true,
                                         1, 1, 1, 2, 1, 0);
// 64 colours (2 bits per component)
static const PixelFormat lowColourPF(8, 6, false, true,
                                     3, 3, 3, 4, 2, 0);
// 256 colours (2-3 bits per component)
static const PixelFormat mediumColourPF(8, 8, false, true,
                                        7, 7, 3, 5, 2, 0);

CConn::CConn(const char* vncServerName, network::Socket* socket=NULL)
  : serverHost(0), serverPort(0), desktop(NULL),
    updateCount(0), pixelCount(0),
    lastServerEncoding((unsigned int)-1)
{
  setShared(::shared);
  sock = socket;

  supportsLocalCursor = true;
  supportsDesktopResize = true;
  supportsLEDState = true;

  if (customCompressLevel)
    setCompressLevel(::compressLevel);

  if (!noJpeg)
    setQualityLevel(::qualityLevel);

  if(sock == NULL) {
    try {
#ifndef WIN32
      if (strchr(vncServerName, '/') != NULL) {
        sock = new network::UnixSocket(vncServerName);
        serverHost = sock->getPeerAddress();
        vlog.info(_("Connected to socket %s"), serverHost);
      } else
#endif
      {
        getHostAndPort(vncServerName, &serverHost, &serverPort);

        sock = new network::TcpSocket(serverHost, serverPort);
        vlog.info(_("Connected to host %s port %d"), serverHost, serverPort);
      }
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      if (alertOnFatalError)
        fl_alert("%s", e.str());
      exit_vncviewer();
      return;
    }
  }

  Fl::add_fd(sock->getFd(), FL_READ | FL_EXCEPT, socketEvent, this);

  // See callback below
  sock->inStream().setBlockCallback(this);

  setServerName(serverHost);
  setStreams(&sock->inStream(), &sock->outStream());

  initialiseProtocol();

  OptionsDialog::addCallback(handleOptions, this);
}

CConn::~CConn()
{
  OptionsDialog::removeCallback(handleOptions);
  Fl::remove_timeout(handleUpdateTimeout, this);

  if (desktop)
    delete desktop;

  delete [] serverHost;
  if (sock)
    Fl::remove_fd(sock->getFd());
  delete sock;
}

const char *CConn::connectionInfo()
{
  static char infoText[1024] = "";

  char scratch[100];
  char pfStr[100];

  // Crude way of avoiding constant overflow checks
  assert((sizeof(scratch) + 1) * 10 < sizeof(infoText));

  infoText[0] = '\0';

  snprintf(scratch, sizeof(scratch),
           _("Desktop name: %.80s"), server.name());
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Host: %.80s port: %d"), serverHost, serverPort);
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Size: %d x %d"), server.width(), server.height());
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  // TRANSLATORS: Will be filled in with a string describing the
  // protocol pixel format in a fairly language neutral way
  server.pf().print(pfStr, 100);
  snprintf(scratch, sizeof(scratch),
           _("Pixel format: %s"), pfStr);
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  // TRANSLATORS: Similar to the earlier "Pixel format" string
  serverPF.print(pfStr, 100);
  snprintf(scratch, sizeof(scratch),
           _("(server default %s)"), pfStr);
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Requested encoding: %s"), encodingName(getPreferredEncoding()));
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Last used encoding: %s"), encodingName(lastServerEncoding));
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Line speed estimate: %d kbit/s"), sock->inStream().kbitsPerSecond());
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Protocol version: %d.%d"), server.majorVersion, server.minorVersion);
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  snprintf(scratch, sizeof(scratch),
           _("Security method: %s"), secTypeName(csecurity->getType()));
  strcat(infoText, scratch);
  strcat(infoText, "\n");

  return infoText;
}

unsigned CConn::getUpdateCount()
{
  return updateCount;
}

unsigned CConn::getPixelCount()
{
  return pixelCount;
}

unsigned CConn::getPosition()
{
  return sock->inStream().pos();
}

// The RFB core is not properly asynchronous, so it calls this callback
// whenever it needs to block to wait for more data. Since FLTK is
// monitoring the socket, we just make sure FLTK gets to run.

void CConn::blockCallback()
{
  run_mainloop();

  if (should_exit())
    throw rdr::Exception("Termination requested");
}

void CConn::socketEvent(FL_SOCKET fd, void *data)
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

      // Make sure that the FLTK handling and the timers gets some CPU
      // time in case of back to back messages
       Fl::check();
       Timer::checkTimeouts();

       // Also check if we need to stop reading and terminate
       if (should_exit())
         break;
    } while (cc->sock->inStream().checkNoWait(1));
  } catch (rdr::EndOfStream& e) {
    vlog.info("%s", e.str());
    exit_vncviewer();
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    // Somebody might already have requested us to terminate, and
    // might have already provided an error message.
    if (!should_exit())
      exit_vncviewer(e.str());
  }

  recursing = false;
}

////////////////////// CConnection callback methods //////////////////////

// initDone() is called when the serverInit message has been received.  At
// this point we create the desktop window and display it.  We also tell the
// server the pixel format and encodings to use and request the first update.
void CConn::initDone()
{
  // If using AutoSelect with old servers, start in FullColor
  // mode. See comment in autoSelectFormatAndEncoding. 
  if (server.beforeVersion(3, 8) && autoSelect)
    fullColour.setParam(true);

  serverPF = server.pf();

  desktop = new DesktopWindow(server.width(), server.height(),
                              server.name(), serverPF, this);
  fullColourPF = desktop->getPreferredPF();

  // Force a switch to the format and encoding we'd like
  updatePixelFormat();
  int encNum = encodingNum(::preferredEncoding);
  if (encNum != -1)
    setPreferredEncoding(encNum);
}

// setDesktopSize() is called when the desktop size changes (including when
// it is set initially).
void CConn::setDesktopSize(int w, int h)
{
  CConnection::setDesktopSize(w,h);
  resizeFramebuffer();
}

// setExtendedDesktopSize() is a more advanced version of setDesktopSize()
void CConn::setExtendedDesktopSize(unsigned reason, unsigned result,
                                   int w, int h, const rfb::ScreenSet& layout)
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
  desktop->setName(name);
}

// framebufferUpdateStart() is called at the beginning of an update.
// Here we try to send out a new framebuffer update request so that the
// next update can be sent out in parallel with us decoding the current
// one.
void CConn::framebufferUpdateStart()
{
  CConnection::framebufferUpdateStart();

  // Update the screen prematurely for very slow updates
  Fl::add_timeout(1.0, handleUpdateTimeout, this);
}

// framebufferUpdateEnd() is called at the end of an update.
// For each rectangle, the FdInStream will have timed the speed
// of the connection, allowing us to select format and encoding
// appropriately, and then request another incremental update.
void CConn::framebufferUpdateEnd()
{
  CConnection::framebufferUpdateEnd();

  updateCount++;

  Fl::remove_timeout(handleUpdateTimeout, this);
  desktop->updateWindow();

  // Compute new settings based on updated bandwidth values
  if (autoSelect)
    autoSelectFormatAndEncoding();
}

// The rest of the callbacks are fairly self-explanatory...

void CConn::setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs)
{
  vlog.error(_("Invalid SetColourMapEntries from server!"));
}

void CConn::bell()
{
  fl_beep();
}

void CConn::dataRect(const Rect& r, int encoding)
{
  sock->inStream().startTiming();

  if (encoding != encodingCopyRect)
    lastServerEncoding = encoding;

  CConnection::dataRect(r, encoding);

  sock->inStream().stopTiming();

  pixelCount += r.area();
}

void CConn::setCursor(int width, int height, const Point& hotspot,
                      const rdr::U8* data)
{
  desktop->setCursor(width, height, hotspot, data);
}

void CConn::fence(rdr::U32 flags, unsigned len, const char data[])
{
  CMsgHandler::fence(flags, len, data);

  if (flags & fenceFlagRequest) {
    // We handle everything synchronously so we trivially honor these modes
    flags = flags & (fenceFlagBlockBefore | fenceFlagBlockAfter);

    writer()->writeFence(flags, len, data);
    return;
  }
}

void CConn::setLEDState(unsigned int state)
{
  CConnection::setLEDState(state);

  desktop->setLEDState(state);
}

void CConn::handleClipboardRequest()
{
  desktop->handleClipboardRequest();
}

void CConn::handleClipboardAnnounce(bool available)
{
  desktop->handleClipboardAnnounce(available);
}

void CConn::handleClipboardData(const char* data)
{
  desktop->handleClipboardData(data);
}


////////////////////// Internal methods //////////////////////

void CConn::resizeFramebuffer()
{
  desktop->resizeFramebuffer(server.width(), server.height());
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
  int newQualityLevel = ::qualityLevel;

  // Always use Tight
  setPreferredEncoding(encodingTight);

  // Check that we have a decent bandwidth measurement
  if ((kbitsPerSecond == 0) || (timeWaited < 10000))
    return;

  // Select appropriate quality level
  if (!noJpeg) {
    if (kbitsPerSecond > 16000)
      newQualityLevel = 8;
    else
      newQualityLevel = 6;

    if (newQualityLevel != ::qualityLevel) {
      vlog.info(_("Throughput %d kbit/s - changing to quality %d"),
                kbitsPerSecond, newQualityLevel);
      ::qualityLevel.setParam(newQualityLevel);
      setQualityLevel(newQualityLevel);
    }
  }

  if (server.beforeVersion(3, 8)) {
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
    if (newFullColour)
      vlog.info(_("Throughput %d kbit/s - full color is now enabled"),
                kbitsPerSecond);
    else
      vlog.info(_("Throughput %d kbit/s - full color is now disabled"),
                kbitsPerSecond);
    fullColour.setParam(newFullColour);
    updatePixelFormat();
  } 
}

// requestNewUpdate() requests an update from the server, having set the
// format and encoding appropriately.
void CConn::updatePixelFormat()
{
  PixelFormat pf;

  if (fullColour) {
    pf = fullColourPF;
  } else {
    if (lowColourLevel == 0)
      pf = verylowColourPF;
    else if (lowColourLevel == 1)
      pf = lowColourPF;
    else
      pf = mediumColourPF;
  }

  char str[256];
  pf.print(str, 256);
  vlog.info(_("Using pixel format %s"),str);
  setPF(pf);
}

void CConn::handleOptions(void *data)
{
  CConn *self = (CConn*)data;

  // Checking all the details of the current set of encodings is just
  // a pain. Assume something has changed, as resending the encoding
  // list is cheap. Avoid overriding what the auto logic has selected
  // though.
  if (!autoSelect) {
    int encNum = encodingNum(::preferredEncoding);

    if (encNum != -1)
      self->setPreferredEncoding(encNum);
  }

  if (customCompressLevel)
    self->setCompressLevel(::compressLevel);
  else
    self->setCompressLevel(-1);

  if (!noJpeg && !autoSelect)
    self->setQualityLevel(::qualityLevel);
  else
    self->setQualityLevel(-1);

  self->updatePixelFormat();
}

void CConn::handleUpdateTimeout(void *data)
{
  CConn *self = (CConn *)data;

  assert(self);

  self->desktop->updateWindow();

  Fl::repeat_timeout(1.0, handleUpdateTimeout, data);
}
