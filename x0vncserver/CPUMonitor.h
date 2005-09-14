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
// CPUMonitor.h
//

#ifndef __CPUMONITOR_H__
#define __CPUMONITOR_H__

#include <sys/time.h>
#include <time.h>

class CPUMonitor {

public:

  CPUMonitor(int optimalLevel = 50, int updatePeriod = 1000);

  //
  // Optimal level is the CPU utilization level to maintain, in
  // percents.
  //
  void setLevel(int optimalLevel);

  //
  // Update period is the minimum time in milliseconds that has to
  // pass after update() or check() call, before next check() call
  // will return meaningful value.
  //
  void setPeriod(int updatePeriod);

  //
  // Save current time and CPU clock, to use in the next check() call.
  //
  void update();

  //
  // This method calculates recent CPU utilization and returns a
  // percentage factor which would convert current CPU usage into the
  // optimal value. For example, if the optimal level was set to 40%
  // and the real value was 50%, the function will return 80, because
  // 50 * 80% == 40.
  //
  // If the CPU utilization cannot be measured, this function returns
  // 100. This may be the case on the first call to check(), or if the
  // time period between two successive check() calls was too small.
  //
  int check();

protected:

  static void getClock(struct timeval *tv, clock_t *clk);

  int m_optimalLevel;
  int m_updatePeriod;
  struct timeval m_savedTime;
  clock_t m_savedClock;

};

#endif // __CPUMONITOR_H__
