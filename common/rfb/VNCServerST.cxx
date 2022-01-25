/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
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

// -=- Single-Threaded VNC Server implementation


// Note about how sockets get closed:
//
// Closing sockets to clients is non-trivial because the code which calls
// VNCServerST must explicitly know about all the sockets (so that it can block
// on them appropriately).  However, VNCServerST may want to close clients for
// a number of reasons, and from a variety of entry points.  The simplest is
// when processSocketEvent() is called for a client, and the remote end has
// closed its socket.  A more complex reason is when processSocketEvent() is
// called for a client which has just sent a ClientInit with the shared flag
// set to false - in this case we want to close all other clients.  Yet another
// reason for disconnecting clients is when the desktop size has changed as a
// result of a call to setPixelBuffer().
//
// The responsibility for creating and deleting sockets is entirely with the
// calling code.  When VNCServerST wants to close a connection to a client it
// calls the VNCSConnectionST's close() method which calls shutdown() on the
// socket.  Eventually the calling code will notice that the socket has been
// shut down and call removeSocket() so that we can delete the
// VNCSConnectionST.  Note that the socket must not be deleted by the calling
// code until after removeSocket() has been called.
//
// One minor complication is that we don't allocate a VNCSConnectionST object
// for a blacklisted host (since we want to minimise the resources used for
// dealing with such a connection).  In order to properly implement the
// getSockets function, we must maintain a separate closingSockets list,
// otherwise blacklisted connections might be "forgotten".


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <rfb/ComparingUpdateTracker.h>
#include <rfb/KeyRemapper.h>
#include <rfb/LogWriter.h>
#include <rfb/Security.h>
#include <rfb/ServerCore.h>
#include <rfb/VNCServerST.h>
#include <rfb/VNCSConnectionST.h>
#include <rfb/util.h>
#include <rfb/ledStates.h>

#include <rdr/types.h>

using namespace rfb;

static LogWriter slog("VNCServerST");
static LogWriter connectionsLog("Connections");

//
// -=- VNCServerST Implementation
//

// -=- Constructors/Destructor

VNCServerST::VNCServerST(const char* name_, SDesktop* desktop_)
  : blHosts(&blacklist), desktop(desktop_), desktopStarted(false),
    blockCounter(0), pb(0), ledState(ledUnknown),
    name(strDup(name_)), pointerClient(0), clipboardClient(0),
    comparer(0), cursor(new Cursor(0, 0, Point(), NULL)),
    renderedCursorInvalid(false),
    keyRemapper(&KeyRemapper::defInstance),
    idleTimer(this), disconnectTimer(this), connectTimer(this),
    frameTimer(this)
{
  slog.debug("creating single-threaded server %s", name.buf);

  // FIXME: Do we really want to kick off these right away?
  if (rfb::Server::maxIdleTime)
    idleTimer.start(secsToMillis(rfb::Server::maxIdleTime));
  if (rfb::Server::maxDisconnectionTime)
    disconnectTimer.start(secsToMillis(rfb::Server::maxDisconnectionTime));
}

VNCServerST::~VNCServerST()
{
  slog.debug("shutting down server %s", name.buf);

  // Close any active clients, with appropriate logging & cleanup
  closeClients("Server shutdown");

  // Stop trying to render things
  stopFrameClock();

  // Delete all the clients, and their sockets, and any closing sockets
  while (!clients.empty()) {
    VNCSConnectionST* client;
    client = clients.front();
    clients.pop_front();
    delete client;
  }

  // Stop the desktop object if active, *only* after deleting all clients!
  stopDesktop();

  if (comparer)
    comparer->logStats();
  delete comparer;

  delete cursor;
}


// SocketServer methods

void VNCServerST::addSocket(network::Socket* sock, bool outgoing)
{
  // - Check the connection isn't black-marked
  // *** do this in getSecurity instead?
  CharArray address(sock->getPeerAddress());
  if (blHosts->isBlackmarked(address.buf)) {
    connectionsLog.error("blacklisted: %s", address.buf);
    try {
      rdr::OutStream& os = sock->outStream();

      // Shortest possible way to tell a client it is not welcome
      os.writeBytes("RFB 003.003\n", 12);
      os.writeU32(0);
      const char* reason = "Too many security failures";
      os.writeU32(strlen(reason));
      os.writeBytes(reason, strlen(reason));
      os.flush();
    } catch (rdr::Exception&) {
    }
    sock->shutdown();
    closingSockets.push_back(sock);
    return;
  }

  CharArray name;
  name.buf = sock->getPeerEndpoint();
  connectionsLog.status("accepted: %s", name.buf);

  // Adjust the exit timers
  if (rfb::Server::maxConnectionTime && clients.empty())
    connectTimer.start(secsToMillis(rfb::Server::maxConnectionTime));
  disconnectTimer.stop();

  VNCSConnectionST* client = new VNCSConnectionST(this, sock, outgoing);
  clients.push_front(client);
  client->init();
}

void VNCServerST::removeSocket(network::Socket* sock) {
  // - If the socket has resources allocated to it, delete them
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock) {
      // - Remove any references to it
      if (pointerClient == *ci)
        pointerClient = NULL;
      if (clipboardClient == *ci)
        handleClipboardAnnounce(*ci, false);
      clipboardRequestors.remove(*ci);

      CharArray name(strDup((*ci)->getPeerEndpoint()));

      // - Delete the per-Socket resources
      delete *ci;

      clients.remove(*ci);

      connectionsLog.status("closed: %s", name.buf);

      // - Check that the desktop object is still required
      if (authClientCount() == 0)
        stopDesktop();

      if (comparer)
        comparer->logStats();

      // Adjust the exit timers
      connectTimer.stop();
      if (rfb::Server::maxDisconnectionTime && clients.empty())
        disconnectTimer.start(secsToMillis(rfb::Server::maxDisconnectionTime));

      return;
    }
  }

  // - If the Socket has no resources, it may have been a closingSocket
  closingSockets.remove(sock);
}

void VNCServerST::processSocketReadEvent(network::Socket* sock)
{
  // - Find the appropriate VNCSConnectionST and process the event
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock) {
      (*ci)->processMessages();
      return;
    }
  }
  throw rdr::Exception("invalid Socket in VNCServerST");
}

void VNCServerST::processSocketWriteEvent(network::Socket* sock)
{
  // - Find the appropriate VNCSConnectionST and process the event
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock) {
      (*ci)->flushSocket();
      return;
    }
  }
  throw rdr::Exception("invalid Socket in VNCServerST");
}

// VNCServer methods

void VNCServerST::blockUpdates()
{
  blockCounter++;

  stopFrameClock();
}

void VNCServerST::unblockUpdates()
{
  assert(blockCounter > 0);

  blockCounter--;

  // Restart the frame clock if we have updates
  if (blockCounter == 0) {
    if (!comparer->is_empty())
      startFrameClock();
  }
}

void VNCServerST::setPixelBuffer(PixelBuffer* pb_, const ScreenSet& layout)
{
  if (comparer)
    comparer->logStats();

  pb = pb_;
  delete comparer;
  comparer = 0;

  if (!pb) {
    screenLayout = ScreenSet();

    if (desktopStarted)
      throw Exception("setPixelBuffer: null PixelBuffer when desktopStarted?");

    return;
  }

  if (!layout.validate(pb->width(), pb->height()))
    throw Exception("setPixelBuffer: invalid screen layout");

  screenLayout = layout;

  // Assume the framebuffer contents wasn't saved and reset everything
  // that tracks its contents
  comparer = new ComparingUpdateTracker(pb);
  renderedCursorInvalid = true;
  add_changed(pb->getRect());

  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->pixelBufferChange();
    // Since the new pixel buffer means an ExtendedDesktopSize needs to
    // be sent anyway, we don't need to call screenLayoutChange.
  }
}

void VNCServerST::setPixelBuffer(PixelBuffer* pb_)
{
  ScreenSet layout = screenLayout;

  // Check that the screen layout is still valid
  if (pb_ && !layout.validate(pb_->width(), pb_->height())) {
    Rect fbRect;
    ScreenSet::iterator iter, iter_next;

    fbRect.setXYWH(0, 0, pb_->width(), pb_->height());

    for (iter = layout.begin();iter != layout.end();iter = iter_next) {
      iter_next = iter; ++iter_next;
      if (iter->dimensions.enclosed_by(fbRect))
          continue;
      iter->dimensions = iter->dimensions.intersect(fbRect);
      if (iter->dimensions.is_empty()) {
        slog.info("Removing screen %d (%x) as it is completely outside the new framebuffer",
                  (int)iter->id, (unsigned)iter->id);
        layout.remove_screen(iter->id);
      }
    }
  }

  // Make sure that we have at least one screen
  if (layout.num_screens() == 0)
    layout.add_screen(Screen(0, 0, 0, pb_->width(), pb_->height(), 0));

  setPixelBuffer(pb_, layout);
}

void VNCServerST::setScreenLayout(const ScreenSet& layout)
{
  if (!pb)
    throw Exception("setScreenLayout: new screen layout without a PixelBuffer");
  if (!layout.validate(pb->width(), pb->height()))
    throw Exception("setScreenLayout: invalid screen layout");

  screenLayout = layout;

  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->screenLayoutChangeOrClose(reasonServer);
  }
}

void VNCServerST::requestClipboard()
{
  if (clipboardClient == NULL) {
    slog.debug("Got request for client clipboard but no client currently owns the clipboard");
    return;
  }

  clipboardClient->requestClipboardOrClose();
}

void VNCServerST::announceClipboard(bool available)
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  clipboardRequestors.clear();

  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->announceClipboardOrClose(available);
  }
}

void VNCServerST::sendClipboardData(const char* data)
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  if (strchr(data, '\r') != NULL)
    throw Exception("Invalid carriage return in clipboard data");

  for (ci = clipboardRequestors.begin();
       ci != clipboardRequestors.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->sendClipboardDataOrClose(data);
  }

  clipboardRequestors.clear();
}

void VNCServerST::bell()
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->bellOrClose();
  }
}

void VNCServerST::setName(const char* name_)
{
  name.replaceBuf(strDup(name_));
  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->setDesktopNameOrClose(name_);
  }
}

void VNCServerST::add_changed(const Region& region)
{
  if (comparer == NULL)
    return;

  comparer->add_changed(region);
  startFrameClock();
}

void VNCServerST::add_copied(const Region& dest, const Point& delta)
{
  if (comparer == NULL)
    return;

  comparer->add_copied(dest, delta);
  startFrameClock();
}

void VNCServerST::setCursor(int width, int height, const Point& newHotspot,
                            const rdr::U8* data)
{
  delete cursor;
  cursor = new Cursor(width, height, newHotspot, data);
  cursor->crop();

  renderedCursorInvalid = true;

  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->renderedCursorChange();
    (*ci)->setCursorOrClose();
  }
}

void VNCServerST::setCursorPos(const Point& pos, bool warped)
{
  if (!cursorPos.equals(pos)) {
    cursorPos = pos;
    renderedCursorInvalid = true;
    std::list<VNCSConnectionST*>::iterator ci;
    for (ci = clients.begin(); ci != clients.end(); ci++) {
      (*ci)->renderedCursorChange();
      if (warped)
        (*ci)->cursorPositionChange();
    }
  }
}

void VNCServerST::setLEDState(unsigned int state)
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  if (state == ledState)
    return;

  ledState = state;

  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->setLEDStateOrClose(state);
  }
}

// Event handlers

void VNCServerST::keyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down)
{
  if (rfb::Server::maxIdleTime)
    idleTimer.start(secsToMillis(rfb::Server::maxIdleTime));

  // Remap the key if required
  if (keyRemapper) {
    rdr::U32 newkey;
    newkey = keyRemapper->remapKey(keysym);
    if (newkey != keysym) {
      slog.debug("Key remapped to 0x%x", newkey);
      keysym = newkey;
    }
  }

  desktop->keyEvent(keysym, keycode, down);
}

void VNCServerST::pointerEvent(VNCSConnectionST* client,
                               const Point& pos, int buttonMask)
{
  if (rfb::Server::maxIdleTime)
    idleTimer.start(secsToMillis(rfb::Server::maxIdleTime));

  // Let one client own the cursor whilst buttons are pressed in order
  // to provide a bit more sane user experience
  if ((pointerClient != NULL) && (pointerClient != client))
    return;

  if (buttonMask)
    pointerClient = client;
  else
    pointerClient = NULL;

  desktop->pointerEvent(pos, buttonMask);
}

void VNCServerST::handleClipboardRequest(VNCSConnectionST* client)
{
  clipboardRequestors.push_back(client);
  if (clipboardRequestors.size() == 1)
    desktop->handleClipboardRequest();
}

void VNCServerST::handleClipboardAnnounce(VNCSConnectionST* client,
                                          bool available)
{
  if (available)
    clipboardClient = client;
  else {
    if (client != clipboardClient)
      return;
    clipboardClient = NULL;
  }
  desktop->handleClipboardAnnounce(available);
}

void VNCServerST::handleClipboardData(VNCSConnectionST* client,
                                      const char* data)
{
  if (client != clipboardClient) {
    slog.debug("Ignoring unexpected clipboard data");
    return;
  }
  desktop->handleClipboardData(data);
}

unsigned int VNCServerST::setDesktopSize(VNCSConnectionST* requester,
                                         int fb_width, int fb_height,
                                         const ScreenSet& layout)
{
  unsigned int result;
  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  // We can't handle a framebuffer larger than this, so don't let a
  // client set one (see PixelBuffer.cxx)
  if ((fb_width > 16384) || (fb_height > 16384)) {
    slog.error("Rejecting too large framebuffer resize request");
    return resultProhibited;
  }

  // Don't bother the desktop with an invalid configuration
  if (!layout.validate(fb_width, fb_height)) {
    slog.error("Invalid screen layout requested by client");
    return resultInvalid;
  }

  // FIXME: the desktop will call back to VNCServerST and an extra set
  // of ExtendedDesktopSize messages will be sent. This is okay
  // protocol-wise, but unnecessary.
  result = desktop->setScreenLayout(fb_width, fb_height, layout);
  if (result != resultSuccess)
    return result;

  // Sanity check
  if (screenLayout != layout)
    throw Exception("Desktop configured a different screen layout than requested");

  // Notify other clients
  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    if ((*ci) == requester)
      continue;
    (*ci)->screenLayoutChangeOrClose(reasonOtherClient);
  }

  return resultSuccess;
}

// Other public methods

void VNCServerST::approveConnection(network::Socket* sock, bool accept,
                                    const char* reason)
{
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock) {
      (*ci)->approveConnectionOrClose(accept, reason);
      return;
    }
  }
}

void VNCServerST::closeClients(const char* reason, network::Socket* except)
{
  std::list<VNCSConnectionST*>::iterator i, next_i;
  for (i=clients.begin(); i!=clients.end(); i=next_i) {
    next_i = i; next_i++;
    if ((*i)->getSock() != except)
      (*i)->close(reason);
  }
}

void VNCServerST::getSockets(std::list<network::Socket*>* sockets)
{
  sockets->clear();
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    sockets->push_back((*ci)->getSock());
  }
  std::list<network::Socket*>::iterator si;
  for (si = closingSockets.begin(); si != closingSockets.end(); si++) {
    sockets->push_back(*si);
  }
}

SConnection* VNCServerST::getConnection(network::Socket* sock) {
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock)
      return (SConnection*)*ci;
  }
  return 0;
}

bool VNCServerST::handleTimeout(Timer* t)
{
  if (t == &frameTimer) {
    // We keep running until we go a full interval without any updates
    if (comparer->is_empty())
      return false;

    writeUpdate();

    // If this is the first iteration then we need to adjust the timeout
    if (frameTimer.getTimeoutMs() != 1000/rfb::Server::frameRate) {
      frameTimer.start(1000/rfb::Server::frameRate);
      return false;
    }

    return true;
  } else if (t == &idleTimer) {
    slog.info("MaxIdleTime reached, exiting");
    desktop->terminate();
  } else if (t == &disconnectTimer) {
    slog.info("MaxDisconnectionTime reached, exiting");
    desktop->terminate();
  } else if (t == &connectTimer) {
    slog.info("MaxConnectionTime reached, exiting");
    desktop->terminate();
  }

  return false;
}

void VNCServerST::queryConnection(VNCSConnectionST* client,
                                  const char* userName)
{
  // - Authentication succeeded - clear from blacklist
  CharArray name;
  name.buf = client->getSock()->getPeerAddress();
  blHosts->clearBlackmark(name.buf);

  // - Prepare the desktop for that the client will start requiring
  // resources after this
  startDesktop();

  // - Special case to provide a more useful error message
  if (rfb::Server::neverShared &&
      !rfb::Server::disconnectClients &&
      authClientCount() > 0) {
    approveConnection(client->getSock(), false,
                      "The server is already in use");
    return;
  }

  // - Are we configured to do queries?
  if (!rfb::Server::queryConnect &&
      !client->getSock()->requiresQuery()) {
    approveConnection(client->getSock(), true, NULL);
    return;
  }

  // - Does the client have the right to bypass the query?
  if (client->accessCheck(SConnection::AccessNoQuery))
  {
    approveConnection(client->getSock(), true, NULL);
    return;
  }

  desktop->queryConnection(client->getSock(), userName);
}

void VNCServerST::clientReady(VNCSConnectionST* client, bool shared)
{
  if (!shared) {
    if (rfb::Server::disconnectClients &&
        client->accessCheck(SConnection::AccessNonShared)) {
      // - Close all the other connected clients
      slog.debug("non-shared connection - closing clients");
      closeClients("Non-shared connection requested", client->getSock());
    } else {
      // - Refuse this connection if there are existing clients, in addition to
      // this one
      if (authClientCount() > 1) {
        client->close("Server is already in use");
        return;
      }
    }
  }
}

// -=- Internal methods

void VNCServerST::startDesktop()
{
  if (!desktopStarted) {
    slog.debug("starting desktop");
    desktop->start(this);
    if (!pb)
      throw Exception("SDesktop::start() did not set a valid PixelBuffer");
    desktopStarted = true;
    // The tracker might have accumulated changes whilst we were
    // stopped, so flush those out
    if (!comparer->is_empty())
      writeUpdate();
  }
}

void VNCServerST::stopDesktop()
{
  if (desktopStarted) {
    slog.debug("stopping desktop");
    desktopStarted = false;
    desktop->stop();
    stopFrameClock();
  }
}

int VNCServerST::authClientCount() {
  int count = 0;
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->authenticated())
      count++;
  }
  return count;
}

inline bool VNCServerST::needRenderedCursor()
{
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++)
    if ((*ci)->needRenderedCursor()) return true;
  return false;
}

void VNCServerST::startFrameClock()
{
  if (frameTimer.isStarted())
    return;
  if (blockCounter > 0)
    return;
  if (!desktopStarted)
    return;

  // The first iteration will be just half a frame as we get a very
  // unstable update rate if we happen to be perfectly in sync with
  // the application's update rate
  frameTimer.start(1000/rfb::Server::frameRate/2);
}

void VNCServerST::stopFrameClock()
{
  frameTimer.stop();
}

int VNCServerST::msToNextUpdate()
{
  // FIXME: If the application is updating slower than frameRate then
  //        we could allow the clients more time here

  if (!frameTimer.isStarted())
    return 1000/rfb::Server::frameRate/2;
  else
    return frameTimer.getRemainingMs();
}

// writeUpdate() is called on a regular interval in order to see what
// updates are pending and propagates them to the update tracker for
// each client. It uses the ComparingUpdateTracker's compare() method
// to filter out areas of the screen which haven't actually changed. It
// also checks the state of the (server-side) rendered cursor, if
// necessary rendering it again with the correct background.

void VNCServerST::writeUpdate()
{
  UpdateInfo ui;
  Region toCheck;

  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  assert(blockCounter == 0);
  assert(desktopStarted);

  comparer->getUpdateInfo(&ui, pb->getRect());
  toCheck = ui.changed.union_(ui.copied);

  if (needRenderedCursor()) {
    Rect clippedCursorRect = Rect(0, 0, cursor->width(), cursor->height())
                             .translate(cursorPos.subtract(cursor->hotspot()))
                             .intersect(pb->getRect());

    if (!toCheck.intersect(clippedCursorRect).is_empty())
      renderedCursorInvalid = true;
  }

  pb->grabRegion(toCheck);

  if (getComparerState())
    comparer->enable();
  else
    comparer->disable();

  if (comparer->compare())
    comparer->getUpdateInfo(&ui, pb->getRect());

  comparer->clear();

  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->add_copied(ui.copied, ui.copy_delta);
    (*ci)->add_changed(ui.changed);
    (*ci)->writeFramebufferUpdateOrClose();
  }
}

// checkUpdate() is called by clients to see if it is safe to read from
// the framebuffer at this time.

Region VNCServerST::getPendingRegion()
{
  UpdateInfo ui;

  // Block clients as the frame buffer cannot be safely accessed
  if (blockCounter > 0)
    return pb->getRect();

  // Block client from updating if there are pending updates
  if (comparer->is_empty())
    return Region();

  comparer->getUpdateInfo(&ui, pb->getRect());

  return ui.changed.union_(ui.copied);
}

const RenderedCursor* VNCServerST::getRenderedCursor()
{
  if (renderedCursorInvalid) {
    renderedCursor.update(pb, cursor, cursorPos);
    renderedCursorInvalid = false;
  }

  return &renderedCursor;
}

bool VNCServerST::getComparerState()
{
  if (rfb::Server::compareFB == 0)
    return false;
  if (rfb::Server::compareFB != 2)
    return true;

  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    if ((*ci)->getComparerState())
      return true;
  }
  return false;
}
