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
// polling pass, and how much time it is ok to sleep before starting. 
// PollingScheduler is given a desired polling interval, but it can
// add time between polling passes if needed for satisfying processor
// usage limitation.
//

#ifndef __POLLINGSCHEDULER_H__
#define __POLLINGSCHEDULER_H__

#include <x0vncserver/TimeMillis.h>

class PollingScheduler {

public:

  PollingScheduler(int interval, int maxload = 50);

  // Set polling parameters.
  void setParameters(int interval, int maxload = 50);

  // Reset the object into the initial state (no polling performed).
  void reset();

  // Check if the object is active (not in the initial state).
  bool isRunning();

  // Tell the scheduler that new polling pass is just being started.
  void newPass();

  // Inform the scheduler about times when we sleep.
  void sleepStarted();
  void sleepFinished();

  // This function estimates time remaining before new polling pass.
  int millisRemaining() const;

  // This function tells if it's ok to start polling pass right now.
  bool goodTimeToPoll() const;

protected:

  // Parameters.
  int m_interval;
  int m_maxload;

  // This boolean flag is true when we do not poll the screen.
  bool m_initialState;

  // Time stamp saved on starting current polling pass.
  TimeMillis m_passStarted;

  // Desired duration of current polling pass.
  int m_ratedDuration;

  // These are for measuring sleep time in current pass.
  TimeMillis m_sleepStarted;
  bool m_sleeping;
  int m_sleptThisPass;

  // Ring buffer for tracking past timing errors.
  int m_errors[8];
  int m_errorSum;
  int m_errorAbsSum;

  // Ring buffer for tracking total pass durations (work + sleep).
  int m_durations[8];
  int m_durationSum;

  // Ring buffer for tracking past sleep times.
  int m_slept[8];
  int m_sleptSum;

  // Indexer for all ring buffers.
  int m_idx;

  // Pass counter.
  int m_count;
};

#endif // __POLLINGSCHEDULER_H__

