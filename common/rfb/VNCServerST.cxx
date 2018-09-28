/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2018 Pierre Ossman for Cendio AB
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


#include <assert.h>
#include <stdlib.h>

#include <rfb/ComparingUpdateTracker.h>
#include <rfb/KeyRemapper.h>
#include <rfb/ListConnInfo.h>
#include <rfb/Security.h>
#include <rfb/ServerCore.h>
#include <rfb/VNCServerST.h>
#include <rfb/VNCSConnectionST.h>
#include <rfb/util.h>
#include <rfb/ledStates.h>

#include <rdr/types.h>

using namespace rfb;

static LogWriter slog("VNCServerST");
LogWriter VNCServerST::connectionsLog("Connections");

//
// -=- VNCServerST Implementation
//

// -=- Constructors/Destructor

VNCServerST::VNCServerST(const char* name_, SDesktop* desktop_)
  : blHosts(&blacklist), desktop(desktop_), desktopStarted(false),
    blockCounter(0), pb(0), ledState(ledUnknown),
    name(strDup(name_)), pointerClient(0), comparer(0),
    cursor(new Cursor(0, 0, Point(), NULL)),
    renderedCursorInvalid(false),
    queryConnectionHandler(0), keyRemapper(&KeyRemapper::defInstance),
    lastConnectionTime(0), disableclients(false),
    frameTimer(this)
{
  lastUserInputTime = lastDisconnectTime = time(0);
  slog.debug("creating single-threaded server %s", name.buf);
}

VNCServerST::~VNCServerST()
{
  slog.debug("shutting down server %s", name.buf);

  // Close any active clients, with appropriate logging & cleanup
  closeClients("Server shutdown");

  // Stop trying to render things
  stopFrameClock();

  // Delete all the clients, and their sockets, and any closing sockets
  //   NB: Deleting a client implicitly removes it from the clients list
  while (!clients.empty()) {
    delete clients.front();
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
      SConnection::writeConnFailedFromScratch("Too many security failures",
                                              &sock->outStream());
    } catch (rdr::Exception&) {
    }
    sock->shutdown();
    closingSockets.push_back(sock);
    return;
  }

  if (clients.empty()) {
    lastConnectionTime = time(0);
  }

  VNCSConnectionST* client = new VNCSConnectionST(this, sock, outgoing);
  client->init();
}

void VNCServerST::removeSocket(network::Socket* sock) {
  // - If the socket has resources allocated to it, delete them
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock) {
      // - Delete the per-Socket resources
      delete *ci;

      // - Check that the desktop object is still required
      if (authClientCount() == 0)
        stopDesktop();

      if (comparer)
        comparer->logStats();

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

int VNCServerST::checkTimeouts()
{
  int timeout = 0;
  std::list<VNCSConnectionST*>::iterator ci, ci_next;

  soonestTimeout(&timeout, Timer::checkTimeouts());

  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    soonestTimeout(&timeout, (*ci)->checkIdleTimeout());
  }

  int timeLeft;
  time_t now = time(0);

  // Check MaxDisconnectionTime 
  if (rfb::Server::maxDisconnectionTime && clients.empty()) {
    if (now < lastDisconnectTime) {
      // Someone must have set the time backwards. 
      slog.info("Time has gone backwards - resetting lastDisconnectTime");
      lastDisconnectTime = now;
    }
    timeLeft = lastDisconnectTime + rfb::Server::maxDisconnectionTime - now;
    if (timeLeft < -60) {
      // Someone must have set the time forwards.
      slog.info("Time has gone forwards - resetting lastDisconnectTime");
      lastDisconnectTime = now;
      timeLeft = rfb::Server::maxDisconnectionTime;
    }
    if (timeLeft <= 0) { 
      slog.info("MaxDisconnectionTime reached, exiting");
      exit(0);
    }
    soonestTimeout(&timeout, timeLeft * 1000);
  }

  // Check MaxConnectionTime 
  if (rfb::Server::maxConnectionTime && lastConnectionTime && !clients.empty()) {
    if (now < lastConnectionTime) {
      // Someone must have set the time backwards. 
      slog.info("Time has gone backwards - resetting lastConnectionTime");
      lastConnectionTime = now;
    }
    timeLeft = lastConnectionTime + rfb::Server::maxConnectionTime - now;
    if (timeLeft < -60) {
      // Someone must have set the time forwards.
      slog.info("Time has gone forwards - resetting lastConnectionTime");
      lastConnectionTime = now;
      timeLeft = rfb::Server::maxConnectionTime;
    }
    if (timeLeft <= 0) {
      slog.info("MaxConnectionTime reached, exiting");
      exit(0);
    }
    soonestTimeout(&timeout, timeLeft * 1000);
  }

  
  // Check MaxIdleTime 
  if (rfb::Server::maxIdleTime) {
    if (now < lastUserInputTime) {
      // Someone must have set the time backwards. 
      slog.info("Time has gone backwards - resetting lastUserInputTime");
      lastUserInputTime = now;
    }
    timeLeft = lastUserInputTime + rfb::Server::maxIdleTime - now;
    if (timeLeft < -60) {
      // Someone must have set the time forwards.
      slog.info("Time has gone forwards - resetting lastUserInputTime");
      lastUserInputTime = now;
      timeLeft = rfb::Server::maxIdleTime;
    }
    if (timeLeft <= 0) {
      slog.info("MaxIdleTime reached, exiting");
      exit(0);
    }
    soonestTimeout(&timeout, timeLeft * 1000);
  }
  
  return timeout;
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

  screenLayout = layout;

  if (!pb) {
    screenLayout = ScreenSet();

    if (desktopStarted)
      throw Exception("setPixelBuffer: null PixelBuffer when desktopStarted?");

    return;
  }

  // Assume the framebuffer contents wasn't saved and reset everything
  // that tracks its contents
  comparer = new ComparingUpdateTracker(pb);
  renderedCursorInvalid = true;
  add_changed(pb->getRect());

  // Make sure that we have at least one screen
  if (screenLayout.num_screens() == 0)
    screenLayout.add_screen(Screen(0, 0, 0, pb->width(), pb->height(), 0));

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

void VNCServerST::bell()
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->bellOrClose();
  }
}

void VNCServerST::serverCutText(const char* str, int len)
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci = clients.begin(); ci != clients.end(); ci = ci_next) {
    ci_next = ci; ci_next++;
    (*ci)->serverCutTextOrClose(str, len);
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

void VNCServerST::setCursorPos(const Point& pos)
{
  if (!cursorPos.equals(pos)) {
    cursorPos = pos;
    renderedCursorInvalid = true;
    std::list<VNCSConnectionST*>::iterator ci;
    for (ci = clients.begin(); ci != clients.end(); ci++)
      (*ci)->renderedCursorChange();
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

SConnection* VNCServerST::getSConnection(network::Socket* sock) {
  std::list<VNCSConnectionST*>::iterator ci;
  for (ci = clients.begin(); ci != clients.end(); ci++) {
    if ((*ci)->getSock() == sock)
      return *ci;
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
  }

  return false;
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

void VNCServerST::getConnInfo(ListConnInfo * listConn)
{
  listConn->Clear();
  listConn->setDisable(getDisable());
  if (clients.empty())
    return;
  std::list<VNCSConnectionST*>::iterator i;
  for (i = clients.begin(); i != clients.end(); i++)
    listConn->addInfo((void*)(*i), (*i)->getSock()->getPeerAddress(),
                      (*i)->getStartTime(), (*i)->getStatus());
}

void VNCServerST::setConnStatus(ListConnInfo* listConn)
{
  setDisable(listConn->getDisable());
  if (listConn->Empty() || clients.empty()) return;
  for (listConn->iBegin(); !listConn->iEnd(); listConn->iNext()) {
    VNCSConnectionST* conn = (VNCSConnectionST*)listConn->iGetConn();
    std::list<VNCSConnectionST*>::iterator i;
    for (i = clients.begin(); i != clients.end(); i++) {
      if ((*i) == conn) {
        int status = listConn->iGetStatus();
        if (status == 3) {
          (*i)->close(0);
        } else {
          (*i)->setStatus(status);
        }
        break;
      }
    }
  }
}

void VNCServerST::notifyScreenLayoutChange(VNCSConnectionST* requester)
{
  std::list<VNCSConnectionST*>::iterator ci, ci_next;
  for (ci=clients.begin();ci!=clients.end();ci=ci_next) {
    ci_next = ci; ci_next++;
    if ((*ci) == requester)
      continue;
    (*ci)->screenLayoutChangeOrClose(reasonOtherClient);
  }
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
