/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2016-2024 Pierre Ossman for Cendio AB
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

#include <algorithm>

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
  long udiff;
  udiff = ((later.tv_sec - earlier.tv_sec) * 1000000) +
          (later.tv_usec - earlier.tv_usec);
  return (udiff + 999) / 1000;
}

std::list<Timer*> Timer::pending;

int Timer::checkTimeouts() {
  timeval start;

  if (pending.empty())
    return -1;

  gettimeofday(&start, nullptr);
  while (pending.front()->isBefore(start)) {
    Timer* timer;

    timer = pending.front();
    pending.pop_front();

    timer->lastDueTime = timer->dueTime;
    timer->cb->handleTimeout(timer);

    if (pending.empty())
      return -1;
  }
  return getNextTimeout();
}

int Timer::getNextTimeout() {
  timeval now;
  gettimeofday(&now, nullptr);

  if (pending.empty())
    return -1;

  int toWait = pending.front()->getRemainingMs();

  if (toWait > pending.front()->timeoutMs) {
    if (toWait - pending.front()->timeoutMs < 1000) {
      vlog.info("gettimeofday is broken...");
      return toWait;
    }
    // Time has jumped backwards!
    vlog.info("time has moved backwards!");
    pending.front()->dueTime = now;
    toWait = 0;
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
  gettimeofday(&now, nullptr);
  stop();
  timeoutMs = timeoutMs_;
  dueTime = addMillis(now, timeoutMs);
  insertTimer(this);
}

void Timer::repeat(int timeoutMs_) {
  timeval now;

  gettimeofday(&now, nullptr);

  if (isStarted()) {
    vlog.error("Incorrectly repeating already running timer");
    stop();
  }

  if (msBetween(&lastDueTime, &dueTime) != 0)
    vlog.error("Timer incorrectly modified whilst repeating");

  if (timeoutMs_ != -1)
    timeoutMs = timeoutMs_;

  dueTime = addMillis(lastDueTime, timeoutMs);
  if (isBefore(now)) {
    // Time has jumped forwards, or we're not getting enough
    // CPU time for the timers
    dueTime = now;
  }

  insertTimer(this);
}

void Timer::stop() {
  pending.remove(this);
}

bool Timer::isStarted() {
  return std::find(pending.begin(), pending.end(),
                   this) != pending.end();
}

int Timer::getTimeoutMs() {
  return timeoutMs;
}

int Timer::getRemainingMs() {
  timeval now;
  gettimeofday(&now, nullptr);
  return __rfbmax(0, diffTimeMillis(dueTime, now));
}

bool Timer::isBefore(timeval other) {
  return (dueTime.tv_sec < other.tv_sec) ||
    ((dueTime.tv_sec == other.tv_sec) &&
     (dueTime.tv_usec < other.tv_usec));
}
