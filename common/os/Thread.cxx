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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif

#include <rdr/Exception.h>

#include <os/Mutex.h>
#include <os/Thread.h>

using namespace os;

Thread::Thread() : running(false), threadId(NULL)
{
  mutex = new Mutex;

#ifdef WIN32
  threadId = new HANDLE;
#else
  threadId = new pthread_t;
#endif
}

Thread::~Thread()
{
#ifdef WIN32
  delete (HANDLE*)threadId;
#else
  if (isRunning())
    pthread_cancel(*(pthread_t*)threadId);
  delete (pthread_t*)threadId;
#endif

  delete mutex;
}

void Thread::start()
{
  AutoMutex a(mutex);

#ifdef WIN32
  *(HANDLE*)threadId = CreateThread(NULL, 0, startRoutine, this, 0, NULL);
  if (*(HANDLE*)threadId == NULL)
    throw rdr::SystemException("Failed to create thread", GetLastError());
#else
  int ret;
  sigset_t all, old;

  // Creating threads from libraries is a bit evil, so mitigate the
  // issue by at least avoiding signals on these threads
  sigfillset(&all);
  ret = pthread_sigmask(SIG_SETMASK, &all, &old);
  if (ret != 0)
    throw rdr::SystemException("Failed to mask signals", ret);

  ret = pthread_create((pthread_t*)threadId, NULL, startRoutine, this);

  pthread_sigmask(SIG_SETMASK, &old, NULL);

  if (ret != 0)
    throw rdr::SystemException("Failed to create thread", ret);
#endif

  running = true;
}

void Thread::wait()
{
  if (!isRunning())
    return;

#ifdef WIN32
  DWORD ret;

  ret = WaitForSingleObject(*(HANDLE*)threadId, INFINITE);
  if (ret != WAIT_OBJECT_0)
    throw rdr::SystemException("Failed to join thread", GetLastError());
#else
  int ret;

  ret = pthread_join(*(pthread_t*)threadId, NULL);
  if (ret != 0)
    throw rdr::SystemException("Failed to join thread", ret);
#endif
}

bool Thread::isRunning()
{
  AutoMutex a(mutex);

  return running;
}

size_t Thread::getSystemCPUCount()
{
#ifdef WIN32
  SYSTEM_INFO si;
  size_t count;
  DWORD mask;

  GetSystemInfo(&si);

  count = 0;
  for (mask = si.dwActiveProcessorMask;mask != 0;mask >>= 1) {
    if (mask & 0x1)
      count++;
  }

  if (count > si.dwNumberOfProcessors)
    count = si.dwNumberOfProcessors;

  return count;
#else
  long ret;

  ret = sysconf(_SC_NPROCESSORS_ONLN);
  if (ret == -1)
    return 0;

  return ret;
#endif
}

#ifdef WIN32
long unsigned __stdcall Thread::startRoutine(void* data)
#else
void* Thread::startRoutine(void* data)
#endif
{
  Thread *self;

  self = (Thread*)data;

  try {
    self->worker();
  } catch(...) {
  }

  self->mutex->lock();
  self->running = false;
  self->mutex->unlock();

  return 0;
}
