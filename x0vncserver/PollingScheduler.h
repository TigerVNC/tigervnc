/* Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
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

//
// PollingScheduler class. It is used for deciding when to start new
// polling pass, and how much time it is ok to wait before starting. 
// PollingScheduler is provided a desired polling interval and watches
// for actual intervals between past polling cycles.
//

#ifndef __POLLINGSCHEDULER_H__
#define __POLLINGSCHEDULER_H__

#include <x0vncserver/TimeMillis.h>

class PollingScheduler {

public:

  PollingScheduler(int interval);

  // Set desired polling interval.
  void setInterval(int interval);

  // Reset the object into the initial state (no polling performed).
  void reset();

  // Tell the scheduler that new polling pass is just being started.
  void newPass();

  // This function estimates time remaining before new polling pass.
  int millisRemaining() const;

  // This function tells if it's ok to start polling pass right now.
  bool goodTimeToPoll() const;

protected:

  bool initialState;
  int m_interval;
  TimeMillis m_passStarted;

};

#endif // __POLLINGSCHEDULER_H__

