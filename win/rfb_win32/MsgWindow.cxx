/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 D. R. Commander.  All Rights Reserved.
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

// -=- MsgWindow.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/WMShatter.h>

#include <malloc.h>

using namespace rfb;
using namespace rfb::win32;

static core::LogWriter vlog("MsgWindow");

//
// -=- MsgWindowClass
//

class MsgWindowClass {
public:
  MsgWindowClass();
  ~MsgWindowClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK MsgWindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;

  if (msg == WM_CREATE)
    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLongPtr(wnd, GWLP_USERDATA, 0);
  MsgWindow* _this = (MsgWindow*) GetWindowLongPtr(wnd, GWLP_USERDATA);
  if (!_this) {
    vlog.info("Null _this in %p, message %x", wnd, msg);
    return SafeDefWindowProc(wnd, msg, wParam, lParam);
  }

  try {
    result = _this->processMessage(msg, wParam, lParam);
  } catch (std::exception& e) {
    vlog.error("Untrapped: %s", e.what());
  }

  return result;
};

MsgWindowClass::MsgWindowClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = MsgWindowProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(nullptr);
  wndClass.hIcon = nullptr;
  wndClass.hCursor = nullptr;
  wndClass.hbrBackground = nullptr;
  wndClass.lpszMenuName = nullptr;
  wndClass.lpszClassName = "rfb::win32::MsgWindowClass";
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw core::win32_error("Unable to register MsgWindow window class", GetLastError());
  }
}

MsgWindowClass::~MsgWindowClass() {
  if (classAtom) {
    UnregisterClass((const char*)(intptr_t)classAtom, instance);
  }
}

static MsgWindowClass baseClass;

//
// -=- MsgWindow
//

MsgWindow::MsgWindow(const char* name_) : name(name_), handle(nullptr) {
  vlog.debug("Creating window \"%s\"", name.c_str());
  handle = CreateWindow((const char*)(intptr_t)baseClass.classAtom,
                        name.c_str(), WS_OVERLAPPED, 0, 0, 10, 10,
                        nullptr, nullptr, baseClass.instance, this);
  if (!handle) {
    throw core::win32_error("Unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("Created window \"%s\" (%p)", name.c_str(), handle);
}

MsgWindow::~MsgWindow() {
  if (handle)
    DestroyWindow(handle);
  vlog.debug("Destroyed window \"%s\" (%p)", name.c_str(), handle);
}

LRESULT
MsgWindow::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  return SafeDefWindowProc(getHandle(), msg, wParam, lParam);
}
