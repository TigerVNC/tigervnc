/* Copyright (C) 2011 TigerVNC Team.  All Rights Reserved.
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

#include "w32tiger.h"

#ifdef WIN32 

#ifdef __GNUC__

#define INITGUID
#include <basetyps.h>

#ifndef HAVE_ACTIVE_DESKTOP_L
DEFINE_GUID(CLSID_ActiveDesktop,0x75048700L,0xEF1F,0x11D0,0x98,0x88,0x00,0x60,0x97,0xDE,0xAC,0xF9);
DEFINE_GUID(IID_IActiveDesktop,0xF490EB00L,0x1240,0x11D1,0x98,0x88,0x00,0x60,0x97,0xDE,0xAC,0xF9);
#endif

#else /* __GNUC__ */

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tmpres /= 10;  /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		TIME_ZONE_INFORMATION tz_winapi;
		//_tzset(),don't work properly, so we use GetTimeZoneInformation
		int rez = GetTimeZoneInformation(&tz_winapi);
		tz->tz_dsttime = (rez == 2) ? TRUE : FALSE;
		tz->tz_minuteswest = tz_winapi.Bias + ((rez == 2) ? tz_winapi.DaylightBias : 0);
	}

	return 0;
}

#endif /* __GNUC__ */
#endif /* WIN32 */
