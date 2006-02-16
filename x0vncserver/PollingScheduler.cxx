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
// PollingScheduler class implementation.
//

#include <x0vncserver/PollingScheduler.h>

PollingScheduler::PollingScheduler(int interval)
{
  setInterval(interval);
  reset();
}

void PollingScheduler::setInterval(int interval)
{
  m_interval = interval;
}

void PollingScheduler::reset()
{
  initialState = true;
}

void PollingScheduler::newPass()
{
  m_passStarted.update();
  initialState = false;
}

int PollingScheduler::millisRemaining() const
{
  if (initialState)
    return 0;

  TimeMillis timeNow;
  int elapsed = timeNow.diffFrom(m_passStarted);

  if (elapsed > m_interval)
    return 0;

  return (m_interval - elapsed);
}

bool PollingScheduler::goodTimeToPoll() const
{
  return (millisRemaining() == 0);
}
