/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <tchar.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/Win32Util.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
 #define max(a,b) ((a)>(b)?(a):(b))
#endif

using namespace rfb;
using namespace win32;

static LogWriter vlog("MonitorInfo");


static void fillMonitorInfo(HMONITOR monitor, MONITORINFOEXA* mi) {
  vlog.debug("monitor=%p", monitor);
  memset(mi, 0, sizeof(MONITORINFOEXA));
  mi->cbSize = sizeof(MONITORINFOEXA);
  if (!GetMonitorInfo(monitor, mi))
    throw rdr::SystemException("failed to GetMonitorInfo", GetLastError());
  vlog.debug("monitor is %ld,%ld-%ld,%ld", mi->rcMonitor.left, mi->rcMonitor.top, mi->rcMonitor.right, mi->rcMonitor.bottom);
  vlog.debug("work area is %ld,%ld-%ld,%ld", mi->rcWork.left, mi->rcWork.top, mi->rcWork.right, mi->rcWork.bottom);
  vlog.debug("device is \"%s\"", mi->szDevice);
}


MonitorInfo::MonitorInfo(HWND window) {
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;

  HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
  if (!monitor)
    throw rdr::SystemException("failed to get monitor", GetLastError());
  fillMonitorInfo(monitor, this);
}

MonitorInfo::MonitorInfo(const RECT& r) {
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;

  HMONITOR monitor = MonitorFromRect(&r, MONITOR_DEFAULTTONEAREST);
  if (!monitor)
    throw rdr::SystemException("failed to get monitor", GetLastError());
  fillMonitorInfo(monitor, this);
}


struct monitorByNameData {
  MONITORINFOEXA* info;
  const char* monitorName;
};

static BOOL CALLBACK monitorByNameEnumProc(HMONITOR monitor,
                                    HDC dc,
                                    LPRECT pos,
                                    LPARAM d) {
  monitorByNameData* data = (monitorByNameData*)d;
  memset(data->info, 0, sizeof(MONITORINFOEXA));
  data->info->cbSize = sizeof(MONITORINFOEXA);
  if (GetMonitorInfo(monitor, data->info)) {
    if (stricmp(data->monitorName, data->info->szDevice) == 0)
      return FALSE;
  }

  return TRUE;
}

MonitorInfo::MonitorInfo(const char* devName) {
  monitorByNameData data;
  data.info = this;
  data.monitorName = devName;
  EnumDisplayMonitors(0, 0, &monitorByNameEnumProc, (LPARAM)&data);
}

void MonitorInfo::moveTo(HWND handle) {
  vlog.debug("moveTo monitor=%s", szDevice);

  MonitorInfo mi(handle);
  if (strcmp(szDevice, mi.szDevice) != 0) {
    centerWindow(handle, rcWork);
    clipTo(handle);
  }
}

void MonitorInfo::clipTo(RECT* r) {
  vlog.debug("clipTo monitor=%s", szDevice);

  if (r->top < rcWork.top) {
    r->bottom += rcWork.top - r->top; r->top = rcWork.top;
  }
  if (r->left < rcWork.left) {
    r->right += rcWork.left - r->left; r->left = rcWork.left;
  }
  if (r->bottom > rcWork.bottom) {
    r->top += rcWork.bottom - r->bottom; r->bottom = rcWork.bottom;
  }
  if (r->right > rcWork.right) {
    r->left += rcWork.right - r->right; r->right = rcWork.right;
  }
  r->left = max(r->left, rcWork.left);
  r->right = min(r->right, rcWork.right);
  r->top = max(r->top, rcWork.top);
  r->bottom = min(r->bottom, rcWork.bottom);
}

void MonitorInfo::clipTo(HWND handle) {
  RECT r;
  GetWindowRect(handle, &r);
  clipTo(&r);
  SetWindowPos(handle, 0, r.left, r.top, r.right-r.left, r.bottom-r.top,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}


