/* Copyright (C) 2005 Constantin Kaplinsky.  All Rights Reserved.
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
// CPUMonitor.cxx
//

#include "CPUMonitor.h"

CPUMonitor::CPUMonitor(int optimalLevel, int updatePeriod)
{
  setLevel(optimalLevel);
  setPeriod(updatePeriod);
  update();
}

void CPUMonitor::setLevel(int optimalLevel)
{
  m_optimalLevel = optimalLevel;
  if (m_optimalLevel < 0) {
    m_optimalLevel = 0;
  } else if (m_optimalLevel > 100) {
    m_optimalLevel = 100;
  }
}

void CPUMonitor::setPeriod(int updatePeriod)
{
  m_updatePeriod = (updatePeriod > 10) ? updatePeriod : 10;
}

void CPUMonitor::update()
{
  getClock(&m_savedTime, &m_savedClock);
}

int CPUMonitor::check()
{
  struct timeval timeNow;
  clock_t clockNow;
  getClock(&timeNow, &clockNow);

  int coeff = 100;              // 100% means no changes.

  if (m_savedClock != (clock_t)-1 && clockNow != (clock_t)-1) {

    // Find out how much real time has been elapsed (in milliseconds).
    int timeDiff = (int)((timeNow.tv_usec - m_savedTime.tv_usec + 500) / 1000 +
                         (timeNow.tv_sec - m_savedTime.tv_sec) * 1000);
    if (timeDiff < m_updatePeriod) {
      // Measuring CPU usage is problematic in this case. So return
      // 100 and do not update saved time and clock numbers.
      return coeff;
    }

    // Calculate how much processor time has been consumed (in milliseconds).
    unsigned int clockDiff = (unsigned int)(clockNow - m_savedClock);
    clockDiff /= (CLOCKS_PER_SEC / 1000);

    // Get actual CPU usage and convert optimal level (to 1/1000).
    int realUsage = (clockDiff * 1000 + timeDiff/2) / timeDiff;
    int optimalUsage = m_optimalLevel * 10;

    // Compute correction coefficient (in percents).
    if (realUsage != 0) {
      coeff = (optimalUsage * 100 + realUsage/2) / realUsage;
    } else {
      coeff = 10000;
    }

  }

  // Update saved time and clock numbers.
  m_savedTime = timeNow;
  m_savedClock = clockNow;

  return coeff;
}

void CPUMonitor::getClock(struct timeval *tv, clock_t *clk)
{
  if (gettimeofday(tv, NULL) != 0) {
    *clk = (clock_t)-1;
  } else {
    *clk = clock();
  }
}
