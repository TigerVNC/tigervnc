/* Copyright 2013-2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include "util.h"

#ifdef WIN32
typedef struct {
  FILETIME kernelTime;
  FILETIME userTime;
} syscounter_t;
#else
typedef struct rusage syscounter_t;
#endif

static syscounter_t _globalCounter[2];
static cpucounter_t globalCounter = _globalCounter;

void startCpuCounter(void)
{
  startCpuCounter(globalCounter);
}

void endCpuCounter(void)
{
  endCpuCounter(globalCounter);
}

double getCpuCounter(void)
{
  return getCpuCounter(globalCounter);
}

cpucounter_t newCpuCounter(void)
{
  syscounter_t *c;

  c = (syscounter_t*)malloc(sizeof(syscounter_t) * 2);
  if (c == NULL)
    return NULL;

  memset(c, 0, sizeof(syscounter_t) * 2);

  return c;
}

void freeCpuCounter(cpucounter_t c)
{
  free(c);
}

static void measureCpu(syscounter_t *counter)
{
#ifdef WIN32
  FILETIME dummy1, dummy2;

  GetProcessTimes(GetCurrentProcess(), &dummy1, &dummy2,
                  &counter->kernelTime, &counter->userTime);
#else
  getrusage(RUSAGE_SELF, counter);
#endif
}

void startCpuCounter(cpucounter_t c)
{
  syscounter_t *s = (syscounter_t*)c;
  measureCpu(&s[0]);
}

void endCpuCounter(cpucounter_t c)
{
  syscounter_t *s = (syscounter_t*)c;
  measureCpu(&s[1]);
}

double getCpuCounter(cpucounter_t c)
{
  syscounter_t *s = (syscounter_t*)c;
  double sysSeconds, userSeconds;

#ifdef WIN32
  uint64_t counters[2];

  counters[0] = (uint64_t)s[0].kernelTime.dwHighDateTime << 32 |
                s[0].kernelTime.dwLowDateTime;
  counters[1] = (uint64_t)s[1].kernelTime.dwHighDateTime << 32 |
                s[1].kernelTime.dwLowDateTime;

  sysSeconds = (double)(counters[1] - counters[0]) / 10000000.0;

  counters[0] = (uint64_t)s[0].userTime.dwHighDateTime << 32 |
                s[0].userTime.dwLowDateTime;
  counters[1] = (uint64_t)s[1].userTime.dwHighDateTime << 32 |
                s[1].userTime.dwLowDateTime;

  userSeconds = (double)(counters[1] - counters[0]) / 10000000.0;
#else
  sysSeconds = (double)(s[1].ru_stime.tv_sec -
                        s[0].ru_stime.tv_sec);
  sysSeconds += (double)(s[1].ru_stime.tv_usec -
                         s[0].ru_stime.tv_usec) / 1000000.0;

  userSeconds = (double)(s[1].ru_utime.tv_sec -
                         s[0].ru_utime.tv_sec);
  userSeconds += (double)(s[1].ru_utime.tv_usec -
                          s[0].ru_utime.tv_usec) / 1000000.0;
#endif

  return sysSeconds + userSeconds;
}

#ifdef WIN32
static LARGE_INTEGER timeStart, timeEnd;
#else
static struct timeval timeStart, timeEnd;
#endif

void startTimeCounter(void)
{
#ifdef WIN32
  QueryPerformanceCounter(&timeStart);
#else
  gettimeofday(&timeStart, NULL);
#endif
}

void endTimeCounter(void)
{
#ifdef WIN32
  QueryPerformanceCounter(&timeEnd);
#else
  gettimeofday(&timeEnd, NULL);
#endif
}

double getTimeCounter(void)
{
  double time;

#ifdef WIN32
  LARGE_INTEGER freq;

  QueryPerformanceFrequency(&freq);

  time = timeEnd.QuadPart - timeStart.QuadPart;
  time = time / freq.QuadPart;
#else
  time = (double)timeEnd.tv_sec - timeStart.tv_sec;
  time += (double)(timeEnd.tv_usec - timeStart.tv_usec) / 1000000.0;
#endif

  return time;
}
