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

#include <cmath>

#include <rfb/LogWriter.h>

#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>

#include "GestureHandler.h"

static rfb::LogWriter vlog("GestureHandler");

GestureHandler::GestureHandler() : state(GH_INITSTATE), tracked(), eventQueue() {
}

int GestureHandler::registerEvent(void *ev) {
#if !defined(WIN32) && !defined(__APPLE__)
  XIDeviceEvent *devev = (XIDeviceEvent*)ev;
  GHEvent ghev;

  switch (devev->evtype) {
    case XI_TouchBegin:
      if (!hasState(this->state))
        trackTouch(*devev);
      break;

    case XI_TouchUpdate:
      if (updateTouch(*devev, &ghev) >= 0)
        eventQueue.push_back(ghev);
      break;

    case XI_TouchEnd:
      if (isTracked(*devev) < 0)
        return 0;

      if (!hasState(this->state))
        expireEvents();

      // FIXME: By this point we should have a state, right?
      //assert(hasState(this->state));

      switch (this->state) {
        case GH_LEFTBTN:
        case GH_RIGHTBTN:
	case GH_MIDDLEBTN:
	case GH_SCROLL:
	case GH_ZOOM:
          ghev.detail = this->state;
          ghev.event_x = devev->event_x;
          ghev.event_y = devev->event_y;
          ghev.type = GH_GestureEnd;
          eventQueue.push_back(ghev);
	  break;
      }

      // Ending a tracked touch also ends the associated gesture
      this->state = GH_INITSTATE;
      tracked.clear();

      break;
  }
#endif
  return tracked.size();
}


int GestureHandler::expireEvents() {
  GHEvent ghev;

  if (!hasState(this->state)) {
    // Scroll and zoom are no longer valid states
    this->state &= ~(GH_SCROLL | GH_ZOOM);

    switch (tracked.size()) {
      case 1:
        // Can't possibly be a multi-touch event
        this->state &= ~(GH_MIDDLEBTN | GH_RIGHTBTN);
	break;

      case 2:
	// Can't possibly be a single- or triple-touch gesture
        this->state &= ~(GH_LEFTBTN | GH_MIDDLEBTN);
	break;

      case 3:
        // Can't possibly be a single- or double-touch gesture
        this->state &= ~(GH_LEFTBTN | GH_RIGHTBTN);
	break;

      default:
        this->state = GH_NOGESTURE;
    }
  }

  if (hasState(this->state)) {
    ghev.detail = this->state;
    ghev.event_x = tracked[0].first_x;
    ghev.event_y = tracked[0].first_y;
    ghev.type = GH_GestureBegin;
    eventQueue.push_back(ghev);
  }
  else {
    this->state = GH_INITSTATE;
    tracked.clear();
  }

  return this->state;
}

unsigned char GestureHandler::getState() {
  return this->state;
}

bool GestureHandler::hasState(unsigned char state) {
  // Invalid state if any of the undefined bits are set
  if ((state & GH_UNDEFINED) != 0) {
    return False;
  }

  // Check to see if the bitmask value is a power of 2
  // (i.e. only one bit set). If it is, we have a state.
  return state && !(state & (state - 1));
}

int GestureHandler::isTracked(XIDeviceEvent ev) {
  for (size_t i = 0; i < tracked.size(); i++) {
    if (tracked[i].id == ev.detail)
      return (int)i;
  }

  return -1;
}

int GestureHandler::trackTouch(XIDeviceEvent ev) {
  GHTouch ght;

  // FIXME: Perhaps implement some sanity checks here,
  // e.g. duplicate IDs etc

  ght.id = ev.detail;
  ght.first_x = ev.event_x;
  ght.first_y = ev.event_y;
  ght.last_x = ght.first_x;
  ght.last_y = ght.first_y;

  tracked.push_back(ght);

  return tracked.size();
}

std::vector<GHEvent> GestureHandler::getEventQueue() {
  return eventQueue;
}

void GestureHandler::clearEventQueue() {
  eventQueue.clear();
}

int GestureHandler::relativeDistanceMoved(int t0_x, int t0_y,
		                          int t1_x, int t1_y,
					  int t2_x, int t2_y)
{
  // Calculate the distance between t0 and t1
  int dx_t0 = t1_x - t0_x;
  int dy_t0 = t1_y - t0_y;
  int dt0 = std::sqrt(dx_t0 * dx_t0 + dy_t0 * dy_t0);

  // Calculate the distance between t0 and t2
  int dx_t1 = t2_x - t0_x;
  int dy_t1 = t2_y - t0_y;
  int dt1 = std::sqrt(dx_t1 * dx_t1 + dy_t1 * dy_t1);

  // Calculate how much the distance between the two touches has changed
  return dt1 - dt0;
}

int GestureHandler::updateTouch(XIDeviceEvent ev, GHEvent *ghev) {
  int detail, event_x, event_y;
  int idx = isTracked(ev);

  if (idx >= 0) {
    // This touch is being tracked...
    if (!hasState(this->state)) {
      // ...and we're not in a gesture yet...
      if (tracked.size() == 2) {
        // ...but we are tracking two touches, so this movement
	// could trigger a transition to zoom or scroll.

        // The distance this touch has moved on the axes since inception
        int dx = std::abs(tracked[idx].first_x - ev.event_x);
        int dy = std::abs(tracked[idx].first_y - ev.event_y);

	int dt = std::abs(relativeDistanceMoved(tracked[1-idx].first_x,
				                tracked[1-idx].first_y,
						tracked[idx].first_x,
						tracked[idx].first_y,
						ev.event_x, ev.event_y));

       /*
	* - If the finger has moved more than GH_CLCKTHRESHOLD, then we can
	*   eliminate the possibility of a click.
	*
	* - If the finger has moved along the y axis _more_ than what it has
	*   relative to the other finger, then we're not looking at a zoom.
	*
	* - If the finger has moved relative to the other finger _more_ than
	*   what it has along the y axis, then we're not looking at a scroll.
	*/

        if (dx > GH_CLCKTHRESHOLD ||
	    dy > GH_CLCKTHRESHOLD ||
	    dt > GH_CLCKTHRESHOLD)
	{
          this->state &= ~(GH_LEFTBTN | GH_MIDDLEBTN | GH_RIGHTBTN);

          if (dt > dy)
            this->state &= ~GH_SCROLL;
          else if (dy > dt)
            this->state &= ~GH_ZOOM;
        }
      }

      detail = GH_NOGESTURE;
      event_x = ev.event_x;
      event_y = ev.event_y;
    }
    else {
      // We got an update for a gesture which is being tracked
      switch (this->state) {
        case GH_SCROLL:
	  //assert(tracked.size() == 2);
	  detail = GH_INVERTSCR ? -(tracked[idx].last_y - ev.event_y)
		                :   tracked[idx].last_y - ev.event_y;
	  event_x = tracked[0].first_x;
	  event_y = tracked[0].first_y;
	  break;

	case GH_ZOOM:
	  //assert(tracked.size() == 2);
	  detail = relativeDistanceMoved(tracked[1-idx].first_x,
			                 tracked[1-idx].first_y,
					 tracked[idx].last_x,
					 tracked[idx].last_y,
					 ev.event_x, ev.event_y);
	  event_x = tracked[0].first_x;
	  event_y = tracked[0].first_y;
	  break;

	default:
	  detail = this->state;
	  event_x = ev.event_x;
	  event_y = ev.event_y;
      }
    }

    // Update the coordinates for the tracked touch
    tracked[idx].last_x = ev.event_x;
    tracked[idx].last_y = ev.event_y;
  }
  else {
    // We got an update for a touch which isn't being tracked
    detail = this->state;
    event_x = ev.event_x;
    event_y = ev.event_y;
  }

  ghev->detail = detail;
  ghev->event_x = event_x;
  ghev->event_y = event_y;
  ghev->type = GH_GestureUpdate;

  return idx;
}
