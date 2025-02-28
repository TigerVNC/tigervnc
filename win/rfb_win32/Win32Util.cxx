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

// Win32Util.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/Exception.h>
#include <core/string.h>

#include <rfb_win32/ModuleFileName.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/MonitorInfo.h>
#include <rfb_win32/Handle.h>

#include <rdr/HexOutStream.h>

#include <stdio.h>

using namespace core;

namespace rfb {
namespace win32 {


FileVersionInfo::FileVersionInfo(const char* filename) {
  // Get executable name
  ModuleFileName exeName;
  if (!filename)
    filename = exeName.buf;

  // Attempt to open the file, to cause Access Denied, etc, errors
  // to be correctly reported, since the GetFileVersionInfoXXX calls lie...
  {
    Handle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	  if (file.h == INVALID_HANDLE_VALUE)
      throw core::win32_error("Failed to open file", GetLastError());
  }

  // Get version info size
  DWORD handle;
  int size = GetFileVersionInfoSize((char*)filename, &handle);
  if (!size)
    throw core::win32_error("GetVersionInfoSize failed", GetLastError());

  // Get version info
  buf = new char[size];
  if (!GetFileVersionInfo((char*)filename, handle, size, buf))
    throw core::win32_error("GetVersionInfo failed", GetLastError());
}

FileVersionInfo::~FileVersionInfo() {
  delete [] buf;
}

const char* FileVersionInfo::getVerString(const char* name, DWORD langId) {
  uint8_t langIdBuf[sizeof(langId)];
  for (int i=sizeof(langIdBuf)-1; i>=0; i--) {
    langIdBuf[i] = (langId & 0xff);
    langId = langId >> 8;
  }

  std::string langIdStr(binToHex(langIdBuf, sizeof(langId)));
  std::string infoName;
  infoName = format("\\StringFileInfo\\%s\\%s", langIdStr.c_str(), name);

  // Locate the required version string within the version info
  char* buffer = nullptr;
  UINT length = 0;
  if (!VerQueryValue(buf, infoName.c_str(), (void**)&buffer, &length)) {
    printf("unable to find %s version string", infoName.c_str());
    throw std::runtime_error("VerQueryValue failed");
  }
  return buffer;
}


void centerWindow(HWND handle, HWND parent) {
  RECT r;
  MonitorInfo mi(parent ? parent : handle);
  if (!parent || !IsWindowVisible(parent) || !GetWindowRect(parent, &r))
    r=mi.rcWork;
  centerWindow(handle, r);
  mi.clipTo(handle);
}

void centerWindow(HWND handle, const RECT& r) {
  RECT wr;
  if (!GetWindowRect(handle, &wr)) return;
  int w = wr.right-wr.left;
  int h = wr.bottom-wr.top;
  int x = (r.left + r.right - w)/2;
  int y = (r.top + r.bottom - h)/2;
  UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOSIZE;
  SetWindowPos(handle, nullptr, x, y, 0, 0, flags);
}

void resizeWindow(HWND handle, int width, int height) {
  RECT r;
  GetWindowRect(handle, &r);
  SetWindowPos(handle, nullptr, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
  centerWindow(handle, r);
}


};
};
