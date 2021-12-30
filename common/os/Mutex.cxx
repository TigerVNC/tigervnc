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
#endif

#include <rdr/Exception.h>

#include <os/Mutex.h>

using namespace os;

Mutex::Mutex()
{
#ifdef WIN32
  systemMutex = new CRITICAL_SECTION;
  InitializeCriticalSection((CRITICAL_SECTION*)systemMutex);
#else
  int ret;

  systemMutex = new pthread_mutex_t;
  ret = pthread_mutex_init((pthread_mutex_t*)systemMutex, NULL);
  if (ret != 0)
    throw rdr::SystemException("Failed to create mutex", ret);
#endif
}

Mutex::~Mutex()
{
#ifdef WIN32
  DeleteCriticalSection((CRITICAL_SECTION*)systemMutex);
  delete (CRITICAL_SECTION*)systemMutex;
#else
  pthread_mutex_destroy((pthread_mutex_t*)systemMutex);
  delete (pthread_mutex_t*)systemMutex;
#endif
}

void Mutex::lock()
{
#ifdef WIN32
  EnterCriticalSection((CRITICAL_SECTION*)systemMutex);
#else
  int ret;

  ret = pthread_mutex_lock((pthread_mutex_t*)systemMutex);
  if (ret != 0)
    throw rdr::SystemException("Failed to lock mutex", ret);
#endif
}

void Mutex::unlock()
{
#ifdef WIN32
  LeaveCriticalSection((CRITICAL_SECTION*)systemMutex);
#else
  int ret;

  ret = pthread_mutex_unlock((pthread_mutex_t*)systemMutex);
  if (ret != 0)
    throw rdr::SystemException("Failed to unlock mutex", ret);
#endif
}

Condition::Condition(Mutex* mutex)
{
  this->mutex = mutex;

#ifdef WIN32
  systemCondition = new CONDITION_VARIABLE;
  InitializeConditionVariable((CONDITION_VARIABLE*)systemCondition);
#else
  int ret;

  systemCondition = new pthread_cond_t;
  ret = pthread_cond_init((pthread_cond_t*)systemCondition, NULL);
  if (ret != 0)
    throw rdr::SystemException("Failed to create condition variable", ret);
#endif
}

Condition::~Condition()
{
#ifdef WIN32
  delete (CONDITION_VARIABLE*)systemCondition;
#else
  pthread_cond_destroy((pthread_cond_t*)systemCondition);
  delete (pthread_cond_t*)systemCondition;
#endif
}

void Condition::wait()
{
#ifdef WIN32
  BOOL ret;

  ret = SleepConditionVariableCS((CONDITION_VARIABLE*)systemCondition,
                                 (CRITICAL_SECTION*)mutex->systemMutex,
                                 INFINITE);
  if (!ret)
    throw rdr::SystemException("Failed to wait on condition variable", GetLastError());
#else
  int ret;

  ret = pthread_cond_wait((pthread_cond_t*)systemCondition,
                          (pthread_mutex_t*)mutex->systemMutex);
  if (ret != 0)
    throw rdr::SystemException("Failed to wait on condition variable", ret);
#endif
}

void Condition::signal()
{
#ifdef WIN32
  WakeConditionVariable((CONDITION_VARIABLE*)systemCondition);
#else
  int ret;

  ret = pthread_cond_signal((pthread_cond_t*)systemCondition);
  if (ret != 0)
    throw rdr::SystemException("Failed to signal condition variable", ret);
#endif
}

void Condition::broadcast()
{
#ifdef WIN32
  WakeAllConditionVariable((CONDITION_VARIABLE*)systemCondition);
#else
  int ret;

  ret = pthread_cond_broadcast((pthread_cond_t*)systemCondition);
  if (ret != 0)
    throw rdr::SystemException("Failed to broadcast condition variable", ret);
#endif
}
