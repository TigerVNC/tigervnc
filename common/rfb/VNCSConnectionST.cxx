/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2015 Pierre Ossman for Cendio AB
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

// Debug output on what the congestion control is up to
#undef CONGESTION_DEBUG

#include <sys/time.h>

#ifdef CONGESTION_DEBUG
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include <network/TcpSocket.h>
#include <rfb/VNCSConnectionST.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/screenTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/ServerCore.h>
#include <rfb/ComparingUpdateTracker.h>
#include <rfb/KeyRemapper.h>
#include <rfb/Encoder.h>
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>

using namespace rfb;

static LogWriter vlog("VNCSConnST");

// This window should get us going fairly fast on a decent bandwidth network.
// If it's too high, it will rapidly be reduced and stay low.
static const unsigned INITIAL_WINDOW = 16384;

// TCP's minimal window is 3*MSS. But since we don't know the MSS, we
// make a guess at 4 KiB (it's probaly a bit higher).
static const unsigned MINIMUM_WINDOW = 4096;

// The current default maximum window for Linux (4 MiB). Should be a good
// limit for now...
static const unsigned MAXIMUM_WINDOW = 4194304;

struct RTTInfo {
  struct timeval tv;
  int offset;
  unsigned inFlight;
};

VNCSConnectionST::VNCSConnectionST(VNCServerST* server_, network::Socket *s,
                                   bool reverse)
  : sock(s), reverseConnection(reverse),
    queryConnectTimer(this), inProcessMessages(false),
    pendingSyncFence(false), syncFence(false), fenceFlags(0),
    fenceDataLen(0), fenceData(NULL),
    baseRTT(-1), minRTT(-1), seenCongestion(false), pingCounter(0),
    ackedOffset(0), sentOffset(0), congWindow(0), congestionTimer(this),
    server(server_), updates(false),
    drawRenderedCursor(false), removeRenderedCursor(false),
    continuousUpdates(false), encodeManager(this),
    updateTimer(this), pointerEventTime(0),
    accessRights(AccessDefault), startTime(time(0))
{
  setStreams(&sock->inStream(), &sock->outStream());
  peerEndpoint.buf = sock->getPeerEndpoint();
  VNCServerST::connectionsLog.write(1,"accepted: %s", peerEndpoint.buf);

  // Configure the socket
  setSocketTimeouts();
  lastEventTime = time(0);

  server->clients.push_front(this);
}


VNCSConnectionST::~VNCSConnectionST()
{
  // If we reach here then VNCServerST is deleting us!
  VNCServerST::connectionsLog.write(1,"closed: %s (%s)",
                                    peerEndpoint.buf,
                                    (closeReason.buf) ? closeReason.buf : "");

  // Release any keys the client still had pressed
  std::set<rdr::U32>::iterator i;
  for (i=pressedKeys.begin(); i!=pressedKeys.end(); i++)
    server->desktop->keyEvent(*i, false);
  if (server->pointerClient == this)
    server->pointerClient = 0;

  // Remove this client from the server
  server->clients.remove(this);

  delete [] fenceData;
}


// Methods called from VNCServerST

bool VNCSConnectionST::init()
{
  try {
    initialiseProtocol();
  } catch (rdr::Exception& e) {
    close(e.str());
    return false;
  }
  return true;
}

void VNCSConnectionST::close(const char* reason)
{
  // Log the reason for the close
  if (!closeReason.buf)
    closeReason.buf = strDup(reason);
  else
    vlog.debug("second close: %s (%s)", peerEndpoint.buf, reason);

  if (authenticated()) {
      server->lastDisconnectTime = time(0);
  }

  // Just shutdown the socket and mark our state as closing.  Eventually the
  // calling code will call VNCServerST's removeSocket() method causing us to
  // be deleted.
  sock->shutdown();
  setState(RFBSTATE_CLOSING);
}


void VNCSConnectionST::processMessages()
{
  if (state() == RFBSTATE_CLOSING) return;
  try {
    // - Now set appropriate socket timeouts and process data
    setSocketTimeouts();

    inProcessMessages = true;

    // Get the underlying TCP layer to build large packets if we send
    // multiple small responses.
    network::TcpSocket::cork(sock->getFd(), true);

    while (getInStream()->checkNoWait(1)) {
      if (pendingSyncFence) {
        syncFence = true;
        pendingSyncFence = false;
      }

      processMsg();

      if (syncFence) {
        writer()->writeFence(fenceFlags, fenceDataLen, fenceData);
        syncFence = false;
      }
    }

    // Flush out everything in case we go idle after this.
    network::TcpSocket::cork(sock->getFd(), false);

    inProcessMessages = false;

    // If there were anything requiring an update, try to send it here.
    // We wait until now with this to aggregate responses and to give 
    // higher priority to user actions such as keyboard and pointer events.
    writeFramebufferUpdate();
  } catch (rdr::EndOfStream&) {
    close("Clean disconnection");
  } catch (rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::pixelBufferChange()
{
  try {
    if (!authenticated()) return;
    if (cp.width && cp.height && (server->pb->width() != cp.width ||
                                  server->pb->height() != cp.height))
    {
      // We need to clip the next update to the new size, but also add any
      // extra bits if it's bigger.  If we wanted to do this exactly, something
      // like the code below would do it, but at the moment we just update the
      // entire new size.  However, we do need to clip the renderedCursorRect
      // because that might be added to updates in writeFramebufferUpdate().

      //updates.intersect(server->pb->getRect());
      //
      //if (server->pb->width() > cp.width)
      //  updates.add_changed(Rect(cp.width, 0, server->pb->width(),
      //                           server->pb->height()));
      //if (server->pb->height() > cp.height)
      //  updates.add_changed(Rect(0, cp.height, cp.width,
      //                           server->pb->height()));

      renderedCursorRect = renderedCursorRect.intersect(server->pb->getRect());

      cp.width = server->pb->width();
      cp.height = server->pb->height();
      cp.screenLayout = server->screenLayout;
      if (state() == RFBSTATE_NORMAL) {
        // We should only send EDS to client asking for both
        if (!writer()->writeExtendedDesktopSize()) {
          if (!writer()->writeSetDesktopSize()) {
            close("Client does not support desktop resize");
            return;
          }
        }
      }
    }
    // Just update the whole screen at the moment because we're too lazy to
    // work out what's actually changed.
    updates.clear();
    updates.add_changed(server->pb->getRect());
    writeFramebufferUpdate();
  } catch(rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::writeFramebufferUpdateOrClose()
{
  try {
    writeFramebufferUpdate();
  } catch(rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::screenLayoutChangeOrClose(rdr::U16 reason)
{
  try {
    screenLayoutChange(reason);
  } catch(rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::bellOrClose()
{
  try {
    if (state() == RFBSTATE_NORMAL) writer()->writeBell();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::serverCutTextOrClose(const char *str, int len)
{
  try {
    if (!(accessRights & AccessCutText)) return;
    if (!rfb::Server::sendCutText) return;
    if (state() == RFBSTATE_NORMAL)
      writer()->writeServerCutText(str, len);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}


void VNCSConnectionST::setDesktopNameOrClose(const char *name)
{
  try {
    setDesktopName(name);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}


void VNCSConnectionST::setCursorOrClose()
{
  try {
    setCursor();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}


int VNCSConnectionST::checkIdleTimeout()
{
  int idleTimeout = rfb::Server::idleTimeout;
  if (idleTimeout == 0) return 0;
  if (state() != RFBSTATE_NORMAL && idleTimeout < 15)
    idleTimeout = 15; // minimum of 15 seconds while authenticating
  time_t now = time(0);
  if (now < lastEventTime) {
    // Someone must have set the time backwards.  Set lastEventTime so that the
    // idleTimeout will count from now.
    vlog.info("Time has gone backwards - resetting idle timeout");
    lastEventTime = now;
  }
  int timeLeft = lastEventTime + idleTimeout - now;
  if (timeLeft < -60) {
    // Our callback is over a minute late - someone must have set the time
    // forwards.  Set lastEventTime so that the idleTimeout will count from
    // now.
    vlog.info("Time has gone forwards - resetting idle timeout");
    lastEventTime = now;
    return secsToMillis(idleTimeout);
  }
  if (timeLeft <= 0) {
    close("Idle timeout");
    return 0;
  }
  return secsToMillis(timeLeft);
}


bool VNCSConnectionST::getComparerState()
{
  // We interpret a low compression level as an indication that the client
  // wants to prioritise CPU usage over bandwidth, and hence disable the
  // comparing update tracker.
  return (cp.compressLevel == -1) || (cp.compressLevel > 1);
}


// renderedCursorChange() is called whenever the server-side rendered cursor
// changes shape or position.  It ensures that the next update will clean up
// the old rendered cursor and if necessary draw the new rendered cursor.

void VNCSConnectionST::renderedCursorChange()
{
  if (state() != RFBSTATE_NORMAL) return;
  if (!renderedCursorRect.is_empty())
    removeRenderedCursor = true;
  if (needRenderedCursor()) {
    drawRenderedCursor = true;
    writeFramebufferUpdateOrClose();
  }
}

// needRenderedCursor() returns true if this client needs the server-side
// rendered cursor.  This may be because it does not support local cursor or
// because the current cursor position has not been set by this client.
// Unfortunately we can't know for sure when the current cursor position has
// been set by this client.  We guess that this is the case when the current
// cursor position is the same as the last pointer event from this client, or
// if it is a very short time since this client's last pointer event (up to a
// second).  [ Ideally we should do finer-grained timing here and make the time
// configurable, but I don't think it's that important. ]

bool VNCSConnectionST::needRenderedCursor()
{
  bool pointerpos = (!server->cursorPos.equals(pointerEventPos) && (time(0) - pointerEventTime) > 0);
  return (state() == RFBSTATE_NORMAL
          && ((!cp.supportsLocalCursor && !cp.supportsLocalXCursor) || pointerpos));
}


void VNCSConnectionST::approveConnectionOrClose(bool accept,
                                                const char* reason)
{
  try {
    approveConnection(accept, reason);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}



// -=- Callbacks from SConnection

void VNCSConnectionST::authSuccess()
{
  lastEventTime = time(0);

  server->startDesktop();

  // - Set the connection parameters appropriately
  cp.width = server->pb->width();
  cp.height = server->pb->height();
  cp.screenLayout = server->screenLayout;
  cp.setName(server->getName());
  
  // - Set the default pixel format
  cp.setPF(server->pb->getPF());
  char buffer[256];
  cp.pf().print(buffer, 256);
  vlog.info("Server default pixel format %s", buffer);

  // - Mark the entire display as "dirty"
  updates.add_changed(server->pb->getRect());
  startTime = time(0);

  // - Bootstrap the congestion control
  ackedOffset = sock->outStream().length();
  congWindow = INITIAL_WINDOW;
}

void VNCSConnectionST::queryConnection(const char* userName)
{
  // - Authentication succeeded - clear from blacklist
  CharArray name; name.buf = sock->getPeerAddress();
  server->blHosts->clearBlackmark(name.buf);

  // - Special case to provide a more useful error message
  if (rfb::Server::neverShared && !rfb::Server::disconnectClients &&
    server->authClientCount() > 0) {
    approveConnection(false, "The server is already in use");
    return;
  }

  // - Does the client have the right to bypass the query?
  if (reverseConnection ||
      !(rfb::Server::queryConnect || sock->requiresQuery()) ||
      (accessRights & AccessNoQuery))
  {
    approveConnection(true);
    return;
  }

  // - Get the server to display an Accept/Reject dialog, if required
  //   If a dialog is displayed, the result will be PENDING, and the
  //   server will call approveConnection at a later time
  CharArray reason;
  VNCServerST::queryResult qr = server->queryConnection(sock, userName,
                                                        &reason.buf);
  if (qr == VNCServerST::PENDING) {
    queryConnectTimer.start(rfb::Server::queryConnectTimeout * 1000);
    return;
  }

  // - If server returns ACCEPT/REJECT then pass result to SConnection
  approveConnection(qr == VNCServerST::ACCEPT, reason.buf);
}

void VNCSConnectionST::clientInit(bool shared)
{
  lastEventTime = time(0);
  if (rfb::Server::alwaysShared || reverseConnection) shared = true;
  if (!(accessRights & AccessNonShared)) shared = true;
  if (rfb::Server::neverShared) shared = false;
  if (!shared) {
    if (rfb::Server::disconnectClients && (accessRights & AccessNonShared)) {
      // - Close all the other connected clients
      vlog.debug("non-shared connection - closing clients");
      server->closeClients("Non-shared connection requested", getSock());
    } else {
      // - Refuse this connection if there are existing clients, in addition to
      // this one
      if (server->authClientCount() > 1) {
        close("Server is already in use");
        return;
      }
    }
  }
  SConnection::clientInit(shared);
}

void VNCSConnectionST::setPixelFormat(const PixelFormat& pf)
{
  SConnection::setPixelFormat(pf);
  char buffer[256];
  pf.print(buffer, 256);
  vlog.info("Client pixel format %s", buffer);
  setCursor();
}

void VNCSConnectionST::pointerEvent(const Point& pos, int buttonMask)
{
  pointerEventTime = lastEventTime = time(0);
  server->lastUserInputTime = lastEventTime;
  if (!(accessRights & AccessPtrEvents)) return;
  if (!rfb::Server::acceptPointerEvents) return;
  if (!server->pointerClient || server->pointerClient == this) {
    pointerEventPos = pos;
    if (buttonMask)
      server->pointerClient = this;
    else
      server->pointerClient = 0;
    server->desktop->pointerEvent(pointerEventPos, buttonMask);
  }
}


class VNCSConnectionSTShiftPresser {
public:
  VNCSConnectionSTShiftPresser(SDesktop* desktop_)
    : desktop(desktop_), pressed(false) {}
  ~VNCSConnectionSTShiftPresser() {
    if (pressed) { desktop->keyEvent(XK_Shift_L, false); }
  }
  void press() {
    desktop->keyEvent(XK_Shift_L, true);
    pressed = true;
  }
  SDesktop* desktop;
  bool pressed;
};

// keyEvent() - record in the pressedKeys which keys were pressed.  Allow
// multiple down events (for autorepeat), but only allow a single up event.
void VNCSConnectionST::keyEvent(rdr::U32 key, bool down) {
  lastEventTime = time(0);
  server->lastUserInputTime = lastEventTime;
  if (!(accessRights & AccessKeyEvents)) return;
  if (!rfb::Server::acceptKeyEvents) return;

  // Remap the key if required
  if (server->keyRemapper)
    key = server->keyRemapper->remapKey(key);

  // Turn ISO_Left_Tab into shifted Tab.
  VNCSConnectionSTShiftPresser shiftPresser(server->desktop);
  if (key == XK_ISO_Left_Tab) {
    if (pressedKeys.find(XK_Shift_L) == pressedKeys.end() &&
        pressedKeys.find(XK_Shift_R) == pressedKeys.end())
      shiftPresser.press();
    key = XK_Tab;
  }

  if (down) {
    pressedKeys.insert(key);
  } else {
    if (!pressedKeys.erase(key)) return;
  }
  server->desktop->keyEvent(key, down);
}

void VNCSConnectionST::clientCutText(const char* str, int len)
{
  if (!(accessRights & AccessCutText)) return;
  if (!rfb::Server::acceptCutText) return;
  server->desktop->clientCutText(str, len);
}

void VNCSConnectionST::framebufferUpdateRequest(const Rect& r,bool incremental)
{
  Rect safeRect;

  if (!(accessRights & AccessView)) return;

  SConnection::framebufferUpdateRequest(r, incremental);

  // Check that the client isn't sending crappy requests
  if (!r.enclosed_by(Rect(0, 0, cp.width, cp.height))) {
    vlog.error("FramebufferUpdateRequest %dx%d at %d,%d exceeds framebuffer %dx%d",
               r.width(), r.height(), r.tl.x, r.tl.y, cp.width, cp.height);
    safeRect = r.intersect(Rect(0, 0, cp.width, cp.height));
  } else {
    safeRect = r;
  }

  // Just update the requested region.
  // Framebuffer update will be sent a bit later, see processMessages().
  Region reqRgn(r);
  if (!incremental || !continuousUpdates)
    requested.assign_union(reqRgn);

  if (!incremental) {
    // Non-incremental update - treat as if area requested has changed
    updates.add_changed(reqRgn);
    server->comparer->add_changed(reqRgn);

    // And send the screen layout to the client (which, unlike the
    // framebuffer dimensions, the client doesn't get during init)
    writer()->writeExtendedDesktopSize();

    // We do not send a DesktopSize since it only contains the
    // framebuffer size (which the client already should know) and
    // because some clients don't handle extra DesktopSize events
    // very well.
  }
}

void VNCSConnectionST::setDesktopSize(int fb_width, int fb_height,
                                      const ScreenSet& layout)
{
  unsigned int result;

  if (!(accessRights & AccessSetDesktopSize)) return;
  if (!rfb::Server::acceptSetDesktopSize) return;

  // Don't bother the desktop with an invalid configuration
  if (!layout.validate(fb_width, fb_height)) {
    writer()->writeExtendedDesktopSize(reasonClient, resultInvalid,
                                       fb_width, fb_height, layout);
    writeFramebufferUpdate();
    return;
  }

  // FIXME: the desktop will call back to VNCServerST and an extra set
  // of ExtendedDesktopSize messages will be sent. This is okay
  // protocol-wise, but unnecessary.
  result = server->desktop->setScreenLayout(fb_width, fb_height, layout);

  writer()->writeExtendedDesktopSize(reasonClient, result,
                                     fb_width, fb_height, layout);

  // Only notify other clients on success
  if (result == resultSuccess) {
    if (server->screenLayout != layout)
        throw Exception("Desktop configured a different screen layout than requested");
    server->notifyScreenLayoutChange(this);
  }

  // but always send back a reply to the requesting client
  // (do this last as it might throw an exception on socket errors)
  writeFramebufferUpdate();
}

void VNCSConnectionST::fence(rdr::U32 flags, unsigned len, const char data[])
{
  if (flags & fenceFlagRequest) {
    if (flags & fenceFlagSyncNext) {
      pendingSyncFence = true;

      fenceFlags = flags & (fenceFlagBlockBefore | fenceFlagBlockAfter | fenceFlagSyncNext);
      fenceDataLen = len;
      delete [] fenceData;
      if (len > 0) {
        fenceData = new char[len];
        memcpy(fenceData, data, len);
      }

      return;
    }

    // We handle everything synchronously so we trivially honor these modes
    flags = flags & (fenceFlagBlockBefore | fenceFlagBlockAfter);

    writer()->writeFence(flags, len, data);
    return;
  }

  struct RTTInfo rttInfo;

  switch (len) {
  case 0:
    // Initial dummy fence;
    break;
  case sizeof(struct RTTInfo):
    memcpy(&rttInfo, data, sizeof(struct RTTInfo));
    handleRTTPong(rttInfo);
    break;
  default:
    vlog.error("Fence response of unexpected size received");
  }
}

void VNCSConnectionST::enableContinuousUpdates(bool enable,
                                               int x, int y, int w, int h)
{
  Rect rect;

  if (!cp.supportsFence || !cp.supportsContinuousUpdates)
    throw Exception("Client tried to enable continuous updates when not allowed");

  continuousUpdates = enable;

  rect.setXYWH(x, y, w, h);
  cuRegion.reset(rect);

  if (enable) {
    requested.clear();
    writeFramebufferUpdate();
  } else {
    writer()->writeEndOfContinuousUpdates();
  }
}

// supportsLocalCursor() is called whenever the status of
// cp.supportsLocalCursor has changed.  If the client does now support local
// cursor, we make sure that the old server-side rendered cursor is cleaned up
// and the cursor is sent to the client.

void VNCSConnectionST::supportsLocalCursor()
{
  if (cp.supportsLocalCursor || cp.supportsLocalXCursor) {
    if (!renderedCursorRect.is_empty())
      removeRenderedCursor = true;
    drawRenderedCursor = false;
    setCursor();
  }
}

void VNCSConnectionST::supportsFence()
{
  writer()->writeFence(fenceFlagRequest, 0, NULL);
}

void VNCSConnectionST::supportsContinuousUpdates()
{
  // We refuse to use continuous updates if we cannot monitor the buffer
  // usage using fences.
  if (!cp.supportsFence)
    return;

  writer()->writeEndOfContinuousUpdates();
}


bool VNCSConnectionST::handleTimeout(Timer* t)
{
  try {
    if (t == &updateTimer)
      writeFramebufferUpdate();
    else if (t == &congestionTimer)
      updateCongestion();
    else if (t == &queryConnectTimer) {
      if (state() == RFBSTATE_QUERYING)
        approveConnection(false, "The attempt to prompt the user to accept the connection failed");
    }
  } catch (rdr::Exception& e) {
    close(e.str());
  }

  return false;
}


void VNCSConnectionST::writeRTTPing()
{
  struct RTTInfo rttInfo;

  if (!cp.supportsFence)
    return;

  memset(&rttInfo, 0, sizeof(struct RTTInfo));

  gettimeofday(&rttInfo.tv, NULL);
  rttInfo.offset = sock->outStream().length();
  rttInfo.inFlight = rttInfo.offset - ackedOffset;

  // We need to make sure any old update are already processed by the
  // time we get the response back. This allows us to reliably throttle
  // back on client overload, as well as network overload.
  writer()->writeFence(fenceFlagRequest | fenceFlagBlockBefore,
                       sizeof(struct RTTInfo), (const char*)&rttInfo);

  pingCounter++;

  sentOffset = rttInfo.offset;

  // Let some data flow before we adjust the settings
  if (!congestionTimer.isStarted())
    congestionTimer.start(__rfbmin(baseRTT * 2, 100));
}

void VNCSConnectionST::handleRTTPong(const struct RTTInfo &rttInfo)
{
  unsigned rtt, delay;
  int bdp;

  pingCounter--;

  rtt = msSince(&rttInfo.tv);
  if (rtt < 1)
    rtt = 1;

  ackedOffset = rttInfo.offset;

  // Try to estimate wire latency by tracking lowest seen latency
  if (rtt < baseRTT)
    baseRTT = rtt;

  if (rttInfo.inFlight > congWindow) {
    seenCongestion = true;

    // Estimate added delay because of overtaxed buffers
    delay = (rttInfo.inFlight - congWindow) * baseRTT / congWindow;

    if (delay < rtt)
      rtt -= delay;
    else
      rtt = 1;

    // If we underestimate the congestion window, then we'll get a latency
    // that's less than the wire latency, which will confuse other portions
    // of the code.
    if (rtt < baseRTT)
      rtt = baseRTT;
  }

  // We only keep track of the minimum latency seen (for a given interval)
  // on the basis that we want to avoid continous buffer issue, but don't
  // mind (or even approve of) bursts.
  if (rtt < minRTT)
    minRTT = rtt;
}

bool VNCSConnectionST::isCongested()
{
  int offset;

  // Stuff still waiting in the send buffer?
  if (sock->outStream().bufferUsage() > 0)
    return true;

  if (!cp.supportsFence)
    return false;

  // Idle for too long? (and no data on the wire)
  //
  // FIXME: This should really just be one baseRTT, but we're getting
  //        problems with triggering the idle timeout on each update.
  //        Maybe we need to use a moving average for the wire latency
  //        instead of baseRTT.
  if ((sentOffset == ackedOffset) &&
      (sock->outStream().getIdleTime() > 2 * baseRTT)) {

#ifdef CONGESTION_DEBUG
    if (congWindow > INITIAL_WINDOW)
      fprintf(stderr, "Reverting to initial window (%d KiB) after %d ms\n",
              INITIAL_WINDOW / 1024, sock->outStream().getIdleTime());
#endif

    // Close congestion window and allow a transfer
    // FIXME: Reset baseRTT like Linux Vegas?
    congWindow = __rfbmin(INITIAL_WINDOW, congWindow);

    return false;
  }

  offset = sock->outStream().length();

  // FIXME: Should we compensate for non-update data?
  //        (i.e. use sentOffset instead of offset)
  if ((offset - ackedOffset) < congWindow)
    return false;

  // If we just have one outstanding "ping", that means the client has
  // started receiving our update. In order to not regress compared to
  // before we had congestion avoidance, we allow another update here.
  // This could further clog up the tubes, but congestion control isn't
  // really working properly right now anyway as the wire would otherwise
  // be idle for at least RTT/2.
  if (pingCounter == 1)
    return false;

  return true;
}


void VNCSConnectionST::updateCongestion()
{
  unsigned diff;

  if (!seenCongestion)
    return;

  diff = minRTT - baseRTT;

  if (diff > __rfbmin(100, baseRTT)) {
    // Way too fast
    congWindow = congWindow * baseRTT / minRTT;
  } else if (diff > __rfbmin(50, baseRTT/2)) {
    // Slightly too fast
    congWindow -= 4096;
  } else if (diff < 5) {
    // Way too slow
    congWindow += 8192;
  } else if (diff < 25) {
    // Too slow
    congWindow += 4096;
  }

  if (congWindow < MINIMUM_WINDOW)
    congWindow = MINIMUM_WINDOW;
  if (congWindow > MAXIMUM_WINDOW)
    congWindow = MAXIMUM_WINDOW;

#ifdef CONGESTION_DEBUG
  fprintf(stderr, "RTT: %d ms (%d ms), Window: %d KiB, Bandwidth: %g Mbps\n",
          minRTT, baseRTT, congWindow / 1024,
          congWindow * 8.0 / baseRTT / 1000.0);

#ifdef TCP_INFO
  struct tcp_info tcp_info;
  socklen_t tcp_info_length;

  tcp_info_length = sizeof(tcp_info);
  if (getsockopt(sock->getFd(), SOL_TCP, TCP_INFO,
                 (void *)&tcp_info, &tcp_info_length) == 0) {
    fprintf(stderr, "Socket: RTT: %d ms (+/- %d ms) Window %d KiB\n",
            tcp_info.tcpi_rtt / 1000, tcp_info.tcpi_rttvar / 1000,
            tcp_info.tcpi_snd_mss * tcp_info.tcpi_snd_cwnd / 1024);
  }
#endif

#endif

  minRTT = -1;
  seenCongestion = false;
}


void VNCSConnectionST::writeFramebufferUpdate()
{
  Region req;
  UpdateInfo ui;
  bool needNewUpdateInfo;

  updateTimer.stop();

  // We're in the middle of processing a command that's supposed to be
  // synchronised. Allowing an update to slip out right now might violate
  // that synchronisation.
  if (syncFence)
    return;

  // We try to aggregate responses, so don't send out anything whilst we
  // still have incoming messages. processMessages() will give us another
  // chance to run once things are idle.
  if (inProcessMessages)
    return;

  if (state() != RFBSTATE_NORMAL)
    return;
  if (requested.is_empty() && !continuousUpdates)
    return;

  // Check that we actually have some space on the link and retry in a
  // bit if things are congested.
  if (isCongested()) {
    updateTimer.start(50);
    return;
  }

  // In continuous mode, we will be outputting at least three distinct
  // messages. We need to aggregate these in order to not clog up TCP's
  // congestion window.
  network::TcpSocket::cork(sock->getFd(), true);

  // First take care of any updates that cannot contain framebuffer data
  // changes.
  if (writer()->needNoDataUpdate()) {
    writer()->writeNoDataUpdate();
    requested.clear();
    if (!continuousUpdates)
      goto out;
  }

  updates.enable_copyrect(cp.useCopyRect);

  // Fetch updates from server object, and see if we are allowed to send
  // anything right now (the framebuffer might have changed in ways we
  // haven't yet been informed of).
  if (!server->checkUpdate())
    goto out;

  // Get the lists of updates. Prior to exporting the data to the `ui' object,
  // getUpdateInfo() will normalize the `updates' object such way that its
  // `changed' and `copied' regions would not intersect.

  if (continuousUpdates)
    req = cuRegion.union_(requested);
  else
    req = requested;

  updates.getUpdateInfo(&ui, req);
  needNewUpdateInfo = false;

  // If the previous position of the rendered cursor overlaps the source of the
  // copy, then when the copy happens the corresponding rectangle in the
  // destination will be wrong, so add it to the changed region.

  if (!ui.copied.is_empty() && !renderedCursorRect.is_empty()) {
    Rect bogusCopiedCursor = (renderedCursorRect.translate(ui.copy_delta)
                              .intersect(server->pb->getRect()));
    if (!ui.copied.intersect(bogusCopiedCursor).is_empty()) {
      updates.add_changed(bogusCopiedCursor);
      needNewUpdateInfo = true;
    }
  }

  // If we need to remove the old rendered cursor, just add the rectangle to
  // the changed region.

  if (removeRenderedCursor) {
    updates.add_changed(renderedCursorRect);
    needNewUpdateInfo = true;
    renderedCursorRect.clear();
    removeRenderedCursor = false;
  }

  // Return if there is nothing to send the client.

  if (updates.is_empty() && !writer()->needFakeUpdate() && !drawRenderedCursor)
    goto out;

  // The `updates' object could change, make sure we have valid update info.

  if (needNewUpdateInfo)
    updates.getUpdateInfo(&ui, req);

  // If the client needs a server-side rendered cursor, work out the cursor
  // rectangle.  If it's empty then don't bother drawing it, but if it overlaps
  // with the update region, we need to draw the rendered cursor regardless of
  // whether it has changed.

  if (needRenderedCursor()) {
    renderedCursorRect
      = server->renderedCursor.getEffectiveRect()
         .intersect(req.get_bounding_rect());

    if (renderedCursorRect.is_empty()) {
      drawRenderedCursor = false;
    } else if (!ui.changed.union_(ui.copied)
               .intersect(renderedCursorRect).is_empty()) {
      drawRenderedCursor = true;
    }

    // We could remove the new cursor rect from updates here.  It's not clear
    // whether this is worth it.  If we do remove it, then we won't draw over
    // the same bit of screen twice, but we have the overhead of a more complex
    // region.

    //if (drawRenderedCursor) {
    //  updates.subtract(renderedCursorRect);
    //  updates.getUpdateInfo(&ui, req);
    //}
  }

  if (!ui.is_empty() || writer()->needFakeUpdate() || drawRenderedCursor) {
    RenderedCursor *cursor;

    cursor = NULL;
    if (drawRenderedCursor)
      cursor = &server->renderedCursor;

    writeRTTPing();

    encodeManager.writeUpdate(ui, server->getPixelBuffer(), cursor);

    writeRTTPing();

    drawRenderedCursor = false;
    requested.clear();
    updates.clear();
  }

out:
  network::TcpSocket::cork(sock->getFd(), false);
}


void VNCSConnectionST::screenLayoutChange(rdr::U16 reason)
{
  if (!authenticated())
    return;

  cp.screenLayout = server->screenLayout;

  if (state() != RFBSTATE_NORMAL)
    return;

  writer()->writeExtendedDesktopSize(reason, 0, cp.width, cp.height,
                                     cp.screenLayout);
  writeFramebufferUpdate();
}


// setCursor() is called whenever the cursor has changed shape or pixel format.
// If the client supports local cursor then it will arrange for the cursor to
// be sent to the client.

void VNCSConnectionST::setCursor()
{
  if (state() != RFBSTATE_NORMAL)
    return;

  cp.setCursor(server->cursor);

  if (!writer()->writeSetCursor()) {
    if (!writer()->writeSetXCursor()) {
      // No client support
      return;
    }
  }

  writeFramebufferUpdate();
}

void VNCSConnectionST::setDesktopName(const char *name)
{
  cp.setName(name);

  if (state() != RFBSTATE_NORMAL)
    return;

  if (!writer()->writeSetDesktopName()) {
    fprintf(stderr, "Client does not support desktop rename\n");
    return;
  }

  writeFramebufferUpdate();
}

void VNCSConnectionST::setSocketTimeouts()
{
  int timeoutms = rfb::Server::clientWaitTimeMillis;
  soonestTimeout(&timeoutms, secsToMillis(rfb::Server::idleTimeout));
  if (timeoutms == 0)
    timeoutms = -1;
  sock->inStream().setTimeout(timeoutms);
  sock->outStream().setTimeout(timeoutms);
}

char* VNCSConnectionST::getStartTime()
{
  char* result = ctime(&startTime);
  result[24] = '\0';
  return result; 
}

void VNCSConnectionST::setStatus(int status)
{
  switch (status) {
  case 0:
    accessRights = accessRights | AccessPtrEvents | AccessKeyEvents | AccessView;
    break;
  case 1:
    accessRights = accessRights & ~(AccessPtrEvents | AccessKeyEvents) | AccessView;
    break;
  case 2:
    accessRights = accessRights & ~(AccessPtrEvents | AccessKeyEvents | AccessView);
    break;
  }
  framebufferUpdateRequest(server->pb->getRect(), false);
}
int VNCSConnectionST::getStatus()
{
  if ((accessRights & (AccessPtrEvents | AccessKeyEvents | AccessView)) == 0x0007)
    return 0;
  if ((accessRights & (AccessPtrEvents | AccessKeyEvents | AccessView)) == 0x0001)
    return 1;
  if ((accessRights & (AccessPtrEvents | AccessKeyEvents | AccessView)) == 0x0000)
    return 2;
  return 4;
}

