/* Copyright 2019 Aaron Sowry for Cendio AB
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

#include <vector>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

// Internal state bitmasks
#define GH_NOGESTURE   0
#define GH_LEFTBTN     1
#define GH_MIDDLEBTN   2
#define GH_RIGHTBTN    4
#define GH_SCROLL      8
#define GH_ZOOM        16
#define GH_UNDEFINED   (32 | 64 | 128)

#define GH_INITSTATE   (255 & ~GH_UNDEFINED)

// Timeout when waiting for gestures
#define GH_TOUCHDELAY  0.25 // s

// Movement threshold for clicks
#define GH_CLCKTHRESHOLD 10

// Invert the scroll
#define GH_INVERTSCR   1

enum GHEventType {
  GH_GestureBegin,
  GH_GestureEnd,
  GH_GestureUpdate,
};

struct GHEvent {
  int detail;
  double event_x;
  double event_y;
  GHEventType type;
};

struct GHTouch {
  int id;
  double first_x;
  double first_y;
  double last_x;
  double last_y;
};

class GestureHandler {
  public:
    GestureHandler();
    ~GestureHandler();

   int registerEvent(void *ev);
   unsigned char getState();
   bool hasState(unsigned char state);

   std::vector<GHEvent> getEventQueue();
   void clearEventQueue();
   int expireEvents();

  private:
    unsigned char state;

    std::vector<GHTouch> tracked;
    std::vector<GHEvent> eventQueue;

    int updateTouch(XIDeviceEvent ev, GHEvent *ghev);
    int trackTouch(XIDeviceEvent ev);
    int isTracked(XIDeviceEvent ev);

    size_t avgTrackedTouches(double *x, double *y, GHEventType t);

    int relativeDistanceMoved(int t0_x, int t0_y,
		              int t1_x, int t1_y,
			      int t2_x, int t2_y);
};

#endif // __GESTUREHANDLER_H__
