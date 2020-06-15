/* Copyright 2019 Aaron Sowry for Cendio AB
 * Copyright 2019-2020 Pierre Ossman for Cendio AB
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

#include <stdlib.h>
#include <math.h>

#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#include <rfb/util.h>

#include "GestureHandler.h"
#include "BaseTouchHandler.h"

// Sensitivity threshold for gestures
static const int ZOOMSENS = 30;
static const int SCRLSENS = 50;

static const unsigned DOUBLE_TAP_TIMEOUT   = 1000;
static const unsigned DOUBLE_TAP_THRESHOLD = 50;

BaseTouchHandler::BaseTouchHandler()
{
  gettimeofday(&lastTapTime, NULL);
}

BaseTouchHandler::~BaseTouchHandler()
{
}

void BaseTouchHandler::handleGestureEvent(const GestureEvent& ev)
{
  double magnitude;

  switch (ev.type) {
  case GestureBegin:
    switch (ev.gesture) {
    case GestureOneTap:
      handleTapEvent(ev, 1);
      break;
    case GestureTwoTap:
      handleTapEvent(ev, 3);
      break;
    case GestureThreeTap:
      handleTapEvent(ev, 2);
      break;
    case GestureDrag:
      fakeMotionEvent(ev);
      fakeButtonEvent(true, 1, ev);
      break;
    case GestureLongPress:
      fakeMotionEvent(ev);
      fakeButtonEvent(true, 3, ev);
      break;
    case GestureTwoDrag:
      lastMagnitudeX = ev.magnitudeX;
      lastMagnitudeY = ev.magnitudeY;
      fakeMotionEvent(ev);
      break;
    case GesturePinch:
      lastMagnitudeX = hypot(ev.magnitudeX, ev.magnitudeY);
      fakeMotionEvent(ev);
      break;
    }
    break;

  case GestureUpdate:
    switch (ev.gesture) {
    case GestureOneTap:
    case GestureTwoTap:
    case GestureThreeTap:
      break;
    case GestureDrag:
    case GestureLongPress:
      fakeMotionEvent(ev);
      break;
    case GestureTwoDrag:
      // Always scroll in the same position.
      // We don't know if the mouse was moved so we need to move it
      // every update.
      fakeMotionEvent(ev);
      while ((ev.magnitudeY - lastMagnitudeY) > SCRLSENS) {
        fakeButtonEvent(true, 4, ev);
        fakeButtonEvent(false, 4, ev);
        lastMagnitudeY += SCRLSENS;
      }
      while ((ev.magnitudeY - lastMagnitudeY) < -SCRLSENS) {
        fakeButtonEvent(true, 5, ev);
        fakeButtonEvent(false, 5, ev);
        lastMagnitudeY -= SCRLSENS;
      }
      while ((ev.magnitudeX - lastMagnitudeX) > SCRLSENS) {
        fakeButtonEvent(true, 6, ev);
        fakeButtonEvent(false, 6, ev);
        lastMagnitudeX += SCRLSENS;
      }
      while ((ev.magnitudeX - lastMagnitudeX) < -SCRLSENS) {
        fakeButtonEvent(true, 7, ev);
        fakeButtonEvent(false, 7, ev);
        lastMagnitudeX -= SCRLSENS;
      }
      break;
    case GesturePinch:
      // Always scroll in the same position.
      // We don't know if the mouse was moved so we need to move it
      // every update.
      fakeMotionEvent(ev);
      magnitude = hypot(ev.magnitudeX, ev.magnitudeY);
      if (abs(magnitude - lastMagnitudeX) > ZOOMSENS) {
        fakeKeyEvent(true, XK_Control_L, ev);

        while ((magnitude - lastMagnitudeX) > ZOOMSENS) {
          fakeButtonEvent(true, 4, ev);
          fakeButtonEvent(false, 4, ev);
          lastMagnitudeX += ZOOMSENS;
        }
        while ((magnitude - lastMagnitudeX) < -ZOOMSENS) {
          fakeButtonEvent(true, 5, ev);
          fakeButtonEvent(false, 5, ev);
          lastMagnitudeX -= ZOOMSENS;
        }

        fakeKeyEvent(false, XK_Control_L, ev);
      }
    }
    break;

  case GestureEnd:
    switch (ev.gesture) {
    case GestureOneTap:
    case GestureTwoTap:
    case GestureThreeTap:
    case GesturePinch:
    case GestureTwoDrag:
      break;
    case GestureDrag:
      fakeMotionEvent(ev);
      fakeButtonEvent(false, 1, ev);
      break;
    case GestureLongPress:
      fakeMotionEvent(ev);
      fakeButtonEvent(false, 3, ev);
      break;
    }
    break;
  }
}

void BaseTouchHandler::handleTapEvent(const GestureEvent& ev,
                                      int buttonEvent)
{
  GestureEvent newEv = ev;

  // If the user quickly taps multiple times we assume they meant to
  // hit the same spot, so slightly adjust coordinates
  if ((rfb::msSince(&lastTapTime) < DOUBLE_TAP_TIMEOUT) &&
      (firstDoubleTapEvent.type == ev.type)) {

    double dx = firstDoubleTapEvent.eventX - ev.eventX;
    double dy = firstDoubleTapEvent.eventY - ev.eventY;
    double distance = hypot(dx, dy);

    if (distance < DOUBLE_TAP_THRESHOLD) {
     newEv.eventX = firstDoubleTapEvent.eventX;
     newEv.eventY = firstDoubleTapEvent.eventY;
    } else {
      firstDoubleTapEvent = ev;
    }
  } else {
    firstDoubleTapEvent = ev;
  }
  gettimeofday(&lastTapTime, NULL);

  fakeMotionEvent(newEv);
  fakeButtonEvent(true, buttonEvent, newEv);
  fakeButtonEvent(false, buttonEvent, newEv);
}
