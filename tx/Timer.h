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
#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/time.h>
#include <unistd.h>

class TimerCallback;

class Timer {
public:
  static void callTimers();
  static bool getTimeout(struct timeval* timeout);

  Timer(TimerCallback* cb_);
  ~Timer();
  void reset(int ms);
  void cancel();
  bool isSet();
  bool before(struct timeval other) {
    return (tv.tv_sec < other.tv_sec ||
            (tv.tv_sec == other.tv_sec && tv.tv_usec < other.tv_usec));
  }
private:
  struct timeval tv;
  TimerCallback* cb;
  Timer* next;
};

class TimerCallback {
public:
  virtual void timerCallback(Timer* timer) = 0;
};

#endif
