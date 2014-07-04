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

#include <stdint.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/resource.h>
#endif

#ifdef WIN32
static FILETIME cpuCounters[2];
#else
struct rusage cpuCounters[2];
#endif

static void measureCpu(void *counter)
{
#ifdef WIN32
  FILETIME dummy1, dummy2, dummy3;

  GetProcessTimes(GetCurrentProcess(), &dummy1, &dummy2,
                  &dummy3, (FILETIME*)counter);
#else
  getrusage(RUSAGE_SELF, (struct rusage*)counter);
#endif
}

void startCpuCounter(void)
{
  measureCpu(&cpuCounters[0]);
}

void endCpuCounter(void)
{
  measureCpu(&cpuCounters[1]);
}

double getCpuCounter(void)
{
  double seconds;

#ifdef WIN32
  uint64_t counters[2];

  counters[0] = (uint64_t)cpuCounters[0].dwHighDateTime << 32 |
                cpuCounters[0].dwLowDateTime;
  counters[1] = (uint64_t)cpuCounters[1].dwHighDateTime << 32 |
                cpuCounters[1].dwLowDateTime;

  seconds = (double)(counters[1] - counters[2]) / 10000000.0;
#else
  seconds = (double)(cpuCounters[1].ru_utime.tv_sec -
                     cpuCounters[0].ru_utime.tv_sec);
  seconds += (double)(cpuCounters[1].ru_utime.tv_usec -
                      cpuCounters[0].ru_utime.tv_usec) / 1000000.0;
#endif

  return seconds;
}
