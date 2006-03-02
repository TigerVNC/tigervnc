/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
// Timer.cxx
//

// XXX Lynx/OS 2.3: get proto for gettimeofday()
#ifdef Lynx
#include <sys/proto.h>
#endif

#include "Timer.h"

static Timer* timers;

void Timer::callTimers()
{
  struct timeval now;
  gettimeofday(&now, 0);
  while (timers) {
    Timer* timer = timers;
    if (timer->before(now)) {
      timers = timers->next;
      timer->cb->timerCallback(timer);
    } else {
      break;
    }
  }
}

bool Timer::getTimeout(struct timeval* timeout)
{
  if (!timers) return false;
  Timer* timer = timers;
  struct timeval now;
  gettimeofday(&now, 0);
  if (timer->before(now)) {
    timeout->tv_sec = 0;
    timeout->tv_usec = 0;
    return true;
  }
  timeout->tv_sec = timer->tv.tv_sec - now.tv_sec;
  if (timer->tv.tv_usec < now.tv_usec) {
    timeout->tv_usec = timer->tv.tv_usec + 1000000 - now.tv_usec;
    timeout->tv_sec--;
  } else {
    timeout->tv_usec = timer->tv.tv_usec - now.tv_usec;
  }
  return true;
}

Timer::Timer(TimerCallback* cb_) : cb(cb_) {}
Timer::~Timer()
{
  cancel();
}

void Timer::reset(int ms) {
  cancel();
  gettimeofday(&tv, 0);
  tv.tv_sec += ms / 1000;
  tv.tv_usec += (ms % 1000) * 1000;
  if (tv.tv_usec > 1000000) {
    tv.tv_sec += 1;
    tv.tv_usec -= 1000000;
  }

  Timer** timerPtr;
  for (timerPtr = &timers; *timerPtr; timerPtr = &(*timerPtr)->next) {
    if (before((*timerPtr)->tv)) {
      next = *timerPtr;
      *timerPtr = this;
      break;
    }
  }
  if (!*timerPtr) {
    next = 0;
    *timerPtr = this;
  }
}

void Timer::cancel() {
  for (Timer** timerPtr = &timers; *timerPtr; timerPtr = &(*timerPtr)->next) {
    if (*timerPtr == this) {
      *timerPtr = (*timerPtr)->next;
      break;
    }
  }
}

bool Timer::isSet() {
  for (Timer* timer = timers; timer; timer = timer->next)
    if (timer == this) return true;
  return false;
}
