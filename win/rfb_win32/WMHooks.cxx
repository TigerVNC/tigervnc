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

// -=- WMHooks.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <os/Mutex.h>
#include <os/Thread.h>

#include <rfb_win32/WMHooks.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/IntervalTimer.h>
#include <rfb/LogWriter.h>

#include <list>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMHooks");


static HMODULE hooksLibrary;

typedef UINT (*WM_Hooks_WMVAL_proto)();
static WM_Hooks_WMVAL_proto WM_Hooks_WindowChanged;
static WM_Hooks_WMVAL_proto WM_Hooks_WindowBorderChanged;
static WM_Hooks_WMVAL_proto WM_Hooks_WindowClientAreaChanged;
static WM_Hooks_WMVAL_proto WM_Hooks_RectangleChanged;
#ifdef _DEBUG
static WM_Hooks_WMVAL_proto WM_Hooks_Diagnostic;
#endif

typedef BOOL (*WM_Hooks_Install_proto)(DWORD owner, DWORD thread);
static WM_Hooks_Install_proto WM_Hooks_Install;
typedef BOOL (*WM_Hooks_Remove_proto)(DWORD owner);
static WM_Hooks_Remove_proto WM_Hooks_Remove;
#ifdef _DEBUG
typedef void (*WM_Hooks_SetDiagnosticRange_proto)(UINT min, UINT max);
static WM_Hooks_SetDiagnosticRange_proto WM_Hooks_SetDiagnosticRange;
#endif

typedef BOOL (*WM_Hooks_EnableRealInputs_proto)(BOOL pointer, BOOL keyboard);
static WM_Hooks_EnableRealInputs_proto WM_Hooks_EnableRealInputs;


static void LoadHooks()
{
  if (hooksLibrary != NULL)
    return;

  hooksLibrary = LoadLibrary("wm_hooks.dll");
  if (hooksLibrary == NULL)
    return;

  WM_Hooks_WindowChanged = (WM_Hooks_WMVAL_proto)GetProcAddress(hooksLibrary, "WM_Hooks_WindowChanged");
  if (WM_Hooks_WindowChanged == NULL)
    goto error;
  WM_Hooks_WindowBorderChanged = (WM_Hooks_WMVAL_proto)GetProcAddress(hooksLibrary, "WM_Hooks_WindowBorderChanged");
  if (WM_Hooks_WindowBorderChanged == NULL)
    goto error;
  WM_Hooks_WindowClientAreaChanged = (WM_Hooks_WMVAL_proto)GetProcAddress(hooksLibrary, "WM_Hooks_WindowClientAreaChanged");
  if (WM_Hooks_WindowClientAreaChanged == NULL)
    goto error;
  WM_Hooks_RectangleChanged = (WM_Hooks_WMVAL_proto)GetProcAddress(hooksLibrary, "WM_Hooks_RectangleChanged");
  if (WM_Hooks_RectangleChanged == NULL)
    goto error;
#ifdef _DEBUG
  WM_Hooks_Diagnostic = (WM_Hooks_WMVAL_proto)GetProcAddress(hooksLibrary, "WM_Hooks_Diagnostic");
  if (WM_Hooks_Diagnostic == NULL)
    goto error;
#endif

  WM_Hooks_Install = (WM_Hooks_Install_proto)GetProcAddress(hooksLibrary, "WM_Hooks_Install");
  if (WM_Hooks_Install == NULL)
    goto error;
  WM_Hooks_Remove = (WM_Hooks_Remove_proto)GetProcAddress(hooksLibrary, "WM_Hooks_Remove");
  if (WM_Hooks_Remove == NULL)
    goto error;
#ifdef _DEBUG
  WM_Hooks_SetDiagnosticRange = (WM_Hooks_SetDiagnosticRange_proto)GetProcAddress(hooksLibrary, "WM_Hooks_SetDiagnosticRange");
  if (WM_Hooks_SetDiagnosticRange == NULL)
    goto error;
#endif

  WM_Hooks_EnableRealInputs = (WM_Hooks_EnableRealInputs_proto)GetProcAddress(hooksLibrary, "WM_Hooks_EnableRealInputs");
  if (WM_Hooks_EnableRealInputs == NULL)
    goto error;

  return;

error:
  FreeLibrary(hooksLibrary);
  hooksLibrary = NULL;
}


class WMHooksThread : public os::Thread {
public:
  WMHooksThread() : active(true), thread_id(-1) { }
  void stop();
  DWORD getThreadId() { return thread_id; }
protected:
  virtual void worker();
protected:
  bool active;
  DWORD thread_id;
};

static WMHooksThread* hook_mgr = 0;
static std::list<WMHooks*> hooks;
static os::Mutex hook_mgr_lock;


static bool StartHookThread() {
  if (hook_mgr)
    return true;
  if (hooksLibrary == NULL)
    return false;
  vlog.debug("creating thread");
  hook_mgr = new WMHooksThread();
  hook_mgr->start();
  while (hook_mgr->getThreadId() == (DWORD)-1)
    Sleep(0);
  vlog.debug("installing hooks");
  if (!WM_Hooks_Install(hook_mgr->getThreadId(), 0)) {
    vlog.error("failed to initialise hooks");
    hook_mgr->stop();
    delete hook_mgr;
    hook_mgr = 0;
    return false;
  }
  return true;
}

static void StopHookThread() {
  if (!hook_mgr)
    return;
  if (!hooks.empty())
    return;
  vlog.debug("closing thread");
  hook_mgr->stop();
  delete hook_mgr;
  hook_mgr = 0;
}


static bool AddHook(WMHooks* hook) {
  vlog.debug("adding hook");
  os::AutoMutex a(&hook_mgr_lock);
  if (!StartHookThread())
    return false;
  hooks.push_back(hook);
  return true;
}

static bool RemHook(WMHooks* hook) {
  {
    vlog.debug("removing hook");
    os::AutoMutex a(&hook_mgr_lock);
    hooks.remove(hook);
  }
  StopHookThread();
  return true;
}

static void NotifyHooksRegion(const Region& r) {
  os::AutoMutex a(&hook_mgr_lock);
  std::list<WMHooks*>::iterator i;
  for (i=hooks.begin(); i!=hooks.end(); i++)
    (*i)->NotifyHooksRegion(r);
}


void
WMHooksThread::worker() {
  // Obtain message ids for all supported hook messages
  UINT windowMsg = WM_Hooks_WindowChanged();
  UINT clientAreaMsg = WM_Hooks_WindowClientAreaChanged();
  UINT borderMsg = WM_Hooks_WindowBorderChanged();
  UINT rectangleMsg = WM_Hooks_RectangleChanged();
#ifdef _DEBUG
  UINT diagnosticMsg = WM_Hooks_Diagnostic();
#endif
  MSG msg;
  RECT wrect;
  HWND hwnd;
  int count = 0;

  // Update delay handling
  //   We delay updates by 40-80ms, so that the triggering application has time to
  //   actually complete them before we notify the hook callbacks & they go off
  //   capturing screen state.
  const int updateDelayMs = 40;
  MsgWindow updateDelayWnd(_T("WMHooks::updateDelay"));
  IntervalTimer updateDelayTimer(updateDelayWnd.getHandle(), 1);
  Region updates[2];
  int activeRgn = 0;

  vlog.debug("starting hook thread");

  thread_id = GetCurrentThreadId();

  while (active && GetMessage(&msg, NULL, 0, 0)) {
    count++;

    if (msg.message == WM_TIMER) {
      // Actually notify callbacks of graphical updates
      NotifyHooksRegion(updates[1-activeRgn]);
      if (updates[activeRgn].is_empty())
        updateDelayTimer.stop();
      activeRgn = 1-activeRgn;
      updates[activeRgn].clear();

    } else if (msg.message == windowMsg) {
      // An entire window has (potentially) changed
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
        GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect)) {
          updates[activeRgn].assign_union(Rect(wrect.left, wrect.top,
                                               wrect.right, wrect.bottom));
          updateDelayTimer.start(updateDelayMs);
      }

    } else if (msg.message == clientAreaMsg) {
      // The client area of a window has (potentially) changed
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetClientRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        POINT pt = {0,0};
        if (ClientToScreen(hwnd, &pt)) {
          updates[activeRgn].assign_union(Rect(wrect.left+pt.x, wrect.top+pt.y,
                                               wrect.right+pt.x, wrect.bottom+pt.y));
          updateDelayTimer.start(updateDelayMs);
        }
      }

    } else if (msg.message == borderMsg) {
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        Region changed(Rect(wrect.left, wrect.top, wrect.right, wrect.bottom));
        RECT crect;
        POINT pt = {0,0};
        if (GetClientRect(hwnd, &crect) && ClientToScreen(hwnd, &pt) &&
            !IsRectEmpty(&crect))
        {
          changed.assign_subtract(Rect(crect.left+pt.x, crect.top+pt.y,
                                       crect.right+pt.x, crect.bottom+pt.y));
        }
        if (!changed.is_empty()) {
          updates[activeRgn].assign_union(changed);
          updateDelayTimer.start(updateDelayMs);
        }
      }
    } else if (msg.message == rectangleMsg) {
      Rect r = Rect(LOWORD(msg.wParam), HIWORD(msg.wParam),
                    LOWORD(msg.lParam), HIWORD(msg.lParam));
      if (!r.is_empty()) {
        updates[activeRgn].assign_union(r);
        updateDelayTimer.start(updateDelayMs);
      }

#ifdef _DEBUG
    } else if (msg.message == diagnosticMsg) {
      vlog.info("DIAG msg=%x(%d) wnd=%lx",
                (unsigned)msg.wParam, (int)msg.wParam,
                (unsigned long)msg.lParam);
#endif
    }
  }

  vlog.debug("stopping hook thread - processed %d events", count);
  WM_Hooks_Remove(getThreadId());
}

void
WMHooksThread::stop() {
  vlog.debug("stopping WMHooks thread");
  active = false;
  PostThreadMessage(thread_id, WM_QUIT, 0, 0);
  vlog.debug("waiting for WMHooks thread");
  wait();
}

// -=- WMHooks class

rfb::win32::WMHooks::WMHooks() : updateEvent(0) {
  LoadHooks();
}

rfb::win32::WMHooks::~WMHooks() {
  RemHook(this);
}

bool rfb::win32::WMHooks::setEvent(HANDLE ue) {
  if (updateEvent)
    RemHook(this);
  updateEvent = ue;
  return AddHook(this);
}

bool rfb::win32::WMHooks::getUpdates(UpdateTracker* ut) {
  if (!updatesReady) return false;
  os::AutoMutex a(&hook_mgr_lock);
  updates.copyTo(ut);
  updates.clear();
  updatesReady = false;
  return true;
}

#ifdef _DEBUG
void
rfb::win32::WMHooks::setDiagnosticRange(UINT min, UINT max) {
  WM_Hooks_SetDiagnosticRange(min, max);
}
#endif

void rfb::win32::WMHooks::NotifyHooksRegion(const Region& r) {
  // hook_mgr_lock is already held at this point
  updates.add_changed(r);
  updatesReady = true;
  SetEvent(updateEvent);
}


// -=- WMBlockInput class

rfb::win32::WMBlockInput::WMBlockInput() : active(false) {
  LoadHooks();
}

rfb::win32::WMBlockInput::~WMBlockInput() {
  blockInputs(false);
}

static bool blocking = false;
static bool blockRealInputs(bool block_) {
  // NB: Requires blockMutex to be held!
  if (hooksLibrary == NULL)
    return false;
  if (block_) {
    if (blocking)
      return true;
    // Enable blocking
    if (!WM_Hooks_EnableRealInputs(false, false))
      return false;
    blocking = true;
  }
  if (blocking) {
    WM_Hooks_EnableRealInputs(true, true);
    blocking = false;
  }
  return block_ == blocking;
}

static os::Mutex blockMutex;
static int blockCount = 0;

bool rfb::win32::WMBlockInput::blockInputs(bool on) {
  if (active == on) return true;
  os::AutoMutex a(&blockMutex);
  int newCount = on ? blockCount+1 : blockCount-1;
  if (!blockRealInputs(newCount > 0))
    return false;
  blockCount = newCount;
  active = on;
  return true;
}
