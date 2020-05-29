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

#ifndef __GESTUREHANDLER_H__
#define __GESTUREHANDLER_H__

#include <set>
#include <map>

#include <rfb/Timer.h>

#include "GestureEvent.h"

class GestureHandler : public rfb::Timer::Callback {
  public:
    GestureHandler();
    virtual ~GestureHandler();

    void handleTouchBegin(int id, double x, double y);
    void handleTouchUpdate(int id, double x, double y);
    void handleTouchEnd(int id);

  protected:
    virtual void handleGestureEvent(const GestureEvent& event) = 0;

  private:
    bool hasDetectedGesture();

    virtual bool handleTimeout(rfb::Timer* t);
    void longpressTimeout();
    void twoTouchTimeout();

    void pushEvent(GestureEventType t);
    GestureEventGesture stateToGesture(unsigned char state);

    void getPosition(double *firstX, double *firstY,
                     double *lastX, double *lastY);
    void getAverageMovement(double *h, double *v);
    void getAverageDistance(double *firstX, double *firstY,
                            double *lastX, double *lastY);

  private:
    struct GHTouch {
      struct timeval started;
      bool active;
      double firstX;
      double firstY;
      double lastX;
      double lastY;
      int angle;
    };

    unsigned char state;

    std::map<int, GHTouch> tracked;
    std::set<int> ignored;

    bool waitingRelease;
    struct timeval releaseStart;

    rfb::Timer longpressTimer;
    rfb::Timer twoTouchTimer;
};

#endif // __GESTUREHANDLER_H__
