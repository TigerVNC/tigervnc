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

// -=- WMHooks.cxx

#include <wm_hooks/wm_hooks.h>

#include <rfb_win32/WMHooks.h>
#include <rfb_win32/Service.h>
#include <rfb/Threading.h>
#include <rfb/LogWriter.h>

#include <list>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("WMHooks");

class WMHooksThread : public Thread {
public:
  WMHooksThread() : Thread("WMHookThread"), active(true) {}
  virtual void run();
  virtual Thread* join();
protected:
  bool active;
};

WMHooksThread* hook_mgr = 0;
std::list<WMHooks*> hooks;
std::list<WMCursorHooks*> cursor_hooks;
Mutex hook_mgr_lock;
HCURSOR hook_cursor = (HCURSOR)LoadImage(0, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);


bool
StartHookThread() {
  if (hook_mgr) return true;
  vlog.debug("opening hook thread");
  hook_mgr = new WMHooksThread();
  if (!WM_Hooks_Install(hook_mgr->getThreadId(), 0)) {
    vlog.error("failed to initialise hooks");
    delete hook_mgr->join();
    hook_mgr = 0;
    return false;
  }
  hook_mgr->start();
  return true;
}

void
StopHookThread() {
  if (!hook_mgr) return;
  if (!hooks.empty() || !cursor_hooks.empty()) return;
  vlog.debug("closing hook thread");
  delete hook_mgr->join();
  hook_mgr = 0;
}


bool
AddHook(WMHooks* hook) {
  vlog.debug("adding hook");
  Lock l(hook_mgr_lock);
  if (!StartHookThread()) return false;
  hooks.push_back(hook);
  return true;
}

bool
AddCursorHook(WMCursorHooks* hook) {
  vlog.debug("adding cursor hook");
  Lock l(hook_mgr_lock);
  if (cursor_hooks.empty()) WM_Hooks_EnableCursorShape(TRUE);
  if (!StartHookThread()) return false;
  cursor_hooks.push_back(hook);
  return true;
}

bool
RemHook(WMHooks* hook) {
  {
    vlog.debug("removing hook");
    Lock l(hook_mgr_lock);
    hooks.remove(hook);
  }
  StopHookThread();
  return true;
}

bool
RemCursorHook(WMCursorHooks* hook) {
  {
    vlog.debug("removing cursor hook");
    Lock l(hook_mgr_lock);
    cursor_hooks.remove(hook);
  }
  StopHookThread();
  if (cursor_hooks.empty()) WM_Hooks_EnableCursorShape(FALSE);
  return true;
}

void
NotifyHooksRegion(const Region& r) {
  Lock l(hook_mgr_lock);
  std::list<WMHooks*>::iterator i;
  for (i=hooks.begin(); i!=hooks.end(); i++) {
    (*i)->new_changes.add_changed(r);
    if (!(*i)->notified) {
      (*i)->notified = true;
      PostMessage((*i)->getHandle(), WM_USER, 0, 0);
    }
  }
}

void
NotifyHooksCursor(HCURSOR c) {
  Lock l(hook_mgr_lock);
  hook_cursor = c;
}

void
WMHooksThread::run() {
  UINT windowMsg = WM_Hooks_WindowChanged();
  UINT clientAreaMsg = WM_Hooks_WindowClientAreaChanged();
  UINT borderMsg = WM_Hooks_WindowBorderChanged();
  UINT rectangleMsg = WM_Hooks_RectangleChanged();
  UINT cursorMsg = WM_Hooks_CursorChanged();
#ifdef _DEBUG
  UINT diagnosticMsg = WM_Hooks_Diagnostic();
#endif
  MSG msg;
  RECT wrect;
  HWND hwnd;
  int count = 0;

  vlog.debug("starting hook thread");

  while (active && GetMessage(&msg, NULL, 0, 0)) {
    count++;
    if (msg.message == windowMsg) {
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetWindowRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        NotifyHooksRegion(Rect(wrect.left, wrect.top,
                               wrect.right, wrect.bottom));

      }
    } else if (msg.message == clientAreaMsg) {
      hwnd = (HWND) msg.lParam;
      if (IsWindow(hwnd) && IsWindowVisible(hwnd) && !IsIconic(hwnd) &&
          GetClientRect(hwnd, &wrect) && !IsRectEmpty(&wrect))
      {
        POINT pt = {0,0};
        if (ClientToScreen(hwnd, &pt)) {
          NotifyHooksRegion(Rect(wrect.left+pt.x, wrect.top+pt.y,
                                 wrect.right+pt.x, wrect.bottom+pt.y));
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
        NotifyHooksRegion(changed);
      }
    } else if (msg.message == rectangleMsg) {
      Rect r = Rect(LOWORD(msg.wParam), HIWORD(msg.wParam),
                    LOWORD(msg.lParam), HIWORD(msg.lParam));
      if (!r.is_empty()) {
        NotifyHooksRegion(r);
      }
    } else if (msg.message == cursorMsg) {
      NotifyHooksCursor((HCURSOR)msg.lParam);
#ifdef _DEBUG
    } else if (msg.message == diagnosticMsg) {
      vlog.info("DIAG msg=%x(%d) wnd=%lx", msg.wParam, msg.wParam, msg.lParam);
#endif
    }
  }

  vlog.debug("stopping hook thread - processed %d events", count);
  WM_Hooks_Remove(getThreadId());
}

Thread*
WMHooksThread::join() {
  vlog.debug("stopping WMHooks thread");
  active = false;
  PostThreadMessage(thread_id, WM_QUIT, 0, 0);
  vlog.debug("joining WMHooks thread");
  return Thread::join();
}

// -=- WMHooks class

rfb::win32::WMHooks::WMHooks()
  : clipper(0), new_changes(true), fg_window(0),
  notified(false), MsgWindow(_T("WMHooks")) {
}

rfb::win32::WMHooks::~WMHooks() {
  RemHook(this);
  if (clipper) delete clipper;
}

LRESULT
rfb::win32::WMHooks::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_USER:
    {
      // *** Yield, to allow the triggering update event to be processed
      //     BEFORE we try to grab the resulting changes.
      // *** IMPROVES THINGS NOTICABLY ON WinXP
      Sleep(0);
      // ***

      Lock l(hook_mgr_lock);
      notified = false;
      new_changes.get_update(*clipper);
      new_changes.clear();
    }
    break;
  }
  return MsgWindow::processMessage(msg, wParam, lParam);
}

bool
rfb::win32::WMHooks::setClipRect(const Rect& r) {
  clip_region = r;
  if (clipper) clipper->set_clip_region(clip_region);
  return true;
}

bool
rfb::win32::WMHooks::setUpdateTracker(UpdateTracker* ut) {
  if (clipper) delete clipper;
  clipper = new ClippedUpdateTracker(*ut);
  clipper->set_clip_region(clip_region);
  return AddHook(this);
}

#ifdef _DEBUG
void
rfb::win32::WMHooks::setDiagnosticRange(UINT min, UINT max) {
  WM_Hooks_SetDiagnosticRange(min, max);
}
#endif


// -=- WMBlockInput class

Mutex blockMutex;
int blockCount = 0;

rfb::win32::WMBlockInput::WMBlockInput() : active(false) {
}

rfb::win32::WMBlockInput::~WMBlockInput() {
  blockInputs(false);
}

bool rfb::win32::WMBlockInput::blockInputs(bool on) {
  if (on == active) return true;
  vlog.debug("blockInput changed");
  Lock l(blockMutex);
  int newCount = blockCount;
  if (on)
    newCount++;
  else
    newCount--;
  if (WM_Hooks_EnableRealInputs(newCount==0, newCount==0)) {
    vlog.debug("set blocking to %d", newCount);
    blockCount = newCount;
    active = on;
    return true;
  }
  return false;
}


// -=- WMCursorHooks class

rfb::win32::WMCursorHooks::WMCursorHooks() {
}

rfb::win32::WMCursorHooks::~WMCursorHooks() {
  RemCursorHook(this);
}

bool
rfb::win32::WMCursorHooks::start() {
  return AddCursorHook(this);
}

HCURSOR
rfb::win32::WMCursorHooks::getCursor() const {
  return hook_cursor;
}
