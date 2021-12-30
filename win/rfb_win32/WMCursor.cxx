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

// -=- WMCursor.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/WMCursor.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMCursor");

WMCursor::WMCursor() : cursor(0) {
  cursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
}

WMCursor::~WMCursor() {
}
  
WMCursor::Info
WMCursor::getCursorInfo() {
  Info result;
  CURSORINFO info;
  info.cbSize = sizeof(CURSORINFO);
  if (!GetCursorInfo(&info))
    throw rdr::SystemException("GetCursorInfo failed", GetLastError());
  result.cursor = info.hCursor;
  result.position = Point(info.ptScreenPos.x, info.ptScreenPos.y);
  result.visible = info.flags & CURSOR_SHOWING;
  return result;
}
