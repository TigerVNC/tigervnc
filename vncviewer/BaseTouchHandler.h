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

#ifndef __BASETOUCHHANDLER_H__
#define __BASETOUCHHANDLER_H__

#include "GestureEvent.h"

class BaseTouchHandler {
  public:
    virtual ~BaseTouchHandler();

  protected:
    BaseTouchHandler();

  protected:
    virtual void fakeMotionEvent(const GestureEvent origEvent) = 0;
    virtual void fakeButtonEvent(bool press, int button,
                                 const GestureEvent origEvent) = 0;
    virtual void fakeKeyEvent(bool press, int keycode,
                              const GestureEvent origEvent) = 0;

    virtual void handleGestureEvent(const GestureEvent& event);

  private:
    void handleTapEvent(const GestureEvent& ev, int buttonEvent);

    double lastMagnitudeX;
    double lastMagnitudeY;

    GestureEvent firstDoubleTapEvent;
    struct timeval lastTapTime;
};

#endif
