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

// -=- WMCursor.cxx

// *** DOESN'T SEEM TO WORK WITH GetCursorInfo POS CODE BUILT-IN UNDER NT4SP6
// *** INSTEAD, WE LOOK FOR Win2000/Win98 OR ABOVE
#define WINVER 0x0500

#include <rfb_win32/WMCursor.h>
#include <rfb_win32/OSVersion.h>
#include <rfb_win32/Win32Util.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMCursor");


typedef BOOL (WINAPI *_GetCursorInfo_proto)(PCURSORINFO pci);
DynamicFn<_GetCursorInfo_proto> _GetCursorInfo(_T("user32.dll"), "GetCursorInfo");


WMCursor::WMCursor() : hooks(0), library(0), use_getCursorInfo(false), cursor(0) {
#if (WINVER >= 0x0500)
  // Check the OS version
  bool is_win98 = (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
    (osVersion.dwMajorVersion > 4) || ((osVersion.dwMajorVersion == 4) && (osVersion.dwMinorVersion > 0));
  bool is_win2K = (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osVersion.dwMajorVersion >= 5);

  // Use GetCursorInfo if OS version is sufficient
  use_getCursorInfo = (is_win98 || is_win2K) && _GetCursorInfo.isValid();
#else
#pragma message ("not building in GetCursorInfo support")
#endif
  if (!use_getCursorInfo) {
    hooks = new WMCursorHooks();
    if (hooks && hooks->start()) {
      vlog.info("falling back to cursor hooking");
    } else {
      delete hooks;
      hooks = 0;
      vlog.error("unable to monitor cursor shape");
    }
  } else {
    vlog.info("using GetCursorInfo");
  }
  cursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
}

WMCursor::~WMCursor() {
  if (hooks) delete hooks;
  if (library) FreeLibrary(library);
}
  
WMCursor::Info
WMCursor::getCursorInfo() {
  Info result;
#if (WINVER >= 0x0500)
  if (use_getCursorInfo) {
    CURSORINFO info;
    info.cbSize = sizeof(CURSORINFO);
    if ((*_GetCursorInfo)(&info)) {
      result.cursor = info.hCursor;
      result.position = Point(info.ptScreenPos.x, info.ptScreenPos.y);
      result.visible = info.flags & CURSOR_SHOWING;
      return result;
    }
  }
#endif
  // Fall back to the old way of doing things
  POINT pos;
  if (hooks) cursor = hooks->getCursor();
  result.cursor = cursor;
  result.visible = cursor != 0;
  GetCursorPos(&pos);
  result.position.x = pos.x;
  result.position.y = pos.y;
  return result;
}
