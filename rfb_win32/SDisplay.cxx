/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

// -=- SDisplay.cxx
//
// The SDisplay class encapsulates a particular system display.

#include <assert.h>

#include <rfb_win32/SDisplay.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/WMShatter.h>
#include <rfb_win32/osVersion.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/IntervalTimer.h>
#include <rfb_win32/CleanDesktop.h>

#include <rfb/util.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

#include <rfb/Configuration.h>

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("SDisplay");

// - SDisplay-specific configuration options

BoolParameter rfb::win32::SDisplay::use_hooks("UseHooks",
  "Set hooks in the operating system to capture display updates more efficiently", true);
BoolParameter rfb::win32::SDisplay::disableLocalInputs("DisableLocalInputs",
  "Disable local keyboard and pointer input while the server is in use", false);
StringParameter rfb::win32::SDisplay::disconnectAction("DisconnectAction",
  "Action to perform when all clients have disconnected.  (None, Lock, Logoff)", "None");

BoolParameter rfb::win32::SDisplay::removeWallpaper("RemoveWallpaper",
  "Remove the desktop wallpaper when the server in in use.", false);
BoolParameter rfb::win32::SDisplay::removePattern("RemovePattern",
  "Remove the desktop background pattern when the server in in use.", false);
BoolParameter rfb::win32::SDisplay::disableEffects("DisableEffects",
  "Disable desktop user interface effects when the server is in use.", false);


// - WM_TIMER ID values

#define TIMER_CURSOR 1
#define TIMER_UPDATE 2
#define TIMER_UPDATE_AND_POLL 3


// -=- Polling settings

const int POLLING_SEGMENTS = 16;

const int FG_POLLING_FPS = 20;
const int FG_POLLING_FS_INTERVAL = 1000 / FG_POLLING_FPS;
const int FG_POLLING_INTERVAL = FG_POLLING_FS_INTERVAL / POLLING_SEGMENTS;

const int BG_POLLING_FS_INTERVAL = 5000;
const int BG_POLLING_INTERVAL = BG_POLLING_FS_INTERVAL / POLLING_SEGMENTS;


//////////////////////////////////////////////////////////////////////////////
//
// SDisplayCore
//

// The SDisplay Core object is created by SDisplay's start() method
// and deleted by its stop() method.
// The Core must be created in the current input desktop in order
// to operate - SDisplay is responsible for ensuring that.
// The structures contained in the Core are manipulated directly
// by the SDisplay, which is also responsible for detecting when
// a desktop-switch is required.

class rfb::win32::SDisplayCore : public MsgWindow {
public:
  SDisplayCore(SDisplay* display);
  ~SDisplayCore();

  void setPixelBuffer(DeviceFrameBuffer* pb_);

  bool isRestartRequired();

  virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

  // -=- Timers
  IntervalTimer pollTimer;
  IntervalTimer cursorTimer;

  // -=- Input handling
  rfb::win32::SPointer ptr;
  rfb::win32::SKeyboard kbd;
  rfb::win32::Clipboard clipboard;

  // -=- Hook handling objects used outside thread run() method
  WMCopyRect wm_copyrect;
  WMPoller wm_poller;
  WMCursor cursor;
  WMMonitor wm_monitor;
  WMHooks wm_hooks;
  WMBlockInput wm_input;

  // -=- Tidying the desktop
  CleanDesktop cleanDesktop;
  bool isWallpaperRemoved;
  bool isPatternRemoved;
  bool areEffectsDisabled;

  // -=- Full screen polling
  int poll_next_y;
  int poll_y_increment;

  // Are we using hooks?
  bool use_hooks;
  bool using_hooks;

  // State of the display object
  SDisplay* display;
};

SDisplayCore::SDisplayCore(SDisplay* display_)
: MsgWindow(_T("SDisplayCore")), display(display_),
  using_hooks(0), use_hooks(rfb::win32::SDisplay::use_hooks),
  isWallpaperRemoved(rfb::win32::SDisplay::removeWallpaper),
  isPatternRemoved(rfb::win32::SDisplay::removePattern),
  areEffectsDisabled(rfb::win32::SDisplay::disableEffects),
  pollTimer(getHandle(), TIMER_UPDATE_AND_POLL),
  cursorTimer(getHandle(), TIMER_CURSOR) {
  setPixelBuffer(display->pb);
}

SDisplayCore::~SDisplayCore() {
}

void SDisplayCore::setPixelBuffer(DeviceFrameBuffer* pb) {
  poll_y_increment = (display->pb->height()+POLLING_SEGMENTS-1)/POLLING_SEGMENTS;
  poll_next_y = display->screenRect.tl.y;
  wm_hooks.setClipRect(display->screenRect);
  wm_copyrect.setClipRect(display->screenRect);
  wm_poller.setClipRect(display->screenRect);
}


bool SDisplayCore::isRestartRequired() {
  // - We must restart the SDesktop if:
  // 1. We are no longer in the input desktop.
  // 2. The use_hooks setting has changed.

  // - Check that we are in the input desktop
  if (rfb::win32::desktopChangeRequired())
    return true;

  // - Check that the hooks setting hasn't changed
  // NB: We can't just check using_hooks because that can be false
  // because they failed, even though use_hooks is true!
  if (use_hooks != rfb::win32::SDisplay::use_hooks)
    return true;

  // - Check that the desktop optimisation settings haven't changed
  //   This isn't very efficient, but it shouldn't change very often!
  if ((isWallpaperRemoved != rfb::win32::SDisplay::removeWallpaper) ||
      (isPatternRemoved != rfb::win32::SDisplay::removePattern) ||
      (areEffectsDisabled != rfb::win32::SDisplay::disableEffects))
    return true;

  return false;
}

LRESULT SDisplayCore::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

  case WM_TIMER:

    if (display->server && display->server->clientsReadyForUpdate()) {

      // - Check that the SDesktop doesn't need restarting
      if (isRestartRequired()) {
        display->restart();
        return 0;
      }
    
      // - Action depends on the timer message type
      switch (wParam) {

        // POLL THE SCREEN
      case TIMER_UPDATE_AND_POLL:
        // Handle window dragging, polling of consoles, etc.
        while (wm_poller.processEvent()) {}

        // Poll the next strip of the screen (in Screen coordinates)
        {
          Rect pollrect = display->screenRect;
          if (poll_next_y >= pollrect.br.y) {
            // Yes.  Reset the counter and return
            poll_next_y = pollrect.tl.y;
          } else {
            // No.  Poll the next section
            pollrect.tl.y = poll_next_y;
            poll_next_y += poll_y_increment;
            pollrect.br.y = min(poll_next_y, pollrect.br.y);
            display->add_changed(pollrect);
          }
        }
        break;

      case TIMER_CURSOR:
        display->triggerUpdate();
        break;

      };

    }
    return 0;

  };

  return MsgWindow::processMessage(msg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////
//
// SDisplay
//

// -=- Constructor/Destructor

SDisplay::SDisplay(const TCHAR* devName)
  : server(0), change_tracker(true), pb(0),
    deviceName(tstrDup(devName)), device(0), releaseDevice(false),
    core(0), statusLocation(0)
{
  updateEvent.h = CreateEvent(0, TRUE, FALSE, 0);
}

SDisplay::~SDisplay()
{
  // XXX when the VNCServer has been deleted with clients active, stop()
  // doesn't get called - this ought to be fixed in VNCServerST.  In any event,
  // we should never call any methods on VNCServer once we're being deleted.
  // This is because it is supposed to be guaranteed that the SDesktop exists
  // throughout the lifetime of the VNCServer.  So if we're being deleted, then
  // the VNCServer ought not to exist and therefore we shouldn't invoke any
  // methods on it.  Setting server to zero here ensures that stop() doesn't
  // call setPixelBuffer(0) on the server.
  server = 0;
  if (core) stop();
}


// -=- SDesktop interface

void SDisplay::start(VNCServer* vs)
{
  vlog.debug("starting");
  server = vs;

  // Switch to the current input desktop
  // ***
  if (rfb::win32::desktopChangeRequired()) {
    if (!rfb::win32::changeDesktop())
      throw rdr::Exception("unable to switch into input desktop");
  }

  // Clear the change tracker
  change_tracker.clear();

  // Create the framebuffer object
  recreatePixelBuffer();

  // Create the SDisplayCore
  core = new SDisplayCore(this);
  assert(core);

  // Start display monitor and clipboard handler
  core->wm_monitor.setNotifier(this);
  core->clipboard.setNotifier(this);

  // Apply desktop optimisations
  if (removePattern)
    core->cleanDesktop.disablePattern();
  if (removeWallpaper)
    core->cleanDesktop.disableWallpaper();
  if (disableEffects)
    core->cleanDesktop.disableEffects();

  // Start hooks
  core->wm_hooks.setClipRect(screenRect);
  if (core->use_hooks) {
    // core->wm_hooks.setDiagnosticRange(0, 0x400-1);
    core->using_hooks = core->wm_hooks.setUpdateTracker(this);
    if (!core->using_hooks)
      vlog.debug("hook subsystem failed to initialise");
  }

  // Set up timers
  core->pollTimer.start(core->using_hooks ? BG_POLLING_INTERVAL : FG_POLLING_INTERVAL);
  core->cursorTimer.start(10);

  // Register an interest in faked copyrect events
  core->wm_copyrect.setUpdateTracker(&change_tracker);
  core->wm_copyrect.setClipRect(screenRect);

  // Polling of particular windows on the desktop
  core->wm_poller.setUpdateTracker(&change_tracker);
  core->wm_poller.setClipRect(screenRect);

  vlog.debug("started");

  if (statusLocation) *statusLocation = true;
}

void SDisplay::stop()
{
  vlog.debug("stopping");
  if (core) {
    // If SDisplay was actually active then perform the disconnect action
    CharArray action = disconnectAction.getData();
    if (stricmp(action.buf, "Logoff") == 0) {
      ExitWindowsEx(EWX_LOGOFF, 0);
    } else if (stricmp(action.buf, "Lock") == 0) {
      typedef BOOL (WINAPI *_LockWorkStation_proto)();
      DynamicFn<_LockWorkStation_proto> _LockWorkStation(_T("user32.dll"), "LockWorkStation");
      if (_LockWorkStation.isValid())
        (*_LockWorkStation)();
      else
        ExitWindowsEx(EWX_LOGOFF, 0);
    }
  }
  delete core;
  core = 0;
  delete pb;
  pb = 0;
  if (device) {
    if (releaseDevice)
      ReleaseDC(0, device);
    else
      DeleteDC(device);
  }
  device = 0;
  if (server)
    server->setPixelBuffer(0);

  server = 0;
  vlog.debug("stopped");

  if (statusLocation) *statusLocation = false;
}

void SDisplay::restart() {
  vlog.debug("restarting");
  // Close down the hooks
  delete core;
  core = 0;
  try {
    // Re-start the hooks if possible
    start(server);
    vlog.debug("restarted");
  } catch (rdr::Exception& e) {
    // If start() fails then we MUST disconnect all clients,
    // to cause the server to stop using the desktop.
    // Otherwise, the SDesktop is in an inconsistent state
    // and the server will crash
    server->closeClients(e.str());
  }
}


void SDisplay::pointerEvent(const Point& pos, rdr::U8 buttonmask) {
  if (pb->getRect().contains(pos)) {
    Point screenPos = pos.translate(screenRect.tl);
    core->ptr.pointerEvent(screenPos, buttonmask);
  }
}

void SDisplay::keyEvent(rdr::U32 key, bool down) {
  core->kbd.keyEvent(key, down);
}

void SDisplay::clientCutText(const char* text, int len) {
  CharArray clip_sz(len+1);
  memcpy(clip_sz.buf, text, len);
  clip_sz.buf[len] = 0;
  core->clipboard.setClipText(clip_sz.buf);
}


void SDisplay::framebufferUpdateRequest()
{
  triggerUpdate();
}

Point SDisplay::getFbSize() {
  bool startAndStop = !core;
  // If not started, do minimal initialisation to get desktop size.
  if (startAndStop) recreatePixelBuffer();
  Point result = Point(pb->width(), pb->height());
  // Destroy the initialised structures.
  if (startAndStop) stop();
  return result;
}


void
SDisplay::add_changed(const Region& rgn) {
  change_tracker.add_changed(rgn);
  triggerUpdate();
}

void
SDisplay::add_copied(const Region& dest, const Point& delta) {
  change_tracker.add_copied(dest, delta);
  triggerUpdate();
}


void
SDisplay::notifyClipboardChanged(const char* text, int len) {
  vlog.debug("clipboard text changed");
  if (server)
    server->serverCutText(text, len);
}


void
SDisplay::notifyDisplayEvent(WMMonitor::Notifier::DisplayEventType evt) {
  switch (evt) {
  case WMMonitor::Notifier::DisplaySizeChanged:
    vlog.debug("desktop size changed");
    recreatePixelBuffer();
    break;
  case WMMonitor::Notifier::DisplayPixelFormatChanged:
    vlog.debug("desktop format changed");
    recreatePixelBuffer();
    break;
  case WMMonitor::Notifier::DisplayColourMapChanged:
    vlog.debug("desktop colormap changed");
    pb->updateColourMap();
    if (server)
      server->setColourMapEntries();
    break;
  default:
    vlog.error("unknown display event received");
  }
}

bool
SDisplay::processEvent(HANDLE event) {
  if (event == updateEvent) {
    vlog.info("processEvent");
    ResetEvent(updateEvent);

    // - If the SDisplay isn't even started then quit now
    if (!core) {
      vlog.error("not start()ed");
      return true;
    }

    // - Ensure that the disableLocalInputs flag is respected
    core->wm_input.blockInputs(SDisplay::disableLocalInputs);

    // - Only process updates if the server is ready
    if (server && server->clientsReadyForUpdate()) {
      bool try_update = false;

      // - Check that the SDesktop doesn't need restarting
      if (core->isRestartRequired()) {
        restart();
        return true;
      }
    
      // *** window dragging can be improved - more frequent, more cunning about updates
      while (core->wm_copyrect.processEvent()) {}
        
      // Ensure the cursor is up to date
      WMCursor::Info info = core->cursor.getCursorInfo();
      if (old_cursor != info) {
        // Update the cursor shape if the visibility has changed
        bool set_cursor = info.visible != old_cursor.visible;
        // OR if the cursor is visible and the shape has changed.
        set_cursor |= info.visible && (old_cursor.cursor != info.cursor);

        // Update the cursor shape
        if (set_cursor)
          pb->setCursor(info.visible ? info.cursor : 0, server);

        // Update the cursor position
        // NB: First translate from Screen coordinates to Desktop
        Point desktopPos = info.position.translate(screenRect.tl.negate());
        server->setCursorPos(desktopPos.x, desktopPos.y);
        try_update = true;

        old_cursor = info;
      }

      // Flush any changes to the server
      try_update = flushChangeTracker() || try_update;
      if (try_update)
        server->tryUpdate();
    }
  } else {
    CloseHandle(event);
    return false;
  }
  return true;
}


// -=- Protected methods

void
SDisplay::recreatePixelBuffer() {
  vlog.debug("attaching to device %s", deviceName);

  // Open the specified display device
  HDC new_device;
  if (deviceName.buf) {
    new_device = ::CreateDC(_T("DISPLAY"), deviceName.buf, NULL, NULL);
    releaseDevice = false;
  } else {
    // If no device is specified, open entire screen.
    // Doing this with CreateDC creates problems on multi-monitor systems.
    new_device = ::GetDC(0);
    releaseDevice = true;
  }
  if (!new_device)
    throw SystemException("cannot open the display", GetLastError());

  // Get the coordinates of the entire virtual display
  Rect newScreenRect;
  {
    WindowDC rootDC(0);
    RECT r;
    if (!GetClipBox(rootDC, &r))
      throw rdr::SystemException("GetClipBox", GetLastError());
    newScreenRect = Rect(r.left, r.top, r.right, r.bottom);
  }

  // Create a DeviceFrameBuffer attached to it
  DeviceFrameBuffer* new_buffer = new DeviceFrameBuffer(new_device);

  // Has anything actually changed about the screen or the buffer?
  if (!pb ||
      (!newScreenRect.equals(screenRect)) ||
      (!new_buffer->getPF().equal(pb->getPF())))
  {
    // Yes.  Update the buffer state.
    screenRect = newScreenRect;
    vlog.debug("creating pixel buffer for device");

    // Flush any existing changes to the server
    flushChangeTracker();

    // Replace the old PixelBuffer
    if (pb) delete pb;
    if (device) DeleteDC(device);
    pb = new_buffer;
    device = new_device;

    // Initialise the pixels
    pb->grabRegion(pb->getRect());

    // Prevent future grabRect operations from throwing exceptions
    pb->setIgnoreGrabErrors(true);

    // Update the SDisplayCore if required
    if (core)
      core->setPixelBuffer(pb);

    // Inform the server of the changes
    if (server)
      server->setPixelBuffer(pb);

  } else {
    delete new_buffer;
    DeleteDC(new_device);
  }
}

bool SDisplay::flushChangeTracker() {
  if (change_tracker.is_empty())
    return false;
  // Translate the update coordinates from Screen coords to Desktop
  change_tracker.translate(screenRect.tl.negate());
  // Flush the updates through
  change_tracker.get_update(*server);
  change_tracker.clear();
  return true;
}

void SDisplay::triggerUpdate() {
  if (core)
    SetEvent(updateEvent);
}
