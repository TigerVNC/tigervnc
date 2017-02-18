/* Copyright 2015 Pierre Ossman for Cendio AB
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

#ifndef __OS_THREAD_H__
#define __OS_THREAD_H__

#include <stddef.h>

namespace os {
  class Mutex;

  class Thread {
  public:
    Thread();
    virtual ~Thread();

    void start();
    void wait();

    bool isRunning();

  public:
    static size_t getSystemCPUCount();

  protected:
    virtual void worker() = 0;

  private:
#ifdef WIN32
    static long unsigned __stdcall startRoutine(void* data);
#else
    static void* startRoutine(void* data);
#endif

  private:
    Mutex *mutex;
    bool running;

    void *threadId;
  };
}

#endif
