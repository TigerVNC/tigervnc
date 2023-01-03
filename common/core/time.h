/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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

//
// time.h - time helper functions
//

#ifndef __CORE_TIME_H__
#define __CORE_TIME_H__

#include <limits.h>

struct timeval;

namespace core {

  // secsToMillis() turns seconds into milliseconds, capping the value so it
  //   can't wrap round and become -ve
  inline int secsToMillis(int secs) {
    return (secs < 0 || secs > (INT_MAX/1000) ? INT_MAX : secs * 1000);
  }

  // Returns time elapsed between two moments in milliseconds.
  unsigned msBetween(const struct timeval *first,
                     const struct timeval *second);

  // Returns time elapsed since given moment in milliseconds.
  unsigned msSince(const struct timeval *then);

  // Returns time until the given moment in milliseconds.
  unsigned msUntil(const struct timeval *then);

  // Returns true if first happened before seconds
  bool isBefore(const struct timeval *first,
                const struct timeval *second);

  // Returns a new timeval a specified number of milliseconds later than
  // the given timeval
  struct timeval addMillis(struct timeval inTime, int millis);
}

#endif
