/* Copyright 2026 jose-pr
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

#ifndef __MSVC_SYS_TIME_H__
#define __MSVC_SYS_TIME_H__

#include <winsock2.h>

// FILETIME counts 100 ns intervals from 1601-01-01; the Unix epoch is
// this many of them later.
#define __MSVC_EPOCH_DIFF 116444736000000000ULL

static inline int gettimeofday(struct timeval* tv, void* tz)
{
  FILETIME ft;
  unsigned long long t;

  (void)tz;

  GetSystemTimeAsFileTime(&ft);

  t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  t = (t - __MSVC_EPOCH_DIFF) / 10;

  tv->tv_sec = (long)(t / 1000000ULL);
  tv->tv_usec = (long)(t % 1000000ULL);

  return 0;
}

#endif
