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

// -=- Threading_win32.h
// Win32 Threading interface implementation

#ifndef __RFB_THREADING_IMPL_WIN32
#define __RFB_THREADING_IMPL_WIN32

#define __RFB_THREADING_IMPL WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

namespace rfb {

  class Mutex {
  public:
    Mutex() {
      InitializeCriticalSection(&crit);
    }
    ~Mutex() {
      DeleteCriticalSection(&crit);
    }
    friend class Lock;
    friend class Condition;
  protected:
    void enter() {EnterCriticalSection(&crit);}
    void exit() {LeaveCriticalSection(&crit);}
    CRITICAL_SECTION crit;
  };

  class Lock {
  public:
    Lock(Mutex& m) : mutex(m) {m.enter();}
    ~Lock() {mutex.exit();}
  protected:
    Mutex& mutex;
  };

  enum ThreadState {ThreadCreated, ThreadStarted, ThreadStopped, ThreadNative};

  class Thread {
  public:
    Thread(const char* name_=0);
    virtual ~Thread();

    virtual void run();

    virtual void start();
    virtual Thread* join();

    const char* getName() const;
    ThreadState getState() const;

    // Determines whether the thread should delete itself when run() returns
    // If you set this, you must NEVER call join()!
    void setDeleteAfterRun() {deleteAfterRun = true;};

    unsigned long getThreadId() const;

    static Thread* self();

    friend class Condition;

  protected:
    Thread(HANDLE thread_, DWORD thread_id_);
    static DWORD WINAPI threadProc(LPVOID lpParameter);

    HANDLE thread;
    DWORD thread_id;
    char* name;
    ThreadState state;
    Condition* sig;
    Mutex mutex;

    HANDLE cond_event;
	  Thread* cond_next;

    bool deleteAfterRun;
  };

  class Condition {
  public:
    Condition(Mutex& m) : mutex(m), waiting(0) {
    }
    ~Condition() {
    }
    void signal() {
      Lock l(cond_lock);
      if (waiting) {
        SetEvent(waiting->cond_event);
        waiting = waiting->cond_next;
      }
    }
    // - MUST hold "mutex" to call wait()
    // WIN32: if processMsg is true then wait will continue
    // to process messages in the thread's queue.
    // Avoid using this unless you have to!
    void wait(bool processMsgs=false) {
      Thread* self = Thread::self();
      ResetEvent(self->cond_event);
      { Lock l(cond_lock);
        self->cond_next = waiting;
        waiting = self;
      }
      mutex.exit();
      if (processMsgs) {
        while (1) {
          DWORD result = MsgWaitForMultipleObjects(1, &self->cond_event, FALSE, INFINITE, QS_ALLINPUT);
          if (result == WAIT_OBJECT_0)
            break;
          MSG msg;
          while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            DispatchMessage(&msg);
          }
        }
      } else {
        WaitForSingleObject(self->cond_event, INFINITE);
      }
      mutex.enter();
    }
    
  protected:
    Mutex& mutex;
    Mutex cond_lock;
	  Thread* waiting;
  };

};

#endif // __RFB_THREADING_IMPL
