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

//
// util_win32.h - miscellaneous useful bits for Win32 only
//

#ifndef __RFB_UTIL_WIN32_H__
#define __RFB_UTIL_WIN32_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// *** #include <iostream.h>

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

namespace rfb {

  // WIN32-ONLY PROFILING CODE
  //
  // CpuTime and CpuTimer provide a simple way to profile particular
  // sections of code
  //
  // Use one CpuTime object per task to be profiled.  CpuTime instances
  // maintain a cumulative total of time spent in user and kernel space
  // by threads.
  // When a CpuTime object is created, a label must be specified to
  // identify the task being profiled.
  // When the object is destroyed, it will print debugging information
  // containing the user and kernel times accumulated.
  //
  // Place a CpuTimer object in each section of code which is to be
  // profiled.  When the object is created, it snapshots the current
  // kernel and user times and stores them.  These are used when the
  // object is destroyed to establish how much time has elapsed in the
  // intervening period.  The accumulated time is then added to the
  // associated CpuTime object.
  //
  // This code works only on platforms providing __int64

	class CpuTime {
	public:
		CpuTime(const char *name)
			: timer_name(strDup(name)),
			  kernel_time(0), user_time(0), max_user_time(0), iterations(0) {}
		~CpuTime() {
      g_log_writer.info("timer %s : %I64ums (krnl), %I64ums (user), %I64uus (user-max) (%I64u its)\n",
				timer_name, kernel_time/10000, user_time/10000, max_user_time/10,
				iterations);
			delete [] timer_name;
		}
    static LogWriter g_log_writer;
		char* timer_name;
		__int64 kernel_time;
		__int64 user_time;
		__int64 iterations;
		__int64 max_user_time;
	};

	class CpuTimer {
	public:
		inline CpuTimer(CpuTime &ct) : cputime(ct) {
			FILETIME create_time, end_time;
			if (!GetThreadTimes(GetCurrentThread(),
				&create_time, &end_time,
				(LPFILETIME)&start_kernel_time,
				(LPFILETIME)&start_user_time)) {
        throw rdr::SystemException("rfb::CpuTimer failed to initialise", GetLastError());
			}
		}
		inline ~CpuTimer() {
			FILETIME create_time, end_time;
			__int64 end_kernel_time, end_user_time;
			if (!GetThreadTimes(GetCurrentThread(),
				&create_time, &end_time,
				(LPFILETIME)&end_kernel_time,
				(LPFILETIME)&end_user_time)) {
        throw rdr::SystemException("rfb::CpuTimer destructor failed", GetLastError());
			}
			cputime.kernel_time += end_kernel_time - start_kernel_time;
			cputime.user_time += end_user_time - start_user_time;
			if (end_user_time - start_user_time > cputime.max_user_time) {
				cputime.max_user_time = end_user_time - start_user_time;
			}
			cputime.iterations++;
		}
	private:
		CpuTime& cputime;
		__int64 start_kernel_time;
		__int64 start_user_time;
	};

};

#endif // __RFB_UTIL_WIN32_H__
