/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
 * Copyright 2018 Peter Astrand for Cendio AB
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

#include <network/TcpSocket.h>

#include <rfb/ComparingUpdateTracker.h>
#include <rfb/Encoder.h>
#include <rfb/KeyRemapper.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/ServerCore.h>
#include <rfb/SMsgWriter.h>
#include <rfb/VNCServerST.h>
#include <rfb/VNCSConnectionST.h>
#include <rfb/screenTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/ledStates.h>
#define XK_LATIN1
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>

using namespace rfb;

static LogWriter vlog("VNCSConnST");

static Cursor emptyCursor(0, 0, Point(0, 0), NULL);

VNCSConnectionST::VNCSConnectionST(VNCServerST* server_, network::Socket *s,
                                   bool reverse)
  : sock(s), reverseConnection(reverse),
    inProcessMessages(false),
    pendingSyncFence(false), syncFence(false), fenceFlags(0),
    fenceDataLen(0), fenceData(NULL), congestionTimer(this),
    losslessTimer(this), server(server_),
    updateRenderedCursor(false), removeRenderedCursor(false),
    continuousUpdates(false), encodeManager(this), idleTimer(this),
    pointerEventTime(0), clientHasCursor(false)
{
  setStreams(&sock->inStream(), &sock->outStream());
  peerEndpoint.buf = sock->getPeerEndpoint();

  // Kick off the idle timer
  if (rfb::Server::idleTimeout) {
    // minimum of 15 seconds while authenticating
    if (rfb::Server::idleTimeout < 15)
      idleTimer.start(secsToMillis(15));
    else
      idleTimer.start(secsToMillis(rfb::Server::idleTimeout));
  }
}


VNCSConnectionST::~VNCSConnectionST()
{
  // If we reach here then VNCServerST is deleting us!
  if (closeReason.buf)
    vlog.info("closing %s: %s", peerEndpoint.buf, closeReason.buf);

  // Release any keys the client still had pressed
  while (!pressedKeys.empty()) {
    rdr::U32 keysym, keycode;

    keysym = pressedKeys.begin()->second;
    keycode = pressedKeys.begin()->first;
    pressedKeys.erase(pressedKeys.begin());

    vlog.debug("Releasing key 0x%x / 0x%x on client disconnect",
               keysym, keycode);
    server->keyEvent(keysym, keycode, false);
  }

  delete [] fenceData;
}


// SConnection methods

bool VNCSConnectionST::accessCheck(AccessRights ar) const
{
  // Reverse connections are user initiated, so they are implicitly
  // allowed to bypass the query
  if (reverseConnection)
    ar &= ~AccessNoQuery;

  return SConnection::accessCheck(ar);
}

void VNCSConnectionST::close(const char* reason)
{
  SConnection::close(reason);

  // Log the reason for the close
  if (!closeReason.buf)
    closeReason.buf = strDup(reason);
  else
    vlog.debug("second close: %s (%s)", peerEndpoint.buf, reason);

  try {
    if (sock->outStream().hasBufferedData()) {
      sock->outStream().cork(false);
      sock->outStream().flush();
      if (sock->outStream().hasBufferedData())
        vlog.error("Failed to flush remaining socket data on close");
    }
  } catch (rdr::Exception& e) {
    vlog.error("Failed to flush remaining socket data on close: %s", e.str());
  }

  // Just shutdown the socket and mark our state as closing.  Eventually the
  // calling code will call VNCServerST's removeSocket() method causing us to
  // be deleted.
  sock->shutdown();
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


void VNCSConnectionST::processMessages()
{
  if (state() == RFBSTATE_CLOSING) return;
  try {
    inProcessMessages = true;

    // Get the underlying transport to build large packets if we send
    // multiple small responses.
    getOutStream()->cork(true);

    while (true) {
      if (pendingSyncFence)
        syncFence = true;

      if (!processMsg())
        break;

      if (syncFence) {
        writer()->writeFence(fenceFlags, fenceDataLen, fenceData);
        syncFence = false;
        pendingSyncFence = false;
      }
    }

    // Flush out everything in case we go idle after this.
    getOutStream()->cork(false);

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

void VNCSConnectionST::flushSocket()
{
  if (state() == RFBSTATE_CLOSING) return;
  try {
    sock->outStream().flush();
    // Flushing the socket might release an update that was previously
    // delayed because of congestion.
    if (!sock->outStream().hasBufferedData())
      writeFramebufferUpdate();
  } catch (rdr::Exception &e) {
    close(e.str());
  }
}

void VNCSConnectionST::pixelBufferChange()
{
  try {
    if (!authenticated()) return;
    if (client.width() && client.height() &&
        (server->getPixelBuffer()->width() != client.width() ||
         server->getPixelBuffer()->height() != client.height()))
    {
      // We need to clip the next update to the new size, but also add any
      // extra bits if it's bigger.  If we wanted to do this exactly, something
      // like the code below would do it, but at the moment we just update the
      // entire new size.  However, we do need to clip the damagedCursorRegion
      // because that might be added to updates in writeFramebufferUpdate().

      //updates.intersect(server->pb->getRect());
      //
      //if (server->pb->width() > client.width())
      //  updates.add_changed(Rect(client.width(), 0, server->pb->width(),
      //                           server->pb->height()));
      //if (server->pb->height() > client.height())
      //  updates.add_changed(Rect(0, client.height(), client.width(),
      //                           server->pb->height()));

      damagedCursorRegion.assign_intersect(server->getPixelBuffer()->getRect());

      client.setDimensions(server->getPixelBuffer()->width(),
                           server->getPixelBuffer()->height(),
                           server->getScreenLayout());
      if (state() == RFBSTATE_NORMAL) {
        if (!client.supportsDesktopSize()) {
          close("Client does not support desktop resize");
          return;
        }
        writer()->writeDesktopSize(reasonServer);
      }

      // Drop any lossy tracking that is now outside the framebuffer
      encodeManager.pruneLosslessRefresh(Region(server->getPixelBuffer()->getRect()));
    }
    // Just update the whole screen at the moment because we're too lazy to
    // work out what's actually changed.
    updates.clear();
    updates.add_changed(server->getPixelBuffer()->getRect());
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
    writeFramebufferUpdate();
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

void VNCSConnectionST::setDesktopNameOrClose(const char *name)
{
  try {
    setDesktopName(name);
    writeFramebufferUpdate();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::setCursorOrClose()
{
  try {
    setCursor();
    writeFramebufferUpdate();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::setLEDStateOrClose(unsigned int state)
{
  try {
    setLEDState(state);
    writeFramebufferUpdate();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::requestClipboardOrClose()
{
  try {
    if (state() != RFBSTATE_NORMAL) return;
    if (!accessCheck(AccessCutText)) return;
    if (!rfb::Server::acceptCutText) return;
    requestClipboard();
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::announceClipboardOrClose(bool available)
{
  try {
    if (state() != RFBSTATE_NORMAL) return;
    if (!accessCheck(AccessCutText)) return;
    if (!rfb::Server::sendCutText) return;
    announceClipboard(available);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

void VNCSConnectionST::sendClipboardDataOrClose(const char* data)
{
  try {
    if (state() != RFBSTATE_NORMAL) return;
    if (!accessCheck(AccessCutText)) return;
    if (!rfb::Server::sendCutText) return;
    sendClipboardData(data);
  } catch(rdr::Exception& e) {
    close(e.str());
  }
}

bool VNCSConnectionST::getComparerState()
{
  // We interpret a low compression level as an indication that the client
  // wants to prioritise CPU usage over bandwidth, and hence disable the
  // comparing update tracker.
  return (client.compressLevel == -1) || (client.compressLevel > 1);
}


// renderedCursorChange() is called whenever the server-side rendered cursor
// changes shape or position.  It ensures that the next update will clean up
// the old rendered cursor and if necessary draw the new rendered cursor.

void VNCSConnectionST::renderedCursorChange()
{
  if (state() != RFBSTATE_NORMAL) return;
  // Are we switching between client-side and server-side cursor?
  if (clientHasCursor == needRenderedCursor())
    setCursorOrClose();
  bool hasRenderedCursor = !damagedCursorRegion.is_empty();
  if (hasRenderedCursor)
    removeRenderedCursor = true;
  if (needRenderedCursor()) {
    updateRenderedCursor = true;
    writeFramebufferUpdateOrClose();
  }
}

// cursorPositionChange() is called whenever the cursor has changed position by
// the server.  If the client supports being informed about these changes then
// it will arrange for the new cursor position to be sent to the client.

void VNCSConnectionST::cursorPositionChange()
{
  setCursorPos();
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
  if (state() != RFBSTATE_NORMAL)
    return false;

  if (!client.supportsLocalCursor())
    return true;
  if (!server->getCursorPos().equals(pointerEventPos) &&
      (time(0) - pointerEventTime) > 0)
    return true;

  return false;
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
  if (rfb::Server::idleTimeout)
    idleTimer.start(secsToMillis(rfb::Server::idleTimeout));

  // - Set the connection parameters appropriately
  client.setDimensions(server->getPixelBuffer()->width(),
                       server->getPixelBuffer()->height(),
                       server->getScreenLayout());
  client.setName(server->getName());
  client.setLEDState(server->getLEDState());
  
  // - Set the default pixel format
  client.setPF(server->getPixelBuffer()->getPF());
  char buffer[256];
  client.pf().print(buffer, 256);
  vlog.info("Server default pixel format %s", buffer);

  // - Mark the entire display as "dirty"
  updates.add_changed(server->getPixelBuffer()->getRect());
}

void VNCSConnectionST::queryConnection(const char* userName)
{
  server->queryConnection(this, userName);
}

void VNCSConnectionST::clientInit(bool shared)
{
  if (rfb::Server::idleTimeout)
    idleTimer.start(secsToMillis(rfb::Server::idleTimeout));
  if (rfb::Server::alwaysShared || reverseConnection) shared = true;
  if (!accessCheck(AccessNonShared)) shared = true;
  if (rfb::Server::neverShared) shared = false;
  SConnection::clientInit(shared);
  server->clientReady(this, shared);
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
  if (rfb::Server::idleTimeout)
    idleTimer.start(secsToMillis(rfb::Server::idleTimeout));
  pointerEventTime = time(0);
  if (!accessCheck(AccessPtrEvents)) return;
  if (!rfb::Server::acceptPointerEvents) return;
  pointerEventPos = pos;
  server->pointerEvent(this, pointerEventPos, buttonMask);
}


class VNCSConnectionSTShiftPresser {
public:
  VNCSConnectionSTShiftPresser(VNCServerST* server_)
    : server(server_), pressed(false) {}
  ~VNCSConnectionSTShiftPresser() {
    if (pressed) {
      vlog.debug("Releasing fake Shift_L");
      server->keyEvent(XK_Shift_L, 0, false);
    }
  }
  void press() {
    vlog.debug("Pressing fake Shift_L");
    server->keyEvent(XK_Shift_L, 0, true);
    pressed = true;
  }
  VNCServerST* server;
  bool pressed;
};

// keyEvent() - record in the pressedKeys which keys were pressed.  Allow
// multiple down events (for autorepeat), but only allow a single up event.
void VNCSConnectionST::keyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down) {
  rdr::U32 lookup;

  if (rfb::Server::idleTimeout)
    idleTimer.start(secsToMillis(rfb::Server::idleTimeout));
  if (!accessCheck(AccessKeyEvents)) return;
  if (!rfb::Server::acceptKeyEvents) return;

  if (down)
    vlog.debug("Key pressed: 0x%x / 0x%x", keysym, keycode);
  else
    vlog.debug("Key released: 0x%x / 0x%x", keysym, keycode);

  // Avoid lock keys if we don't know the server state
  if ((server->getLEDState() == ledUnknown) &&
      ((keysym == XK_Caps_Lock) ||
       (keysym == XK_Num_Lock))) {
    vlog.debug("Ignoring lock key (e.g. caps lock)");
    return;
  }

  // Lock key heuristics
  // (only for clients that do not support the LED state extension)
  if (!client.supportsLEDState()) {
    if (down && (server->getLEDState() != ledUnknown)) {
      // CapsLock synchronisation heuristic
      // (this assumes standard interaction between CapsLock the Shift
      // keys and normal characters)
      if (((keysym >= XK_A) && (keysym <= XK_Z)) ||
          ((keysym >= XK_a) && (keysym <= XK_z))) {
        bool uppercase, shift, lock;

        uppercase = (keysym >= XK_A) && (keysym <= XK_Z);
        shift = isShiftPressed();
        lock = server->getLEDState() & ledCapsLock;

        if (lock == (uppercase == shift)) {
          vlog.debug("Inserting fake CapsLock to get in sync with client");
          server->keyEvent(XK_Caps_Lock, 0, true);
          server->keyEvent(XK_Caps_Lock, 0, false);
        }
      }

      // NumLock synchronisation heuristic
      // (this is more cautious because of the differences between Unix,
      // Windows and macOS)
      if (((keysym >= XK_KP_Home) && (keysym <= XK_KP_Delete)) ||
          ((keysym >= XK_KP_0) && (keysym <= XK_KP_9)) ||
          (keysym == XK_KP_Separator) || (keysym == XK_KP_Decimal)) {
        bool number, shift, lock;

        number = ((keysym >= XK_KP_0) && (keysym <= XK_KP_9)) ||
                  (keysym == XK_KP_Separator) || (keysym == XK_KP_Decimal);
        shift = isShiftPressed();
        lock = server->getLEDState() & ledNumLock;

        if (shift) {
          // We don't know the appropriate NumLock state for when Shift
          // is pressed as it could be one of:
          //
          // a) A Unix client where Shift negates NumLock
          //
          // b) A Windows client where Shift only cancels NumLock
          //
          // c) A macOS client where Shift doesn't have any effect
          //
        } else if (lock == (number == shift)) {
          vlog.debug("Inserting fake NumLock to get in sync with client");
          server->keyEvent(XK_Num_Lock, 0, true);
          server->keyEvent(XK_Num_Lock, 0, false);
        }
      }
    }
  }

  // Turn ISO_Left_Tab into shifted Tab.
  VNCSConnectionSTShiftPresser shiftPresser(server);
  if (keysym == XK_ISO_Left_Tab) {
    if (!isShiftPressed())
      shiftPresser.press();
    keysym = XK_Tab;
  }

  // We need to be able to track keys, so generate a fake index when we
  // aren't given a keycode
  if (keycode == 0)
    lookup = 0x80000000 | keysym;
  else
    lookup = keycode;

  // We force the same keysym for an already down key for the
  // sake of sanity
  if (pressedKeys.find(lookup) != pressedKeys.end())
    keysym = pressedKeys[lookup];

  if (down) {
    pressedKeys[lookup] = keysym;
  } else {
    if (!pressedKeys.erase(lookup))
      return;
  }

  server->keyEvent(keysym, keycode, down);
}

void VNCSConnectionST::framebufferUpdateRequest(const Rect& r,bool incremental)
{
  Rect safeRect;

  if (!accessCheck(AccessView)) return;

  SConnection::framebufferUpdateRequest(r, incremental);

  // Check that the client isn't sending crappy requests
  if (!r.enclosed_by(Rect(0, 0, client.width(), client.height()))) {
    vlog.error("FramebufferUpdateRequest %dx%d at %d,%d exceeds framebuffer %dx%d",
               r.width(), r.height(), r.tl.x, r.tl.y,
               client.width(), client.height());
    safeRect = r.intersect(Rect(0, 0, client.width(), client.height()));
  } else {
    safeRect = r;
  }

  // Just update the requested region.
  // Framebuffer update will be sent a bit later, see processMessages().
  Region reqRgn(safeRect);
  if (!incremental || !continuousUpdates)
    requested.assign_union(reqRgn);

  if (!incremental) {
    // Non-incremental update - treat as if area requested has changed
    updates.add_changed(reqRgn);

    // And send the screen layout to the client (which, unlike the
    // framebuffer dimensions, the client doesn't get during init)
    if (client.supportsEncoding(pseudoEncodingExtendedDesktopSize))
      writer()->writeDesktopSize(reasonServer);

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
  char buffer[2048];

  vlog.debug("Got request for framebuffer resize to %dx%d",
             fb_width, fb_height);
  layout.print(buffer, sizeof(buffer));
  vlog.debug("%s", buffer);

  if (!accessCheck(AccessSetDesktopSize) ||
      !rfb::Server::acceptSetDesktopSize) {
    vlog.debug("Rejecting unauthorized framebuffer resize request");
    result = resultProhibited;
  } else {
    result = server->setDesktopSize(this, fb_width, fb_height, layout);
  }

  writer()->writeDesktopSize(reasonClient, result);
}

void VNCSConnectionST::fence(rdr::U32 flags, unsigned len, const char data[])
{
  rdr::U8 type;

  if (flags & fenceFlagRequest) {
    if (flags & fenceFlagSyncNext) {
      pendingSyncFence = true;

      fenceFlags = flags & (fenceFlagBlockBefore | fenceFlagBlockAfter | fenceFlagSyncNext);
      fenceDataLen = len;
      delete [] fenceData;
      fenceData = NULL;
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

  if (len < 1)
    vlog.error("Fence response of unexpected size received");

  type = data[0];

  switch (type) {
  case 0:
    // Initial dummy fence;
    break;
  case 1:
    congestion.gotPong();
    break;
  default:
    vlog.error("Fence response of unexpected type received");
  }
}

void VNCSConnectionST::enableContinuousUpdates(bool enable,
                                               int x, int y, int w, int h)
{
  Rect rect;

  if (!client.supportsFence() || !client.supportsContinuousUpdates())
    throw Exception("Client tried to enable continuous updates when not allowed");

  continuousUpdates = enable;

  rect.setXYWH(x, y, w, h);
  cuRegion.reset(rect);

  if (enable) {
    requested.clear();
  } else {
    writer()->writeEndOfContinuousUpdates();
  }
}

void VNCSConnectionST::handleClipboardRequest()
{
  if (!accessCheck(AccessCutText)) return;
  server->handleClipboardRequest(this);
}

void VNCSConnectionST::handleClipboardAnnounce(bool available)
{
  if (!accessCheck(AccessCutText)) return;
  if (!rfb::Server::acceptCutText) return;
  server->handleClipboardAnnounce(this, available);
}

void VNCSConnectionST::handleClipboardData(const char* data)
{
  if (!accessCheck(AccessCutText)) return;
  if (!rfb::Server::acceptCutText) return;
  server->handleClipboardData(this, data);
}

// supportsLocalCursor() is called whenever the status of
// client.supportsLocalCursor() has changed.  If the client does now support local
// cursor, we make sure that the old server-side rendered cursor is cleaned up
// and the cursor is sent to the client.

void VNCSConnectionST::supportsLocalCursor()
{
  bool hasRenderedCursor = !damagedCursorRegion.is_empty();
  if (hasRenderedCursor && !needRenderedCursor())
    removeRenderedCursor = true;
  setCursor();
}

void VNCSConnectionST::supportsFence()
{
  char type = 0;
  writer()->writeFence(fenceFlagRequest, sizeof(type), &type);
}

void VNCSConnectionST::supportsContinuousUpdates()
{
  // We refuse to use continuous updates if we cannot monitor the buffer
  // usage using fences.
  if (!client.supportsFence())
    return;

  writer()->writeEndOfContinuousUpdates();
}

void VNCSConnectionST::supportsLEDState()
{
  if (client.ledState() == ledUnknown)
    return;

  writer()->writeLEDState();
}

bool VNCSConnectionST::handleTimeout(Timer* t)
{
  try {
    if ((t == &congestionTimer) ||
        (t == &losslessTimer))
      writeFramebufferUpdate();
  } catch (rdr::Exception& e) {
    close(e.str());
  }

  if (t == &idleTimer)
    close("Idle timeout");

  return false;
}

bool VNCSConnectionST::isShiftPressed()
{
    std::map<rdr::U32, rdr::U32>::const_iterator iter;

    for (iter = pressedKeys.begin(); iter != pressedKeys.end(); ++iter) {
      if (iter->second == XK_Shift_L)
        return true;
      if (iter->second == XK_Shift_R)
        return true;
    }

  return false;
}

void VNCSConnectionST::writeRTTPing()
{
  char type;

  if (!client.supportsFence())
    return;

  congestion.updatePosition(sock->outStream().length());

  // We need to make sure any old update are already processed by the
  // time we get the response back. This allows us to reliably throttle
  // back on client overload, as well as network overload.
  type = 1;
  writer()->writeFence(fenceFlagRequest | fenceFlagBlockBefore,
                       sizeof(type), &type);

  congestion.sentPing();
}

bool VNCSConnectionST::isCongested()
{
  int eta;

  congestionTimer.stop();

  // Stuff still waiting in the send buffer?
  sock->outStream().flush();
  congestion.debugTrace("congestion-trace.csv", sock->getFd());
  if (sock->outStream().hasBufferedData())
    return true;

  if (!client.supportsFence())
    return false;

  congestion.updatePosition(sock->outStream().length());
  if (!congestion.isCongested())
    return false;

  eta = congestion.getUncongestedETA();
  if (eta >= 0)
    congestionTimer.start(eta);

  return true;
}


void VNCSConnectionST::writeFramebufferUpdate()
{
  congestion.updatePosition(sock->outStream().length());

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
  if (isCongested())
    return;

  // Updates often consists of many small writes, and in continuous
  // mode, we will also have small fence messages around the update. We
  // need to aggregate these in order to not clog up TCP's congestion
  // window.
  getOutStream()->cork(true);

  // First take care of any updates that cannot contain framebuffer data
  // changes.
  writeNoDataUpdate();

  // Then real data (if possible)
  writeDataUpdate();

  getOutStream()->cork(false);

  congestion.updatePosition(sock->outStream().length());
}

void VNCSConnectionST::writeNoDataUpdate()
{
  if (!writer()->needNoDataUpdate())
    return;

  writer()->writeNoDataUpdate();

  // Make sure no data update is sent until next request
  requested.clear();
}

void VNCSConnectionST::writeDataUpdate()
{
  Region req;
  UpdateInfo ui;
  bool needNewUpdateInfo;
  const RenderedCursor *cursor;

  // See what the client has requested (if anything)
  if (continuousUpdates)
    req = cuRegion.union_(requested);
  else
    req = requested;

  if (req.is_empty())
    return;

  // Get the lists of updates. Prior to exporting the data to the `ui' object,
  // getUpdateInfo() will normalize the `updates' object such way that its
  // `changed' and `copied' regions would not intersect.
  updates.getUpdateInfo(&ui, req);
  needNewUpdateInfo = false;

  // If the previous position of the rendered cursor overlaps the source of the
  // copy, then when the copy happens the corresponding rectangle in the
  // destination will be wrong, so add it to the changed region.

  if (!ui.copied.is_empty() && !damagedCursorRegion.is_empty()) {
    Region bogusCopiedCursor;

    bogusCopiedCursor = damagedCursorRegion;
    bogusCopiedCursor.translate(ui.copy_delta);
    bogusCopiedCursor.assign_intersect(server->getPixelBuffer()->getRect());
    if (!ui.copied.intersect(bogusCopiedCursor).is_empty()) {
      updates.add_changed(bogusCopiedCursor);
      needNewUpdateInfo = true;
    }
  }

  // If we need to remove the old rendered cursor, just add the region to
  // the changed region.

  if (removeRenderedCursor) {
    updates.add_changed(damagedCursorRegion);
    needNewUpdateInfo = true;
    damagedCursorRegion.clear();
    removeRenderedCursor = false;
  }

  // If we need a full cursor update then make sure its entire region
  // is marked as changed.

  if (updateRenderedCursor) {
    updates.add_changed(server->getRenderedCursor()->getEffectiveRect());
    needNewUpdateInfo = true;
    updateRenderedCursor = false;
  }

  // The `updates' object could change, make sure we have valid update info.

  if (needNewUpdateInfo)
    updates.getUpdateInfo(&ui, req);

  // If there are queued updates then we cannot safely send an update
  // without risking a partially updated screen
  if (!server->getPendingRegion().is_empty()) {
    req.clear();
    ui.changed.clear();
    ui.copied.clear();
  }

  // Does the client need a server-side rendered cursor?

  cursor = NULL;
  if (needRenderedCursor()) {
    Rect renderedCursorRect;

    cursor = server->getRenderedCursor();
    renderedCursorRect = cursor->getEffectiveRect();

    // Check that we don't try to copy over the cursor area, and
    // if that happens we need to treat it as changed so that we can
    // re-render it
    if (!ui.copied.intersect(renderedCursorRect).is_empty()) {
      ui.changed.assign_union(ui.copied.intersect(renderedCursorRect));
      ui.copied.assign_subtract(renderedCursorRect);
    }

    // Track where we've rendered the cursor
    damagedCursorRegion.assign_union(ui.changed.intersect(renderedCursorRect));
  }

  // If we don't have a normal update, then try a lossless refresh
  if (ui.is_empty() && !writer()->needFakeUpdate()) {
    writeLosslessRefresh();
    return;
  }

  // We have something to send, so let's get to it

  writeRTTPing();

  encodeManager.writeUpdate(ui, server->getPixelBuffer(), cursor);

  writeRTTPing();

  // The request might be for just part of the screen, so we cannot
  // just clear the entire update tracker.
  updates.subtract(req);

  requested.clear();
}

void VNCSConnectionST::writeLosslessRefresh()
{
  Region req, pending;
  const RenderedCursor *cursor;

  int nextRefresh, nextUpdate;
  size_t bandwidth, maxUpdateSize;

  if (continuousUpdates)
    req = cuRegion.union_(requested);
  else
    req = requested;

  // If there are queued updates then we could not safely send an
  // update without risking a partially updated screen, however we
  // might still be able to send a lossless refresh
  pending = server->getPendingRegion();
  if (!pending.is_empty()) {
    UpdateInfo ui;

    // Don't touch the updates pending in the server core
    req.assign_subtract(pending);

    // Or any updates pending just for this connection
    updates.getUpdateInfo(&ui, req);
    req.assign_subtract(ui.changed);
    req.assign_subtract(ui.copied);
  }

  // Any lossy area we can refresh?
  if (!encodeManager.needsLosslessRefresh(req))
    return;

  // Right away? Or later?
  nextRefresh = encodeManager.getNextLosslessRefresh(req);
  if (nextRefresh > 0) {
    losslessTimer.start(nextRefresh);
    return;
  }

  // Prepare the cursor in case it overlaps with a region getting
  // refreshed
  cursor = NULL;
  if (needRenderedCursor())
    cursor = server->getRenderedCursor();

  // FIXME: If continuous updates aren't used then the client might
  //        be slower than frameRate in its requests and we could
  //        afford a larger update size
  nextUpdate = server->msToNextUpdate();

  // Don't bother if we're about to send a real update
  if (nextUpdate == 0)
    return;

  // FIXME: Bandwidth estimation without congestion control
  bandwidth = congestion.getBandwidth();

  // FIXME: Hard coded value for maximum CPU throughput
  if (bandwidth > 5000000)
    bandwidth = 5000000;

  maxUpdateSize = bandwidth * nextUpdate / 1000;

  writeRTTPing();

  encodeManager.writeLosslessRefresh(req, server->getPixelBuffer(),
                                     cursor, maxUpdateSize);

  writeRTTPing();

  requested.clear();
}


void VNCSConnectionST::screenLayoutChange(rdr::U16 reason)
{
  if (!authenticated())
    return;

  client.setDimensions(client.width(), client.height(),
                       server->getScreenLayout());

  if (state() != RFBSTATE_NORMAL)
    return;

  writer()->writeDesktopSize(reason);
}


// setCursor() is called whenever the cursor has changed shape or pixel format.
// If the client supports local cursor then it will arrange for the cursor to
// be sent to the client.

void VNCSConnectionST::setCursor()
{
  if (state() != RFBSTATE_NORMAL)
    return;

  // We need to blank out the client's cursor or there will be two
  if (needRenderedCursor()) {
    client.setCursor(emptyCursor);
    clientHasCursor = false;
  } else {
    client.setCursor(*server->getCursor());
    clientHasCursor = true;
  }

  if (client.supportsLocalCursor())
    writer()->writeCursor();
}

// setCursorPos() is called whenever the cursor has changed position by the
// server.  If the client supports being informed about these changes then it
// will arrange for the new cursor position to be sent to the client.

void VNCSConnectionST::setCursorPos()
{
  if (state() != RFBSTATE_NORMAL)
    return;

  if (client.supportsCursorPosition()) {
    client.setCursorPos(server->getCursorPos());
    writer()->writeCursorPos();
  }
}

void VNCSConnectionST::setDesktopName(const char *name)
{
  client.setName(name);

  if (state() != RFBSTATE_NORMAL)
    return;

  if (client.supportsEncoding(pseudoEncodingDesktopName))
    writer()->writeSetDesktopName();
}

void VNCSConnectionST::setLEDState(unsigned int ledstate)
{
  if (state() != RFBSTATE_NORMAL)
    return;

  client.setLEDState(ledstate);

  if (client.supportsLEDState())
    writer()->writeLEDState();
}
