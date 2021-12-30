/* Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
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
// TimeMillis.cxx
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <x0vncserver/TimeMillis.h>

TimeMillis::TimeMillis()
{
  update();
}

bool TimeMillis::update()
{
  struct timezone tz;
  return (gettimeofday(&m_timeval, &tz) == 0);
}

int TimeMillis::diffFrom(const TimeMillis &older) const
{
  int diff = (int)
    ((m_timeval.tv_usec - older.m_timeval.tv_usec + 500) / 1000 +
     (m_timeval.tv_sec - older.m_timeval.tv_sec) * 1000);

  return diff;
}

