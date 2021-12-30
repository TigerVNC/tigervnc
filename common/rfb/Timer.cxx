/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2016-2018 Pierre Ossman for Cendio AB
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

// -=- Timer.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/time.h>

#include <rfb/Timer.h>
#include <rfb/util.h>
#include <rfb/LogWriter.h>

using namespace rfb;

#ifndef __NO_DEFINE_VLOG__
static LogWriter vlog("Timer");
#endif


// Millisecond timeout processing helper functions

inline static timeval addMillis(timeval inTime, int millis) {
  int secs = millis / 1000;
  millis = millis % 1000;
  inTime.tv_sec += secs;
  inTime.tv_usec += millis * 1000;
  if (inTime.tv_usec >= 1000000) {
    inTime.tv_sec++;
    inTime.tv_usec -= 1000000;
  }
  return inTime;
}

inline static int diffTimeMillis(timeval later, timeval earlier) {
  return ((later.tv_sec - earlier.tv_sec) * 1000) + ((later.tv_usec - earlier.tv_usec) / 1000);
}

std::list<Timer*> Timer::pending;

int Timer::checkTimeouts() {
  timeval start;

  if (pending.empty())
    return 0;

  gettimeofday(&start, 0);
  while (pending.front()->isBefore(start)) {
    Timer* timer;
    timeval before;

    timer = pending.front();
    pending.pop_front();

    gettimeofday(&before, 0);
    if (timer->cb->handleTimeout(timer)) {
      timeval now;

      gettimeofday(&now, 0);

      timer->dueTime = addMillis(timer->dueTime, timer->timeoutMs);
      if (timer->isBefore(now)) {
        // Time has jumped forwards, or we're not getting enough
        // CPU time for the timers

        timer->dueTime = addMillis(before, timer->timeoutMs);
        if (timer->isBefore(now))
          timer->dueTime = now;
      }

      insertTimer(timer);
    } else if (pending.empty()) {
      return 0;
    }
  }
  return getNextTimeout();
}

int Timer::getNextTimeout() {
  timeval now;
  gettimeofday(&now, 0);
  int toWait = __rfbmax(1, pending.front()->getRemainingMs());
  if (toWait > pending.front()->timeoutMs) {
    if (toWait - pending.front()->timeoutMs < 1000) {
      vlog.info("gettimeofday is broken...");
      return toWait;
    }
    // Time has jumped backwards!
    vlog.info("time has moved backwards!");
    pending.front()->dueTime = now;
    toWait = 1;
  }
  return toWait;
}

void Timer::insertTimer(Timer* t) {
  std::list<Timer*>::iterator i;
  for (i=pending.begin(); i!=pending.end(); i++) {
    if (t->isBefore((*i)->dueTime)) {
      pending.insert(i, t);
      return;
    }
  }
  pending.push_back(t);
}

void Timer::start(int timeoutMs_) {
  timeval now;
  gettimeofday(&now, 0);
  stop();
  timeoutMs = timeoutMs_;
  // The rest of the code assumes non-zero timeout
  if (timeoutMs <= 0)
    timeoutMs = 1;
  dueTime = addMillis(now, timeoutMs);
  insertTimer(this);
}

void Timer::stop() {
  pending.remove(this);
}

bool Timer::isStarted() {
  std::list<Timer*>::iterator i;
  for (i=pending.begin(); i!=pending.end(); i++) {
    if (*i == this)
      return true;
  }
  return false;
}

int Timer::getTimeoutMs() {
  return timeoutMs;
}

int Timer::getRemainingMs() {
  timeval now;
  gettimeofday(&now, 0);
  return __rfbmax(0, diffTimeMillis(dueTime, now));
}

bool Timer::isBefore(timeval other) {
  return (dueTime.tv_sec < other.tv_sec) ||
    ((dueTime.tv_sec == other.tv_sec) &&
     (dueTime.tv_usec < other.tv_usec));
}
