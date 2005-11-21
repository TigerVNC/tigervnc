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
#define WIN32_LEAN_AND_MEAN
#if (_WIN32_WINNT < 0x0400)
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <winsock2.h>
#include <tchar.h>
#include <commctrl.h>

#include <network/TcpSocket.h>

#include <vncviewer/CView.h>
#include <vncviewer/UserPasswdDialog.h>
#include <vncviewer/resource.h>

#include <rfb/encodings.h>
#include <rfb/secTypes.h>
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/CMsgWriter.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>

#include <rfb_win32/WMShatter.h>

using namespace rfb;
using namespace rfb::win32;
using namespace rdr;

// - Statics & consts

static LogWriter vlog("CView");

const int IDM_FULLSCREEN = ID_FULLSCREEN;
const int IDM_SEND_MENU_KEY = ID_SEND_MENU_KEY;
const int IDM_SEND_CAD = ID_SEND_CAD;
const int IDM_SEND_CTLESC = ID_SEND_CTLESC;
const int IDM_ABOUT = ID_ABOUT;
const int IDM_OPTIONS = ID_OPTIONS;
const int IDM_INFO = ID_INFO;
const int IDM_NEWCONN = ID_NEW_CONNECTION;
const int IDM_REQUEST_REFRESH = ID_REQUEST_REFRESH;
const int IDM_CTRL_KEY = ID_CTRL_KEY;
const int IDM_ALT_KEY = ID_ALT_KEY;
const int IDM_FILE_TRANSFER = ID_FILE_TRANSFER;
const int IDM_CONN_SAVE_AS = ID_CONN_SAVE_AS;

const int TIMER_BUMPSCROLL = 1;
const int TIMER_POINTER_INTERVAL = 2;
const int TIMER_POINTER_3BUTTON = 3;

const int HOTKEY_ALTTAB = 0;


IntParameter debugDelay("DebugDelay","Milliseconds to display inverted "
                        "pixel data - a debugging feature", 0);


//
// -=- CViewClass

//
// Window class used as the basis for all CView instances
//

class CViewClass {
public:
  CViewClass();
  ~CViewClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK CViewProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;

  // *** vlog.debug("CViewMsg %x->(%x, %x, %x)", wnd, msg, wParam, lParam);

  if (msg == WM_CREATE)
    SetWindowLong(wnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLong(wnd, GWL_USERDATA, 0);
  CView* _this = (CView*) GetWindowLong(wnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %u", wnd, msg);
    return rfb::win32::SafeDefWindowProc(wnd, msg, wParam, lParam);
  }

  try {
    result = _this->processMessage(msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
};

HCURSOR dotCursor = (HCURSOR)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDC_DOT_CURSOR), IMAGE_CURSOR, 0, 0, LR_SHARED);
HCURSOR arrowCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED); 

CViewClass::CViewClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = CViewProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_SHARED);
  if (!wndClass.hIcon)
    printf("unable to load icon:%ld", GetLastError());
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = NULL;
  wndClass.lpszMenuName = 0;
  wndClass.lpszClassName = _T("rfb::win32::CViewClass");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register CView window class", GetLastError());
  }
}

CViewClass::~CViewClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

CViewClass baseClass;

//
// -=- FrameClass

//
// Window class used to display the rfb data
//

class FrameClass {
public:
  FrameClass();
  ~FrameClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;

  if (msg == WM_CREATE)
    SetWindowLong(hwnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLong(hwnd, GWL_USERDATA, 0);
  CView* _this = (CView*) GetWindowLong(hwnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %u", hwnd, msg);
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  try {
    result = _this->processFrameMessage(msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
}

FrameClass::FrameClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = FrameProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = 0;
  wndClass.hCursor = NULL;
  wndClass.hbrBackground = 0;
  wndClass.lpszMenuName = 0;
  wndClass.lpszClassName = _T("FrameClass");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register frame window class",
                               GetLastError());
  }
}

FrameClass::~FrameClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

FrameClass frameClass;

//
// -=- CView instance implementation
//

RegKey CView::userConfigKey;


CView::CView() 
  : quit_on_destroy(false), buffer(0), sock(0), readyToRead(false),
    client_size(0, 0, 16, 16), window_size(0, 0, 32, 32),
    cursorVisible(false), cursorAvailable(false), cursorInBuffer(false),
    systemCursorVisible(true), trackingMouseLeave(false),
    hwnd(0), frameHwnd(0), requestUpdate(false), has_focus(false), 
    palette_changed(false), sameMachine(false), encodingChange(false), 
    formatChange(false), lastUsedEncoding_(encodingRaw), fullScreenActive(false),
    bumpScroll(false), manager(0) {

  // Create the main window
  const TCHAR* name = _T("VNC Viewer 4.0b");
  hwnd = CreateWindow((const TCHAR*)baseClass.classAtom, name, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    0, 0, 10, 10, 0, 0, baseClass.instance, this);
  if (!hwnd) {
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", (const char*)CStr(name), hwnd);

  // Create the viewer toolbar
  tb.create(getHandle());
  vlog.debug("created toolbar window \"%s\" (%x)", "ViewerToolBar", tb.getHandle());

  // Create the frame window
  frameHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const TCHAR*)frameClass.classAtom,
    0, WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT, getHandle(), 0, frameClass.instance, this);
  if (!frameHwnd) {
    throw rdr::SystemException("unable to create rfb frame window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", "Frame Window", frameHwnd);

  // Initialise the CPointer pointer handler
  ptr.setHWND(getFrameHandle());
  ptr.setIntervalTimerId(TIMER_POINTER_INTERVAL);
  ptr.set3ButtonTimerId(TIMER_POINTER_3BUTTON);

  // Initialise the bumpscroll timer
  bumpScrollTimer.setHWND(getHandle());
  bumpScrollTimer.setId(TIMER_BUMPSCROLL);

  // Grab AltTab
  setAltTabGrab(options.sendSysKeys);

  // Hook the clipboard
  clipboard.setNotifier(this);

  // Create the backing buffer
  buffer = new win32::DIBSectionBuffer(getFrameHandle());
}

CView::~CView() {
  vlog.debug("~CView");
  showSystemCursor();
  if (hwnd) {
    setVisible(false);
    DestroyWindow(hwnd);
    hwnd = 0;
  }
  delete buffer;
  vlog.debug("~CView done");
}

bool CView::initialise(network::Socket* s) {
  // Update the window menu
  HMENU wndmenu = GetSystemMenu(hwnd, FALSE);
  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  AppendMenu(wndmenu, MF_STRING, IDM_FULLSCREEN, _T("&Full screen"));
  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  AppendMenu(wndmenu, MF_STRING, IDM_CTRL_KEY, _T("Ctr&l"));
  AppendMenu(wndmenu, MF_STRING, IDM_ALT_KEY, _T("Al&t"));
  AppendMenu(wndmenu, MF_STRING, IDM_SEND_CAD, _T("Send Ctrl-Alt-&Del"));
  AppendMenu(wndmenu, MF_STRING, IDM_REQUEST_REFRESH, _T("Refres&h Screen"));
  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  if (manager) AppendMenu(wndmenu, MF_STRING, IDM_NEWCONN, _T("Ne&w Connection..."));
  AppendMenu(wndmenu, MF_STRING, IDM_OPTIONS, _T("&Options..."));
  AppendMenu(wndmenu, MF_STRING, IDM_INFO, _T("Connection &Info..."));
  AppendMenu(wndmenu, MF_STRING, IDM_ABOUT, _T("&About..."));

  // Set the server's name for MRU purposes
  CharArray endpoint(s->getPeerEndpoint());
  setServerName(endpoint.buf);
  if (!options.host.buf)
    options.setHost(endpoint.buf);

  // Initialise the underlying CConnection
  setStreams(&s->inStream(), &s->outStream());

  // Enable processing of window messages while blocked on I/O
  s->inStream().setBlockCallback(this);

  // Initialise the viewer options
  applyOptions(options);

  // - Set which auth schemes we support
  addSecType(secTypeNone);
  addSecType(secTypeVncAuth);

  initialiseProtocol();
  WSAAsyncSelect(s->getFd(), getHandle(), WM_USER, FD_READ | FD_CLOSE);
  sock = s;

  m_fileTransfer.initialize(&s->inStream(), &s->outStream());

  return true;
}


void
CView::applyOptions(CViewOptions& opt) {
  // *** CHANGE THIS TO USE CViewOptions::operator= ***

  // - Take the username, password, config filename, and host spec
  options.setUserName(opt.userName.buf);
  options.setPassword(opt.password.buf);
  options.setHost(opt.host.buf);
  options.setConfigFileName(opt.configFileName.buf);
  options.setMonitor(opt.monitor.buf);

  // - Set optional features in ConnParams
  encodingChange |= ((options.useLocalCursor != opt.useLocalCursor) ||
    (options.useDesktopResize != opt.useDesktopResize));
  cp.supportsLocalCursor = options.useLocalCursor = opt.useLocalCursor;
  cp.supportsDesktopResize = options.useDesktopResize = opt.useDesktopResize;

  encodingChange |= ((options.customCompressLevel != opt.customCompressLevel) ||
		     (options.compressLevel != opt.compressLevel) ||
		     (options.noJpeg != opt.noJpeg) ||
		     (options.qualityLevel != opt.qualityLevel));
  cp.customCompressLevel = options.customCompressLevel = opt.customCompressLevel;
  cp.compressLevel = options.compressLevel = opt.compressLevel;
  cp.noJpeg = options.noJpeg = opt.noJpeg;
  cp.qualityLevel = options.qualityLevel = opt.qualityLevel;

  if (cursorAvailable)
    hideLocalCursor();
  cursorAvailable = cursorAvailable && options.useLocalCursor;

  // - Switch full-screen mode on/off
  options.fullScreen = opt.fullScreen;
  setFullscreen(options.fullScreen);

  // - Handle format/encoding options
  encodingChange |= (options.preferredEncoding != opt.preferredEncoding);
  options.preferredEncoding = opt.preferredEncoding;

  formatChange |= (options.fullColour != opt.fullColour);
  options.fullColour = opt.fullColour;

  if (!options.fullColour)
    formatChange |= (options.lowColourLevel != opt.lowColourLevel);
  options.lowColourLevel = opt.lowColourLevel;

  options.autoSelect = opt.autoSelect;

  // - Sharing
  options.shared = opt.shared;
  setShared(options.shared);

  // - Inputs
  options.sendPtrEvents = opt.sendPtrEvents;
  options.sendKeyEvents = opt.sendKeyEvents;
  options.sendSysKeys = opt.sendSysKeys;
  setAltTabGrab(options.sendSysKeys);
  options.clientCutText = opt.clientCutText;
  options.serverCutText = opt.serverCutText;
  options.emulate3 = opt.emulate3;
  ptr.enableEmulate3(opt.emulate3);
  options.pointerEventInterval = opt.pointerEventInterval;
  ptr.enableInterval(opt.pointerEventInterval);
  options.menuKey = opt.menuKey;

  // - Protocol version override
  options.protocol3_3 = opt.protocol3_3;
  setProtocol3_3(options.protocol3_3);

  // - Bell
  options.acceptBell = opt.acceptBell;
}

void
CView::setFullscreen(bool fs) {
  // Set the menu fullscreen option tick
  CheckMenuItem(GetSystemMenu(getHandle(), FALSE), IDM_FULLSCREEN,
    (options.fullScreen ? MF_CHECKED : 0) | MF_BYCOMMAND);

  // If the window is not visible then we ignore the request.
  // setVisible() will call us to correct the full-screen state when
  // the window is visible, to keep things consistent.
  if (!IsWindowVisible(getHandle()))
    return;

  if (fs && !fullScreenActive) {
    fullScreenActive = bumpScroll = true;

    // Un-minimize the window if required
    if (GetWindowLong(getHandle(), GWL_STYLE) & WS_MINIMIZE)
      ShowWindow(getHandle(), SW_RESTORE);

    // Save the non-fullscreen window position
    RECT wrect;
    GetWindowRect(getHandle(), &wrect);
    fullScreenOldRect = Rect(wrect.left, wrect.top, wrect.right, wrect.bottom);

    // Find the size of the display the window is on
    MonitorInfo mi(getHandle());

    // Set the window full-screen
    DWORD flags = GetWindowLong(getHandle(), GWL_STYLE);
    fullScreenOldFlags = flags;
    flags = flags & ~(WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZE | WS_MINIMIZE);
    vlog.debug("flags=%x", flags);

    tb.hide();
    SetWindowLong(getFrameHandle(), GWL_EXSTYLE, 0);
    SetWindowLong(getHandle(), GWL_STYLE, flags);
    SetWindowPos(getHandle(), HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
      mi.rcMonitor.right-mi.rcMonitor.left,
      mi.rcMonitor.bottom-mi.rcMonitor.top,
      SWP_FRAMECHANGED);
  } else if (!fs && fullScreenActive) {
    fullScreenActive = bumpScroll = false;

    // Set the window non-fullscreen
    tb.show();
    SetWindowLong(getFrameHandle(), GWL_EXSTYLE, WS_EX_CLIENTEDGE);
    SetWindowLong(getHandle(), GWL_STYLE, fullScreenOldFlags);
    SetWindowPos(getHandle(), HWND_NOTOPMOST,
      fullScreenOldRect.tl.x, fullScreenOldRect.tl.y,
      fullScreenOldRect.width(), fullScreenOldRect.height(),
      SWP_FRAMECHANGED);
  }

  // Adjust the viewport offset to cope with change in size between FS
  // and previous window state.
  setViewportOffset(scrolloffset);
}


bool CView::setViewportOffset(const Point& tl) {
/* ***
  Point np = Point(max(0, min(maxscrolloffset.x, tl.x)),
    max(0, min(maxscrolloffset.y, tl.y)));
    */
  Point np = Point(max(0, min(tl.x, buffer->width()-client_size.width())),
    max(0, min(tl.y, buffer->height()-client_size.height())));
  Point delta = np.translate(scrolloffset.negate());
  if (!np.equals(scrolloffset)) {
    scrolloffset = np;
    ScrollWindowEx(getFrameHandle(), -delta.x, -delta.y, 0, 0, 0, 0, SW_INVALIDATE);
    UpdateWindow(getFrameHandle());
    return true;
  }
  return false;
}


bool CView::processBumpScroll(const Point& pos)
{
  if (!bumpScroll) return false;
  int bumpScrollPixels = 20;
  bumpScrollDelta = Point();

  if (pos.x == client_size.width()-1)
    bumpScrollDelta.x = bumpScrollPixels;
  else if (pos.x == 0)
    bumpScrollDelta.x = -bumpScrollPixels;
  if (pos.y == client_size.height()-1)
    bumpScrollDelta.y = bumpScrollPixels;
  else if (pos.y == 0)
    bumpScrollDelta.y = -bumpScrollPixels;

  if (bumpScrollDelta.x || bumpScrollDelta.y) {
    if (bumpScrollTimer.isActive()) return true;
    if (setViewportOffset(scrolloffset.translate(bumpScrollDelta))) {
      bumpScrollTimer.start(25);
      return true;
    }
  }

  bumpScrollTimer.stop();
  return false;
}


LRESULT
CView::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

    // -=- Process standard window messages

  case WM_DISPLAYCHANGE:
    // Display has changed - use new pixel format
    calculateFullColourPF();
    break;

    // -=- Window position

    // Prevent the window from being resized to be too large if in normal mode.
    // If maximized or fullscreen the allow oversized windows.

  case WM_WINDOWPOSCHANGING:
    {
      WINDOWPOS* wpos = (WINDOWPOS*)lParam;
      if (wpos->flags &  SWP_NOSIZE)
        break;

      // Work out how big the window should ideally be
      DWORD frame_current_style = GetWindowLong(getFrameHandle(), GWL_STYLE);
      DWORD frame_style = frame_current_style & ~(WS_VSCROLL | WS_HSCROLL);
      DWORD frame_ex_style = GetWindowLong(getFrameHandle(), GWL_EXSTYLE);

      RECT r;
      SetRect(&r, 0, 0, buffer->width(), buffer->height());
      AdjustWindowRectEx(&r, frame_style, FALSE, frame_ex_style);
      Rect reqd_size = Rect(r.left, r.top, r.right, r.bottom);
      if (frame_current_style & WS_VSCROLL)
        reqd_size.br.x += GetSystemMetrics(SM_CXVSCROLL);
      if (frame_current_style & WS_HSCROLL)
        reqd_size.br.y += GetSystemMetrics(SM_CXHSCROLL);
      r.left = reqd_size.tl.x; r.right  = reqd_size.br.x;
      r.top  = reqd_size.tl.y; r.bottom = reqd_size.br.y;
      if (tb.isVisible()) r.bottom += tb.getHeight();
      AdjustWindowRect(&r, GetWindowLong(getHandle(), GWL_STYLE), FALSE);
      reqd_size = Rect(r.left, r.top, r.right, r.bottom);
      RECT current;
      GetWindowRect(getHandle(), &current);

      // Ensure that the window isn't resized too large
      // If the window is maximized or full-screen then any size is allowed
      if (!(GetWindowLong(getHandle(), GWL_STYLE) & WS_MAXIMIZE) && !fullScreenActive) {
        if (wpos->cx > reqd_size.width()) {
          wpos->cx = reqd_size.width();
          wpos->x = current.left;
        }
        if (wpos->cy > reqd_size.height()) {
          wpos->cy = reqd_size.height();
          wpos->y = current.top;
        }
      }

    }
    break;

    // Resize child windows and update window size info we have cached.

  case WM_SIZE:
    {
      Point old_offset = bufferToClient(Point(0, 0));

      // Resize the child windows
      RECT r;
      GetClientRect(getHandle(), &r);
      if (tb.isVisible()) {
        MoveWindow(getFrameHandle(), 0, tb.getHeight(), r.right,
          r.bottom - tb.getHeight(), TRUE);
      } else {
        MoveWindow(getFrameHandle(), 0, 0, r.right, r.bottom, TRUE);
      }
      tb.autoSize();
 
      // Update the cached sizing information
      GetWindowRect(getFrameHandle(), &r);
      window_size = Rect(r.left, r.top, r.right, r.bottom);
      GetClientRect(getFrameHandle(), &r);
      client_size = Rect(r.left, r.top, r.right, r.bottom);

      // Determine whether scrollbars are required
      calculateScrollBars();
         
      // Redraw if required
      if (!old_offset.equals(bufferToClient(Point(0, 0))))
        InvalidateRect(getFrameHandle(), 0, TRUE);
    }
    break;

    // -=- Bump-scrolling

  case WM_TIMER:
    switch (wParam) {
    case TIMER_BUMPSCROLL:
      if (!setViewportOffset(scrolloffset.translate(bumpScrollDelta)))
        bumpScrollTimer.stop();
      break;
    case TIMER_POINTER_INTERVAL:
    case TIMER_POINTER_3BUTTON:
      try {
        ptr.handleTimer(writer(), wParam);
      } catch (rdr::Exception& e) {
        close(e.str());
      }
      break;
    }
    break;

    // -=- Track whether or not the window has focus

  case WM_SETFOCUS:
    has_focus = true;
    // Re-register AltTab hotkey
    setAltTabGrab(options.sendSysKeys);
    break;
  case WM_KILLFOCUS:
    has_focus = false;
    cursorOutsideBuffer();
    // Unregister AltTab hotkey
    setAltTabGrab(false);
    // Restore the remote keys to consistent states
    try {
      kbd.releaseAllKeys(writer());
    } catch (rdr::Exception& e) {
      close(e.str());
    }
    break;

    // -=- Handle the extra window menu items

    // Process the items added to the system menu
  case WM_SYSCOMMAND:

    // - First check whether it's one of our messages
    switch (wParam) {
    case IDM_FULLSCREEN:
      options.fullScreen = !options.fullScreen;
      setFullscreen(options.fullScreen);
      return 0;
    case IDM_CTRL_KEY:
      writeKeyEvent(VK_CONTROL, 0, !kbd.keyPressed(VK_CONTROL));
      return 0;
    case IDM_ALT_KEY:
      writeKeyEvent(VK_MENU, 0, !kbd.keyPressed(VK_MENU));
      return 0;
    case IDM_SEND_MENU_KEY:
      writeKeyEvent(options.menuKey, 0, true);
      writeKeyEvent(options.menuKey, 0, false);
      return 0;
    case IDM_SEND_CAD:
      writeKeyEvent(VK_CONTROL, 0, true);
      writeKeyEvent(VK_MENU, 0, true);
      writeKeyEvent(VK_DELETE, 0, true);
      writeKeyEvent(VK_DELETE, 0, false);
      writeKeyEvent(VK_MENU, 0, false);
      writeKeyEvent(VK_CONTROL, 0, false);
      return 0;
    case IDM_SEND_CTLESC:
      return 0;
    case IDM_REQUEST_REFRESH:
      try {
        writer()->writeFramebufferUpdateRequest(Rect(0,0,cp.width,cp.height), false);
        requestUpdate = false;
      } catch (rdr::Exception& e) {
        close(e.str());
      }
      return 0;
    case IDM_NEWCONN:
      manager->addClient(0);
      return 0;
    case IDM_OPTIONS:
      // Update the monitor device name in the CViewOptions instance
      {
        MonitorInfo mi(getHandle());
        options.setMonitor(mi.szDevice);
        optionsDialog.showDialog(this);
        return 0;
      }
    case IDM_INFO:
      infoDialog.showDialog(this);
      return 0;
    case IDM_ABOUT:
      AboutDialog::instance.showDialog();
      return 0;
    case IDM_FILE_TRANSFER:
      m_fileTransfer.show(getHandle());
      return 0;
    case IDM_CONN_SAVE_AS:
      return 0;
    case ID_CLOSE:
      PostQuitMessage(0);
      return 0;
    };

    // - Not one of our messages, so process it as a system message
    switch (wParam & 0xfff0) {

      // When restored, ensure that full-screen mode is re-enabled if required.
    case SC_RESTORE:
      rfb::win32::SafeDefWindowProc(getHandle(), msg, wParam, lParam);
      setFullscreen(options.fullScreen);
      return 0;

      // If we are maximized or minimized then that cancels full-screen mode.
    case SC_MINIMIZE:
    case SC_MAXIMIZE:
      setFullscreen(false);
      break;

      // If the system menu is shown then make sure it's up to date
    case SC_KEYMENU:
    case SC_MOUSEMENU:
      updateF8Menu(false);
      break;

    };
    break;

    // Treat all menu commands as system menu commands
  case WM_COMMAND:
    SendMessage(getHandle(), WM_SYSCOMMAND, wParam, lParam);
    return 0;

  case WM_MENUCHAR:
    vlog.debug("menuchar");
    break;

    // -=- Handle keyboard input

  case WM_HOTKEY:
    if (wParam == HOTKEY_ALTTAB) {
      writeKeyEvent(VK_TAB, 0, true);
      writeKeyEvent(VK_TAB, 0, false);
      return 0;
    }
    break;

  case WM_SYSKEYDOWN:
    // Translate Alt-Space and Alt-F4 to WM_SYSCHAR and WM_CLOSE,
    // since we are not using TranslateMessage(). 
    if (!options.sendSysKeys) {
      switch (wParam) {
      case VK_SPACE:
	writeKeyEvent(VK_MENU, 0, false);
	SendMessage(getHandle(), WM_SYSCHAR, wParam, lParam);
	return 0; 
      case VK_F4:
	SendMessage(getHandle(), WM_CLOSE, wParam, lParam);
	return 0; 
      }
    }

  case WM_SYSKEYUP:
    // When we have registered for AltTab as a hotkey, SYSKEYUPs for
    // Tabs are sent (but no SYSKEYDOWNs). AltTab is handled by
    // WM_HOTKEY, though.
    if (wParam == VK_TAB) return 0; 

  case WM_KEYUP:
  case WM_KEYDOWN:
    // Hook the MenuKey to pop-up the window menu
    if (options.menuKey && (wParam == options.menuKey)) {

      bool ctrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
      bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
      bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
      if (!(ctrlDown || altDown || shiftDown)) {

        // If MenuKey is being released then pop-up the menu
        if ((msg == WM_KEYDOWN)) {
          // Make sure it's up to date
          updateF8Menu(true);

          // Show it under the pointer
          POINT pt;
          GetCursorPos(&pt);
          cursorInBuffer = false;
          TrackPopupMenu(GetSystemMenu(getHandle(), FALSE),
            TPM_CENTERALIGN | TPM_VCENTERALIGN, pt.x, pt.y, 0, getHandle(), 0);
        }

	// Ignore the MenuKey keypress for both press & release events
	return 0;
      }
    }

    writeKeyEvent(wParam, lParam, (msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN));
    return 0;

    // -=- Handle the window closing

  case WM_CLOSE:
    vlog.debug("WM_CLOSE %x", getHandle());
    if (quit_on_destroy) {
      vlog.debug("posting WM_QUIT");
      PostQuitMessage(0);
    } else {
      vlog.debug("not posting WM_QUIT");
    }
    break;

    // -=- Process incoming socket data

  case WM_USER:
    readyToRead = true;
    break;

  }

  return rfb::win32::SafeDefWindowProc(getHandle(), msg, wParam, lParam);
}

LRESULT CView::processFrameMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

    // -=- Palette management

  case WM_PALETTECHANGED:
    vlog.debug("WM_PALETTECHANGED");
    if ((HWND)wParam == getFrameHandle()) {
      vlog.debug("ignoring");
      break;
    }
  case WM_QUERYNEWPALETTE:
    vlog.debug("re-selecting palette");
    {
      WindowDC wdc(getFrameHandle());
      PaletteSelector pSel(wdc, windowPalette.getHandle());
      if (pSel.isRedrawRequired()) {
        InvalidateRect(getFrameHandle(), 0, FALSE);
        UpdateWindow(getFrameHandle());
      }
    }
    return TRUE;

    // Paint the remote frame buffer

  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC paintDC = BeginPaint(getFrameHandle(), &ps);
      if (!paintDC)
        throw SystemException("unable to BeginPaint", GetLastError());
      Rect pr = Rect(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

      if (!pr.is_empty()) {

        // Draw using the correct palette
        PaletteSelector pSel(paintDC, windowPalette.getHandle());

        if (buffer->bitmap) {
          // Update the bitmap's palette
          if (palette_changed) {
            palette_changed = false;
            buffer->refreshPalette();
          }

          // Get device context
          BitmapDC bitmapDC(paintDC, buffer->bitmap);

          // Blit the border if required
          Rect bufpos = bufferToClient(buffer->getRect());
          if (!pr.enclosed_by(bufpos)) {
            vlog.debug("draw border");
            HBRUSH black = (HBRUSH) GetStockObject(BLACK_BRUSH);
            RECT r;
            SetRect(&r, 0, 0, bufpos.tl.x, client_size.height()); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.tl.x, 0, bufpos.br.x, bufpos.tl.y); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.br.x, 0, client_size.width(), client_size.height()); FillRect(paintDC, &r, black);
            SetRect(&r, bufpos.tl.x, bufpos.br.y, bufpos.br.x, client_size.height()); FillRect(paintDC, &r, black);
          }

          // Do the blit
          Point buf_pos = clientToBuffer(pr.tl);
          if (!BitBlt(paintDC, pr.tl.x, pr.tl.y, pr.width(), pr.height(),
            bitmapDC, buf_pos.x, buf_pos.y, SRCCOPY))
            throw SystemException("unable to BitBlt to window", GetLastError());

        } else {
          // Blit a load of black
          if (!BitBlt(paintDC, pr.tl.x, pr.tl.y, pr.width(), pr.height(),
            0, 0, 0, BLACKNESS))
            throw SystemException("unable to BitBlt to blank window", GetLastError());
        }
      }

      EndPaint(getFrameHandle(), &ps);

      // - Request the next update from the server, if required
      requestNewUpdate();
    }
    return 0;

    // Process the frame scroll messages

  case WM_VSCROLL:
  case WM_HSCROLL: 
    {
      Point delta;
      int newpos = (msg == WM_VSCROLL) ? scrolloffset.y : scrolloffset.x;

      switch (LOWORD(wParam)) {
      case SB_PAGEUP: newpos -= 50; break;
      case SB_PAGEDOWN: newpos += 50; break;
      case SB_LINEUP: newpos -= 5; break;
      case SB_LINEDOWN: newpos += 5; break;
      case SB_THUMBTRACK:
      case SB_THUMBPOSITION: newpos = HIWORD(wParam); break;
      default: vlog.info("received unknown scroll message");
      };

      if (msg == WM_HSCROLL)
        setViewportOffset(Point(newpos, scrolloffset.y));
      else
        setViewportOffset(Point(scrolloffset.x, newpos));
  
      SCROLLINFO si;
      si.cbSize = sizeof(si); 
      si.fMask  = SIF_POS; 
      si.nPos   = newpos; 
      SetScrollInfo(getFrameHandle(), (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ, &si, TRUE); 
    }
    break;

    // -=- Cursor shape/visibility handling

  case WM_SETCURSOR:
    if (LOWORD(lParam) != HTCLIENT)
      break;
    SetCursor(cursorInBuffer ? dotCursor : arrowCursor);
    return TRUE;

  case WM_MOUSELEAVE:
    trackingMouseLeave = false;
    cursorOutsideBuffer();
    return 0;

    // -=- Mouse input handling

  case WM_MOUSEMOVE:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MOUSEWHEEL:
    if (has_focus)
    {
      if (!trackingMouseLeave) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = getFrameHandle();
        _TrackMouseEvent(&tme);
        trackingMouseLeave = true;
      }
      int mask = 0;
      if (LOWORD(wParam) & MK_LBUTTON) mask |= 1;
      if (LOWORD(wParam) & MK_MBUTTON) mask |= 2;
      if (LOWORD(wParam) & MK_RBUTTON) mask |= 4;

      if (msg == WM_MOUSEWHEEL) {
        int delta = (short)HIWORD(wParam);
        int repeats = (abs(delta)+119) / 120;
        int wheelMask = (delta > 0) ? 8 : 16;
        vlog.debug("repeats %d, mask %d\n",repeats,wheelMask);
        for (int i=0; i<repeats; i++) {
          writePointerEvent(oldpos.x, oldpos.y, mask | wheelMask);
          writePointerEvent(oldpos.x, oldpos.y, mask);
        }
      } else {
        Point clientPos = Point(LOWORD(lParam), HIWORD(lParam));
        Point p = clientToBuffer(clientPos);

        // If the mouse is not within the server buffer area, do nothing
        cursorInBuffer = buffer->getRect().contains(p);
        if (!cursorInBuffer) {
          cursorOutsideBuffer();
          break;
        }

        // If we're locally rendering the cursor then redraw it
        if (cursorAvailable) {
          // - Render the cursor!
          if (!p.equals(cursorPos)) {
            hideLocalCursor();
            cursorPos = p;
            showLocalCursor();
            if (cursorVisible)
              hideSystemCursor();
          }
        }

        // If we are doing bump-scrolling then try that first...
        if (processBumpScroll(clientPos))
          break;

        // Send a pointer event to the server
        writePointerEvent(p.x, p.y, mask);
        oldpos = p;
      }
    } else {
      cursorOutsideBuffer();
    }
    break;
  }

  return rfb::win32::SafeDefWindowProc(getFrameHandle(), msg, wParam, lParam);
}

void CView::blockCallback() {
  // - An InStream has blocked on I/O while processing an RFB message
  //   We re-enable socket event notifications, so we'll know when more
  //   data is available, then we sit and dispatch window events until
  //   the notification arrives.
  readyToRead = false;
  WSAAsyncSelect(sock->getFd(), getHandle(), WM_USER, FD_READ | FD_CLOSE);
  MSG msg;
  while (true) {
    if (readyToRead) {
      // - Network event notification.  Return control to I/O routine.
      WSAAsyncSelect(sock->getFd(), getHandle(), WM_USER, 0);
      return;
    }

    DWORD result = GetMessage(&msg, NULL, 0, 0);
    if (result == 0) {
      vlog.debug("WM_QUIT");
      throw QuitMessage(msg.wParam);
    } else if (result < 0) {
      throw rdr::SystemException("GetMessage error", GetLastError());
    }

    // IMPORTANT: We mustn't call TranslateMessage() here, because instead we
    // call ToAscii() in CKeyboard::keyEvent().  ToAscii() stores dead key
    // state from one call to the next, which would be messed up by calls to
    // TranslateMessage() (actually it looks like TranslateMessage() calls
    // ToAscii() internally).
    DispatchMessage(&msg);
  }
}


void
CView::hideLocalCursor() {
  // - Blit the cursor backing store over the cursor
  // *** ALWAYS call this BEFORE changing buffer PF!!!
  if (cursorVisible) {
    cursorVisible = false;
    buffer->imageRect(cursorBackingRect, cursorBacking.data);
    invalidateBufferRect(cursorBackingRect);
  }
}

void
CView::showLocalCursor() {
  if (cursorAvailable && !cursorVisible && cursorInBuffer) {
    if (!cp.pf().equal(cursor.getPF()) ||
      cursor.getRect().is_empty()) {
      vlog.info("attempting to render invalid local cursor");
      cursorAvailable = false;
      showSystemCursor();
      return;
    }
    cursorVisible = true;
    
    cursorBackingRect = cursor.getRect().translate(cursorPos).translate(cursor.hotspot.negate());
    cursorBackingRect = cursorBackingRect.intersect(buffer->getRect());
    buffer->getImage(cursorBacking.data, cursorBackingRect);

    renderLocalCursor();

    invalidateBufferRect(cursorBackingRect);
  }
}

void CView::cursorOutsideBuffer()
{
  cursorInBuffer = false;
  hideLocalCursor();
  showSystemCursor();
}

void
CView::renderLocalCursor()
{
  Rect r = cursor.getRect();
  r = r.translate(cursorPos).translate(cursor.hotspot.negate());
  buffer->maskRect(r, cursor.data, cursor.mask.buf);
}

void
CView::hideSystemCursor() {
  if (systemCursorVisible) {
    vlog.debug("hide system cursor");
    systemCursorVisible = false;
    ShowCursor(FALSE);
  }
}

void
CView::showSystemCursor() {
  if (!systemCursorVisible) {
    vlog.debug("show system cursor");
    systemCursorVisible = true;
    ShowCursor(TRUE);
  }
}

void
CView::setAltTabGrab(bool grab) {
  BOOL hotKeyResult;
  static bool grabstate = false;

  // Do not call RegisterHotKey/UnregisterHotKey if not necessary
  if (grabstate == grab)
    return;

  grabstate = grab;
    
  // Only works for NT/2k/XP
  if (grab) {
    hotKeyResult = RegisterHotKey(hwnd, HOTKEY_ALTTAB, MOD_ALT, VK_TAB);
    if (!hotKeyResult)
      vlog.debug("RegisterHotkey failed with error %d", GetLastError());
  } else {
    hotKeyResult = UnregisterHotKey(hwnd, HOTKEY_ALTTAB);
  }
}

bool
CView::invalidateBufferRect(const Rect& crect) {
  Rect rect = bufferToClient(crect);
  if (rect.intersect(client_size).is_empty()) return false;
  RECT invalid = {rect.tl.x, rect.tl.y, rect.br.x, rect.br.y};
  InvalidateRect(getFrameHandle(), &invalid, FALSE);
  return true;
}


void
CView::notifyClipboardChanged(const char* text, int len) {
  if (!options.clientCutText) return;
  if (state() != RFBSTATE_NORMAL) return;
  try {
    writer()->writeClientCutText(text, len);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}


CSecurity* CView::getCSecurity(int secType)
{
  switch (secType) {
  case secTypeNone:
    return new CSecurityNone();
  case secTypeVncAuth:
    return new CSecurityVncAuth(this);
  default:
    throw Exception("Unsupported secType?");
  }
}


void
CView::setColourMapEntries(int first, int count, U16* rgbs) {
  vlog.debug("setColourMapEntries: first=%d, count=%d", first, count);
  int i;
  for (i=0;i<count;i++) {
    buffer->setColour(i+first, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);
  }
  // *** change to 0, 256?
  refreshWindowPalette(first, count);
  palette_changed = true;
  InvalidateRect(getHandle(), 0, FALSE);
}

void
CView::bell() {
  if (options.acceptBell)
    MessageBeep(-1);
}


void
CView::setDesktopSize(int w, int h) {
  vlog.debug("setDesktopSize %dx%d", w, h);

  // If the locally-rendered cursor is visible then remove it
  hideLocalCursor();

  // Resize the backing buffer
  buffer->setSize(w, h);

  // If the window is not maximised or full-screen then resize it
  if (!(GetWindowLong(getHandle(), GWL_STYLE) & WS_MAXIMIZE) && !fullScreenActive) {
    // Resize the window to the required size
    RECT r = {0, 0, w, h};
    AdjustWindowRectEx(&r, GetWindowLong(getFrameHandle(), GWL_STYLE), FALSE,
      GetWindowLong(getFrameHandle(), GWL_EXSTYLE));
    if (tb.isVisible()) r.bottom += tb.getHeight();
    AdjustWindowRect(&r, GetWindowLong(getHandle(), GWL_STYLE), FALSE);
    SetWindowPos(getHandle(), 0, 0, 0, r.right-r.left, r.bottom-r.top,
      SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

    // Move the window to the desired monitor
    if (options.monitor.buf)
      moveToMonitor(getHandle(), options.monitor.buf);

    // Clip to the system work area
    centerWindow(getHandle(), 0, true);
  } else {
    // Ensure the screen contents are consistent
    InvalidateRect(getFrameHandle(), 0, FALSE);
  }

  // Tell the underlying CConnection
  CConnection::setDesktopSize(w, h);

  // Enable/disable scrollbars as appropriate
  calculateScrollBars();
}

void
CView::setCursor(const Point& hotspot, const Point& size, void* data, void* mask) {
  if (!options.useLocalCursor) return;
  hideLocalCursor();

  cursor.hotspot = hotspot;

  cursor.setSize(size.x, size.y);
  cursor.setPF(cp.pf());
  cursor.imageRect(cursor.getRect(), data);
  memcpy(cursor.mask.buf, mask, cursor.maskLen());
  cursor.crop();

  cursorBacking.setSize(size.x, size.y);
  cursorBacking.setPF(cp.pf());

  cursorAvailable = true;

  showLocalCursor();
}

PixelFormat
CView::getNativePF() const {
  vlog.debug("getNativePF()");
  return WindowDC(getHandle()).getPF();
}

void
CView::setVisible(bool visible) {
  ShowWindow(getHandle(), visible ? SW_SHOW : SW_HIDE);
  if (visible) {
    // When the window becomes visible, make it active
    SetForegroundWindow(getHandle());
    SetActiveWindow(getHandle());
    // If the window should be full-screen, then do so
    setFullscreen(options.fullScreen);
  } else {
    // Disable full-screen mode
    setFullscreen(false);
  }
}

void
CView::close(const char* reason) {
  setVisible(false);
  if (reason) {
    vlog.info("closing - %s", reason);
    MsgBox(NULL, TStr(reason), MB_ICONINFORMATION | MB_OK);
  }
  SendMessage(getHandle(), WM_CLOSE, 0, 0);
}


void
CView::framebufferUpdateEnd() {
  if (debugDelay != 0) {
    vlog.debug("debug delay %d",(int)debugDelay);
    UpdateWindow(getHandle());
    Sleep(debugDelay);
    std::list<rfb::Rect>::iterator i;
    for (i = debugRects.begin(); i != debugRects.end(); i++) {
      invertRect(*i);
    }
    debugRects.clear();
  }
  if (options.autoSelect)
    autoSelectFormatAndEncoding();

  // Always request the next update
  requestUpdate = true;

  // Check that at least part of the window has changed
  if (!GetUpdateRect(getHandle(), 0, FALSE)) {
    if (!(GetWindowLong(getHandle(), GWL_STYLE) & WS_MINIMIZE))
      requestNewUpdate();
  }

  showLocalCursor();
}


// Note: The method below is duplicated in vncviewer_unix/CConn.cxx!

// autoSelectFormatAndEncoding() chooses the format and encoding appropriate
// to the connection speed:
//
//   Above 16Mbps (timing for at least a second), switch to hextile
//   Otherwise, switch to ZRLE
//
//   Above 256Kbps, use full colour mode
//
void 
CView::autoSelectFormatAndEncoding() {
  int kbitsPerSecond = sock->inStream().kbitsPerSecond();
  unsigned int newEncoding = options.preferredEncoding;
  bool newFullColour = options.fullColour;
  unsigned int timeWaited = sock->inStream().timeWaited();

  // Select best encoding
  if (kbitsPerSecond > 16000 && timeWaited >= 10000) {
    newEncoding = encodingHextile;
  } else {
    newEncoding = encodingZRLE;
  }

  if (newEncoding != options.preferredEncoding) {
    vlog.info("Throughput %d kbit/s - changing to %s encoding",
              kbitsPerSecond, encodingName(newEncoding));
    options.preferredEncoding = newEncoding;
    encodingChange = true;
  }

  if (kbitsPerSecond == 0) {
    return;
  }

  if (cp.beforeVersion(3, 8)) {
    // Xvnc from TightVNC 1.2.9 sends out FramebufferUpdates with
    // cursors "asynchronously". If this happens in the middle of a
    // pixel format change, the server will encode the cursor with
    // the old format, but the client will try to decode it
    // according to the new format. This will lead to a
    // crash. Therefore, we do not allow automatic format change for
    // old servers.
    return;
  }
  
  // Select best color level
  newFullColour = (kbitsPerSecond > 256);
  if (newFullColour != options.fullColour) {
    vlog.info("Throughput %d kbit/s - full color is now %s", 
	      kbitsPerSecond,
	      newFullColour ? "enabled" : "disabled");
    options.fullColour = newFullColour;
    formatChange = true;
  } 
}

void
CView::requestNewUpdate() {
  if (!requestUpdate) return;

  if (formatChange) {
    // Hide the rendered cursor, if any, to prevent
    // the backing buffer being used in the wrong format
    hideLocalCursor();

    // Select the required pixel format
    if (options.fullColour) {
      buffer->setPF(fullColourPF);
    } else {
      switch (options.lowColourLevel) {
      case 0:
        buffer->setPF(PixelFormat(8,3,0,1,1,1,1,2,1,0));
        break;
      case 1:
        buffer->setPF(PixelFormat(8,6,0,1,3,3,3,4,2,0));
        break;
      case 2:
        buffer->setPF(PixelFormat(8,8,0,0,0,0,0,0,0,0));
        break;
      }
    }

    // Print the current pixel format
    char str[256];
    buffer->getPF().print(str, 256);
    vlog.info("Using pixel format %s",str);

    // Save the connection pixel format and tell server to use it
    cp.setPF(buffer->getPF());
    writer()->writeSetPixelFormat(cp.pf());

    // Correct the local window's palette
    if (!getNativePF().trueColour)
      refreshWindowPalette(0, 1 << cp.pf().depth);
  }

  if (encodingChange) {
    vlog.info("Using %s encoding",encodingName(options.preferredEncoding));
    writer()->writeSetEncodings(options.preferredEncoding, true);
  }

  writer()->writeFramebufferUpdateRequest(Rect(0, 0, cp.width, cp.height),
                                          !formatChange);

  encodingChange = formatChange = requestUpdate = false;
}


void
CView::writeKeyEvent(rdr::U8 vkey, rdr::U32 flags, bool down) {
  if (!options.sendKeyEvents) return;
  try {
    kbd.keyEvent(writer(), vkey, flags, down);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}

void
CView::writePointerEvent(int x, int y, int buttonMask) {
  if (!options.sendPtrEvents) return;
  try {
    ptr.pointerEvent(writer(), x, y, buttonMask);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}


void
CView::refreshWindowPalette(int start, int count) {
  vlog.debug("refreshWindowPalette(%d, %d)", start, count);

  Colour colours[256];
  if (count > 256) {
    vlog.debug("%d palette entries", count);
    throw rdr::Exception("too many palette entries");
  }

  // Copy the palette from the DIBSectionBuffer
  ColourMap* cm = buffer->getColourMap();
  if (!cm) return;
  for (int i=0; i<count; i++) {
    int r, g, b;
    cm->lookup(i, &r, &g, &b);
    colours[i].r = r;
    colours[i].g = g;
    colours[i].b = b;
  }

  // Set the window palette
  windowPalette.setEntries(start, count, colours);

  // Cause the window to be redrawn
  InvalidateRect(getHandle(), 0, 0);
}


void CView::calculateScrollBars() {
  // Calculate the required size of window
  DWORD current_style = GetWindowLong(getFrameHandle(), GWL_STYLE);
  DWORD style = current_style & ~(WS_VSCROLL | WS_HSCROLL);
  DWORD ex_style = GetWindowLong(getFrameHandle(), GWL_EXSTYLE);
  DWORD old_style;
  RECT r;
  SetRect(&r, 0, 0, buffer->width(), buffer->height());
  AdjustWindowRectEx(&r, style, FALSE, ex_style);
  Rect reqd_size = Rect(r.left, r.top, r.right, r.bottom);

  if (!bumpScroll) {
    // We only enable scrollbars if bump-scrolling is not active.
    // Effectively, this means if full-screen is not active,
    // but I think it's better to make these things explicit.
    // Work out whether scroll bars are required
    do {
      old_style = style;

      if (!(style & WS_HSCROLL) && (reqd_size.width() > window_size.width())) {
        style |= WS_HSCROLL;
        reqd_size.br.y += GetSystemMetrics(SM_CXHSCROLL);
      }
      if (!(style & WS_VSCROLL) && (reqd_size.height() > window_size.height())) {
        style |= WS_VSCROLL;
        reqd_size.br.x += GetSystemMetrics(SM_CXVSCROLL);
      }
    } while (style != old_style);
  }
  
  // Tell Windows to update the window style & cached settings
  if (style != current_style) {
    SetWindowLong(getFrameHandle(), GWL_STYLE, style);
    SetWindowPos(getFrameHandle(), NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
  }

  // Update the scroll settings
  SCROLLINFO si;
  if (style & WS_VSCROLL) {
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = buffer->height();
    si.nPage  = buffer->height() - (reqd_size.height() - window_size.height());
    maxscrolloffset.y = max(0, si.nMax-si.nPage);
    scrolloffset.y = min(maxscrolloffset.y, scrolloffset.y);
    si.nPos   = scrolloffset.y;
    SetScrollInfo(getFrameHandle(), SB_VERT, &si, TRUE);
  }
  if (style & WS_HSCROLL) {
    si.cbSize = sizeof(si);
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;
    si.nMax   = buffer->width();
    si.nPage  = buffer->width() - (reqd_size.width() - window_size.width());
    maxscrolloffset.x = max(0, si.nMax-si.nPage);
    scrolloffset.x = min(maxscrolloffset.x, scrolloffset.x);
    si.nPos   = scrolloffset.x;
    SetScrollInfo(getFrameHandle(), SB_HORZ, &si, TRUE);
  }

  // Update the cached client size
  GetClientRect(getFrameHandle(), &r);
  client_size = Rect(r.left, r.top, r.right, r.bottom);
}


void
CView::calculateFullColourPF() {
  // If the server is palette based then use palette locally
  // Also, don't bother doing bgr222
  if (!serverDefaultPF.trueColour || (serverDefaultPF.depth < 6)) {
    fullColourPF = serverDefaultPF;
    options.fullColour = true;
  } else {
    // If server is trueColour, use lowest depth PF
    PixelFormat native = getNativePF();
    if ((serverDefaultPF.bpp < native.bpp) ||
      ((serverDefaultPF.bpp == native.bpp) &&
      (serverDefaultPF.depth < native.depth)))
      fullColourPF = serverDefaultPF;
    else
      fullColourPF = getNativePF();
  }
  formatChange = true;
}


void
CView::updateF8Menu(bool hideSystemCommands) {
  HMENU menu = GetSystemMenu(getHandle(), FALSE);

  if (hideSystemCommands) {  
    // Gray out menu items that might cause a World Of Pain
    HMENU menu = GetSystemMenu(getHandle(), FALSE);
    EnableMenuItem(menu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(menu, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(menu, SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(menu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(menu, SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
  }

  // Update the modifier key menu items
  UINT ctrlCheckFlags = kbd.keyPressed(VK_CONTROL) ? MF_CHECKED : MF_UNCHECKED;
  UINT altCheckFlags = kbd.keyPressed(VK_MENU) ? MF_CHECKED : MF_UNCHECKED;
  CheckMenuItem(menu, IDM_CTRL_KEY, MF_BYCOMMAND | ctrlCheckFlags);
  CheckMenuItem(menu, IDM_ALT_KEY, MF_BYCOMMAND | altCheckFlags);

  // Ensure that the Send <MenuKey> menu item has the correct text
  if (options.menuKey) {
    TCharArray menuKeyStr(options.menuKeyName());
    TCharArray tmp(_tcslen(menuKeyStr.buf) + 6);
    _stprintf(tmp.buf, _T("Send %s"), menuKeyStr.buf);
    if (!ModifyMenu(menu, IDM_SEND_MENU_KEY, MF_BYCOMMAND | MF_STRING, IDM_SEND_MENU_KEY, tmp.buf))
      InsertMenu(menu, IDM_SEND_CAD, MF_BYCOMMAND | MF_STRING, IDM_SEND_MENU_KEY, tmp.buf);
  } else {
    RemoveMenu(menu, IDM_SEND_MENU_KEY, MF_BYCOMMAND);
  }
}


void
CView::setName(const char* name) {
  vlog.debug("setName %s", name);
  ::SetWindowText(getHandle(), TStr(name));
  CConnection::setName(name);
}


void CView::serverInit() {
  CConnection::serverInit();

  // If using AutoSelect with old servers, start in FullColor
  // mode. See comment in autoSelectFormatAndEncoding. 
  if (cp.beforeVersion(3, 8) && options.autoSelect) {
    options.fullColour = true;
  }

  // Save the server's current format
  serverDefaultPF = cp.pf();

  // Calculate the full-colour format to use
  calculateFullColourPF();

  // Request the initial update
  vlog.info("requesting initial update");
  formatChange = encodingChange = requestUpdate = true;
  requestNewUpdate();

  // Show the window
  setVisible(true);
}

void
CView::serverCutText(const char* str, int len) {
  if (!options.serverCutText) return;
  CharArray t(len+1);
  memcpy(t.buf, str, len);
  t.buf[len] = 0;
  clipboard.setClipText(t.buf);
}


void CView::beginRect(const Rect& r, unsigned int encoding) {
  sock->inStream().startTiming();
}

void CView::endRect(const Rect& r, unsigned int encoding) {
  sock->inStream().stopTiming();
  lastUsedEncoding_ = encoding;
  if (debugDelay != 0) {
    invertRect(r);
    debugRects.push_back(r);
  }
}

void CView::fillRect(const Rect& r, Pixel pix) {
  if (cursorBackingRect.overlaps(r)) hideLocalCursor();
  buffer->fillRect(r, pix);
  invalidateBufferRect(r);
}
void CView::imageRect(const Rect& r, void* pixels) {
  if (cursorBackingRect.overlaps(r)) hideLocalCursor();
  buffer->imageRect(r, pixels);
  invalidateBufferRect(r);
}
void CView::copyRect(const Rect& r, int srcX, int srcY) {
  if (cursorBackingRect.overlaps(r) ||
      cursorBackingRect.overlaps(Rect(srcX, srcY, srcX+r.width(), srcY+r.height())))
    hideLocalCursor();
  buffer->copyRect(r, Point(r.tl.x-srcX, r.tl.y-srcY));
  invalidateBufferRect(r);
}

void CView::invertRect(const Rect& r) {
  int stride;
  rdr::U8* p = buffer->getPixelsRW(r, &stride);
  for (int y = 0; y < r.height(); y++) {
    for (int x = 0; x < r.width(); x++) {
      switch (buffer->getPF().bpp) {
      case 8:  ((rdr::U8* )p)[x+y*stride] ^= 0xff;       break;
      case 16: ((rdr::U16*)p)[x+y*stride] ^= 0xffff;     break;
      case 32: ((rdr::U32*)p)[x+y*stride] ^= 0xffffffff; break;
      }
    }
  }
  invalidateBufferRect(r);
}

bool CView::getUserPasswd(char** user, char** password) {
  if (!user && options.passwordFile.buf[0]) {
    FILE* fp = fopen(options.passwordFile.buf, "rb");
    if (!fp) return false;
    char data[256];
    int datalen = fread(data, 1, 256, fp);
    fclose(fp);
    if (datalen != 8) return false;
    vncAuthUnobfuscatePasswd(data);
    *password = strDup(data);
    memset(data, 0, strlen(data));
    return true;
  }

  if (user && options.userName.buf)
    *user = strDup(options.userName.buf);
  if (password && options.password.buf)
    *password = strDup(options.password.buf);
  if ((user && !*user) || (password && !*password)) {
    // Missing username or password - prompt the user
    UserPasswdDialog userPasswdDialog;
    userPasswdDialog.setCSecurity(getCurrentCSecurity());
    if (!userPasswdDialog.getUserPasswd(user, password))
      return false;
  }
  if (user) options.setUserName(*user);
  if (password) options.setPassword(*password);
  return true;
}

bool CView::processFTMsg(int type)
{
  return m_fileTransfer.processFTMsg(type);
}