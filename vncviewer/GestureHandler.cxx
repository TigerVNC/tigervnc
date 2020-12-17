/* Copyright 2019 Aaron Sowry for Cendio AB
 * Copyright 2020 Pierre Ossman for Cendio AB
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
#include <math.h>

#include <rfb/util.h>
#include <rfb/LogWriter.h>

#include "GestureHandler.h"

static rfb::LogWriter vlog("GestureHandler");

static const unsigned char GH_NOGESTURE = 0;
static const unsigned char GH_ONETAP    = 1;
static const unsigned char GH_TWOTAP    = 2;
static const unsigned char GH_THREETAP  = 4;
static const unsigned char GH_DRAG      = 8;
static const unsigned char GH_LONGPRESS = 16;
static const unsigned char GH_TWODRAG   = 32;
static const unsigned char GH_PINCH     = 64;

static const unsigned char GH_INITSTATE = 127;

const unsigned GH_MOVE_THRESHOLD = 50;
const unsigned GH_ANGLE_THRESHOLD = 90; // Degrees

// Timeout when waiting for gestures (ms)
const unsigned GH_MULTITOUCH_TIMEOUT = 250;

// Maximum time between press and release for a tap (ms)
const unsigned GH_TAP_TIMEOUT = 1000;

// Timeout when waiting for longpress (ms)
const unsigned GH_LONGPRESS_TIMEOUT = 1000;

// Timeout when waiting to decide between PINCH and TWODRAG (ms)
const unsigned GH_TWOTOUCH_TIMEOUT = 50;

GestureHandler::GestureHandler() :
  state(GH_INITSTATE), waitingRelease(false),
  longpressTimer(this), twoTouchTimer(this)
{
}

GestureHandler::~GestureHandler()
{
}

void GestureHandler::handleTouchBegin(int id, double x, double y)
{
  GHTouch ght;

  // Ignore any new touches if there is already an active gesture,
  // or we're in a cleanup state
  if (hasDetectedGesture() || (state == GH_NOGESTURE)) {
    ignored.insert(id);
    return;
  }

  // Did it take too long between touches that we should no longer
  // consider this a single gesture?
  if ((tracked.size() > 0) &&
      (rfb::msSince(&tracked.begin()->second.started) > GH_MULTITOUCH_TIMEOUT)) {
    state = GH_NOGESTURE;
    ignored.insert(id);
    return;
  }

  // If we're waiting for fingers to release then we should no longer
  // recognize new touches
  if (waitingRelease) {
    state = GH_NOGESTURE;
    ignored.insert(id);
    return;
  }

  gettimeofday(&ght.started, NULL);
  ght.active = true;
  ght.lastX = ght.firstX = x;
  ght.lastY = ght.firstY = y;
  ght.angle = 0;

  tracked[id] = ght;

  switch (tracked.size()) {
    case 1:
      longpressTimer.start(GH_LONGPRESS_TIMEOUT);
      break;

    case 2:
      state &= ~(GH_ONETAP | GH_DRAG | GH_LONGPRESS);
      longpressTimer.stop();
      break;

    case 3:
      state &= ~(GH_TWOTAP | GH_TWODRAG | GH_PINCH);
      break;

    default:
      state = GH_NOGESTURE;
  }
}

void GestureHandler::handleTouchUpdate(int id, double x, double y)
{
  GHTouch *touch, *prevTouch;
  double deltaX, deltaY, prevDeltaMove;
  unsigned deltaAngle;

  // If this is an update for a touch we're not tracking, ignore it
  if (tracked.count(id) == 0)
    return;

  touch = &tracked[id];

  // Update the touches last position with the event coordinates
  touch->lastX = x;
  touch->lastY = y;

  deltaX = x - touch->firstX;
  deltaY = y - touch->firstY;

  // Update angle when the touch has moved
  if ((touch->firstX != touch->lastX) ||
      (touch->firstY != touch->lastY))
    touch->angle = atan2(deltaY, deltaX) * 180 / M_PI;

  if (!hasDetectedGesture()) {
    // Ignore moves smaller than the minimum threshold
    if (hypot(deltaX, deltaY) < GH_MOVE_THRESHOLD)
      return;

    // Can't be a tap or long press as we've seen movement
    state &= ~(GH_ONETAP | GH_TWOTAP | GH_THREETAP | GH_LONGPRESS);
    longpressTimer.stop();

    if (tracked.size() != 1)
      state &= ~(GH_DRAG);
    if (tracked.size() != 2)
      state &= ~(GH_TWODRAG | GH_PINCH);

    // We need to figure out which of our different two touch gestures
    // this might be
    if (tracked.size() == 2) {

      // The other touch can be first or last in tracked
      // depending on which event came first
      prevTouch = &tracked.rbegin()->second;
      if (prevTouch == touch)
        prevTouch = &tracked.begin()->second;

      // How far the previous touch point has moved since start
      prevDeltaMove = hypot(prevTouch->firstX - prevTouch->lastX,
                            prevTouch->firstY - prevTouch->lastY);

      // We know that the current touch moved far enough,
      // but unless both touches moved further than their
      // threshold we don't want to disqualify any gestures
      if (prevDeltaMove > GH_MOVE_THRESHOLD) {

        // The angle difference between the direction of the touch points
        deltaAngle = fabs(touch->angle - prevTouch->angle);
        deltaAngle = fabs(((deltaAngle + 180) % 360) - 180);

        // PINCH or TWODRAG can be eliminated depending on the angle
        if (deltaAngle > GH_ANGLE_THRESHOLD)
          state &= ~GH_TWODRAG;
        else
          state &= ~GH_PINCH;

        if (twoTouchTimer.isStarted())
          twoTouchTimer.stop();

      } else if (!twoTouchTimer.isStarted()) {
        // We can't determine the gesture right now, let's
        // wait and see if more events are on their way
        twoTouchTimer.start(GH_TWOTOUCH_TIMEOUT);
      }
    }

    if (!hasDetectedGesture())
      return;

    pushEvent(GestureBegin);
  }

  pushEvent(GestureUpdate);
}

void GestureHandler::handleTouchEnd(int id)
{
  std::map<int, GHTouch>::const_iterator iter;

  // Check if this is an ignored touch
  if (ignored.count(id)) {
      ignored.erase(id);
      if (ignored.empty() && tracked.empty()) {
        state = GH_INITSTATE;
        waitingRelease = false;
      }
      return;
  }

  // We got a TouchEnd before the timer triggered,
  // this cannot result in a gesture anymore.
  if (!hasDetectedGesture() && twoTouchTimer.isStarted()) {
    twoTouchTimer.stop();
    state = GH_NOGESTURE;
  }

  // Some gestures don't trigger until a touch is released
  if (!hasDetectedGesture()) {
    // Can't be a gesture that relies on movement
    state &= ~(GH_DRAG | GH_TWODRAG | GH_PINCH);
    // Or something that relies on more time
    state &= ~GH_LONGPRESS;
    longpressTimer.stop();

    if (!waitingRelease) {
      gettimeofday(&releaseStart, NULL);
      waitingRelease = true;

      // Can't be a tap that requires more touches than we current have
      switch (tracked.size()) {
        case 1:
          state &= ~(GH_TWOTAP | GH_THREETAP);
          break;

        case 2:
          state &= ~(GH_ONETAP | GH_THREETAP);
          break;
      }
    }
  }

  // Waiting for all touches to release? (i.e. some tap)
  if (waitingRelease) {
    // Were all touches released at roughly the same time?
    if (rfb::msSince(&releaseStart) > GH_MULTITOUCH_TIMEOUT)
      state = GH_NOGESTURE;

    // Did too long time pass between press and release?
    for (iter = tracked.begin(); iter != tracked.end(); ++iter) {
      if (rfb::msSince(&iter->second.started) > GH_TAP_TIMEOUT) {
        state = GH_NOGESTURE;
        break;
      }
    }

    tracked[id].active = false;

    // Are we still waiting for more releases?
    if (hasDetectedGesture()) {
      pushEvent(GestureBegin);
    } else {
      // Have we reached a dead end?
      if (state != GH_NOGESTURE)
        return;
    }
  }

  if (hasDetectedGesture())
    pushEvent(GestureEnd);

  // Ignore any remaining touches until they are ended
  for (iter = tracked.begin(); iter != tracked.end(); ++iter) {
    if (iter->second.active)
      ignored.insert(iter->first);
  }
  tracked.clear();

  state = GH_NOGESTURE;

  ignored.erase(id);
  if (ignored.empty()) {
    state = GH_INITSTATE;
    waitingRelease = false;
  }
}

bool GestureHandler::hasDetectedGesture()
{
  if (state == GH_NOGESTURE)
    return false;
  // Check to see if the bitmask value is a power of 2
  // (i.e. only one bit set). If it is, we have a state.
  if (state & (state - 1))
    return false;

  // For taps we also need to have all touches released
  // before we've fully detected the gesture
  if (state & (GH_ONETAP | GH_TWOTAP | GH_THREETAP)) {
    std::map<int, GHTouch>::const_iterator iter;

    // Any touch still active/pressed?
    for (iter = tracked.begin(); iter != tracked.end(); ++iter) {
      if (iter->second.active)
        return false;
    }
  }

  return true;
}

bool GestureHandler::handleTimeout(rfb::Timer* t)
{
  if (t == &longpressTimer)
    longpressTimeout();
  else if (t == &twoTouchTimer)
    twoTouchTimeout();

  return false;
}

void GestureHandler::longpressTimeout()
{
  assert(!hasDetectedGesture());

  state = GH_LONGPRESS;
  pushEvent(GestureBegin);
}

void GestureHandler::twoTouchTimeout()
{
  double avgMoveH, avgMoveV, fdx, fdy, ldx, ldy, deltaTouchDistance;

  assert(!tracked.empty());

  // How far each touch point has moved since start
  getAverageMovement(&avgMoveH, &avgMoveV);
  avgMoveH = fabs(avgMoveH);
  avgMoveV = fabs(avgMoveV);

  // The difference in the distance between where
  // the touch points started and where they are now
  getAverageDistance(&fdx, &fdy, &ldx, &ldy);
  deltaTouchDistance = fabs(hypot(fdx, fdy) - hypot(ldx, ldy));

  if ((avgMoveV < deltaTouchDistance) &&
      (avgMoveH < deltaTouchDistance))
    state = GH_PINCH;
  else
    state = GH_TWODRAG;

  pushEvent(GestureBegin);
  pushEvent(GestureUpdate);
}

void GestureHandler::pushEvent(GestureEventType t)
{
  GestureEvent gev;
  double avgX, avgY;

  gev.type = t;
  gev.gesture = stateToGesture(state);

  // For most gesture events the current (average) position is the
  // most useful
  getPosition(NULL, NULL, &avgX, &avgY);

  // However we have a slight distance to detect gestures, so for the
  // first gesture event we want to use the first positions we saw
  if (t == GestureBegin)
    getPosition(&avgX, &avgY, NULL, NULL);

  // For these gestures, we always want the event coordinates
  // to be where the gesture began, not the current touch location.
  switch (state) {
    case GH_TWODRAG:
    case GH_PINCH:
      getPosition(&avgX, &avgY, NULL, NULL);
      break;
  }

  gev.eventX = avgX;
  gev.eventY = avgY;

  // Some gestures also have a magnitude
  if (state == GH_PINCH) {
    if (t == GestureBegin)
      getAverageDistance(&gev.magnitudeX, &gev.magnitudeY,
                         NULL, NULL);
    else
      getAverageDistance(NULL, NULL,
                         &gev.magnitudeX, &gev.magnitudeY);
  } else if (state == GH_TWODRAG) {
    if (t == GestureBegin)
      gev.magnitudeX = gev.magnitudeY = 0;
    else
      getAverageMovement(&gev.magnitudeX, &gev.magnitudeY);
  }

  handleGestureEvent(gev);
}

GestureEventGesture GestureHandler::stateToGesture(unsigned char state)
{
  switch (state) {
    case GH_ONETAP:
      return GestureOneTap;
    case GH_TWOTAP:
      return GestureTwoTap;
    case GH_THREETAP:
      return GestureThreeTap;
    case GH_DRAG:
      return GestureDrag;
    case GH_LONGPRESS:
      return GestureLongPress;
    case GH_TWODRAG:
      return GestureTwoDrag;
    case GH_PINCH:
      return GesturePinch;
  }

  assert(false);

  return (GestureEventGesture)0;
}

void GestureHandler::getPosition(double *firstX, double *firstY,
                                 double *lastX, double *lastY)
{
  size_t size;
  double fx = 0, fy = 0, lx = 0, ly = 0;

  assert(!tracked.empty());

  size = tracked.size();

  std::map<int, GHTouch>::const_iterator iter;
  for (iter = tracked.begin(); iter != tracked.end(); ++iter) {
    fx += iter->second.firstX;
    fy += iter->second.firstY;
    lx += iter->second.lastX;
    ly += iter->second.lastY;
  }

  if (firstX)
    *firstX = fx / size;
  if (firstY)
    *firstY = fy / size;
  if (lastX)
    *lastX = lx / size;
  if (lastY)
    *lastY = ly / size;
}

void GestureHandler::getAverageMovement(double *h, double *v)
{
  double totalH, totalV;
  size_t size;

  assert(!tracked.empty());

  totalH = totalV = 0;
  size = tracked.size();

  std::map<int, GHTouch>::const_iterator iter;
  for (iter = tracked.begin(); iter != tracked.end(); ++iter) {
    totalH += iter->second.lastX - iter->second.firstX;
    totalV += iter->second.lastY - iter->second.firstY;
  }

  if (h)
    *h = totalH / size;
  if (v)
    *v = totalV / size;
}

void GestureHandler::getAverageDistance(double *firstX, double *firstY,
                                        double *lastX, double *lastY)
{
  double dx, dy;

  assert(!tracked.empty());

  // Distance between the first and last tracked touches

  dx = fabs(tracked.rbegin()->second.firstX - tracked.begin()->second.firstX);
  dy = fabs(tracked.rbegin()->second.firstY - tracked.begin()->second.firstY);

  if (firstX)
    *firstX = dx;
  if (firstY)
    *firstY = dy;

  dx = fabs(tracked.rbegin()->second.lastX - tracked.begin()->second.lastX);
  dy = fabs(tracked.rbegin()->second.lastY - tracked.begin()->second.lastY);

  if (lastX)
    *lastX = dx;
  if (lastY)
    *lastY = dy;
}
