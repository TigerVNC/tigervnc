/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- RFB Player for Win32

#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/Threading.h>

#include <rfb_win32/Win32Util.h>
#include <rfb_win32/WMShatter.h> 

#include <rfbplayer/rfbplayer.h>
#include <rfbplayer/utils.h>
#include <rfbplayer/resource.h>

using namespace rfb;
using namespace rfb::win32;

// -=- Variables & consts

static LogWriter vlog("RfbPlayer");

TStr rfb::win32::AppName("RfbPlayer");
extern const char* buildTime;

char wrong_cmd_msg[] = 
 "Wrong command-line parameters!\n"
 "Use for help: rfbplayer -help";

char usage_msg[] = 
 "usage: rfbplayer <options> <filename>\n"
 "Command-line options:\n"
 "  -help         \t- Provide usage information.\n"
 "  -speed <value>\t- Sets playback speed, where 1 is normal speed,\n"
 "                \t  is double speed, 0.5 is half speed. Default: 1.0.\n"
 "  -pos <ms>     \t- Sets initial time position in the session file,\n"
 "                \t  in milliseconds. Default: 0.\n"
 "  -autoplay     \t- Runs the player in the playback mode.\n"
 "  -bell         \t- Accepts the bell.\n";

// -=- RfbPlayer's defines

#define strcasecmp _stricmp
#define MAX_SPEED 10
#define MAX_POS_TRACKBAR_RANGE 50

#define ID_TOOLBAR 500
#define ID_PLAY 510
#define ID_PAUSE 520
#define ID_TIME_STATIC 530
#define ID_SPEED_STATIC 540
#define ID_SPEED_EDIT 550
#define ID_POS_TRACKBAR 560
#define ID_SPEED_UPDOWN 570


//
// -=- RfbPlayerClass

//
// Window class used as the basis for RfbPlayer instance
//

class RfbPlayerClass {
public:
  RfbPlayerClass();
  ~RfbPlayerClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK RfbPlayerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;

  if (msg == WM_CREATE)
    SetWindowLong(hwnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY) {
    RfbPlayer* _this = (RfbPlayer*) GetWindowLong(hwnd, GWL_USERDATA);

    // Resume playback (It's need to quit from FbsInputStream::waitWhilePaused())
    _this->setPaused(false);
    SetWindowLong(hwnd, GWL_USERDATA, 0);
  }
  RfbPlayer* _this = (RfbPlayer*) GetWindowLong(hwnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %u", hwnd, msg);
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  try {
    result = _this->processMainMessage(hwnd, msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
};

RfbPlayerClass::RfbPlayerClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = RfbPlayerProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = (HICON)LoadImage(GetModuleHandle(0),
    MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_SHARED);
  if (!wndClass.hIcon)
    printf("unable to load icon:%ld", GetLastError());
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = HBRUSH(COLOR_WINDOW);
  wndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
  wndClass.lpszClassName = _T("RfbPlayerClass");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register RfbPlayer window class",
                               GetLastError());
  }
}

RfbPlayerClass::~RfbPlayerClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

RfbPlayerClass baseClass;

//
// -=- RfbFrameClass

//
// Window class used to displaying the rfb data
//

class RfbFrameClass {
public:
  RfbFrameClass();
  ~RfbFrameClass();
  ATOM classAtom;
  HINSTANCE instance;
};

LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  LRESULT result;

  if (msg == WM_CREATE)
    SetWindowLong(hwnd, GWL_USERDATA, (long)((CREATESTRUCT*)lParam)->lpCreateParams);
  else if (msg == WM_DESTROY)
    SetWindowLong(hwnd, GWL_USERDATA, 0);
  RfbPlayer* _this = (RfbPlayer*) GetWindowLong(hwnd, GWL_USERDATA);
  if (!_this) {
    vlog.info("null _this in %x, message %u", hwnd, msg);
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  try {
    result = _this->processFrameMessage(hwnd, msg, wParam, lParam);
  } catch (rdr::Exception& e) {
    vlog.error("untrapped: %s", e.str());
  }

  return result;
}

RfbFrameClass::RfbFrameClass() : classAtom(0) {
  WNDCLASS wndClass;
  wndClass.style = 0;
  wndClass.lpfnWndProc = FrameProc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = instance = GetModuleHandle(0);
  wndClass.hIcon = 0;
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = 0;
  wndClass.lpszMenuName = 0;
  wndClass.lpszClassName = _T("RfbPlayerClass1");
  classAtom = RegisterClass(&wndClass);
  if (!classAtom) {
    throw rdr::SystemException("unable to register RfbPlayer window class",
                               GetLastError());
  }
}

RfbFrameClass::~RfbFrameClass() {
  if (classAtom) {
    UnregisterClass((const TCHAR*)classAtom, instance);
  }
}

RfbFrameClass frameClass;

//
// -=- RfbPlayer instance implementation
//

RfbPlayer::RfbPlayer(char *_fileName, long _initTime = 0, double _playbackSpeed = 1.0,
                     bool _autoplay = false, bool _acceptBell = false)
: RfbProto(_fileName), initTime(_initTime), playbackSpeed(_playbackSpeed),
  autoplay(_autoplay), buffer(0), client_size(0, 0, 32, 32), 
  window_size(0, 0, 32, 32), cutText(0), seekMode(false), fileName(_fileName), 
  serverInitTime(0), lastPos(0), timeStatic(0), speedEdit(0), posTrackBar(0),
  speedUpDown(0), acceptBell(_acceptBell), rfbReader(0), sessionTimeMs(0),
  sliderDraging(false), sliderStepMs(0), loopPlayback(false) {

  CTRL_BAR_HEIGHT = 28;

  // Reset the full session time
  strcpy(fullSessionTime, "00m:00s");

  // Create the main window
  const TCHAR* name = _T("RfbPlayer");
  mainHwnd = CreateWindow((const TCHAR*)baseClass.classAtom, name, WS_OVERLAPPEDWINDOW,
    0, 0, 640, 480, 0, 0, baseClass.instance, this);
  if (!mainHwnd) {
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", (const char*)CStr(name), getMainHandle());

  // Create the backing buffer
  buffer = new win32::DIBSectionBuffer(getFrameHandle());
  setVisible(true);
    
  // Open the session file
  if (fileName) {
    openSessionFile(fileName);
    if (initTime > 0) setPos(initTime);
    setSpeed(playbackSpeed);
  }
}

RfbPlayer::~RfbPlayer() {
  vlog.debug("~RfbPlayer");
  if (rfbReader) {
    delete rfbReader->join();
    rfbReader = 0;
  }
  if (mainHwnd) {
    setVisible(false);
    DestroyWindow(mainHwnd);
    mainHwnd = 0;
  }
  delete buffer;
  delete cutText;
  vlog.debug("~RfbPlayer done"); 
}

LRESULT 
RfbPlayer::processMainMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

    // -=- Process standard window messages
  
  case WM_CREATE:
    {
      // Create the frame window
      frameHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const TCHAR*)frameClass.classAtom,
        0, WS_CHILD | WS_VISIBLE, 0, CTRL_BAR_HEIGHT, 10, CTRL_BAR_HEIGHT + 10,
        hwnd, 0, frameClass.instance, this);

      createToolBar(hwnd);

      hMenu = GetMenu(hwnd);

      return 0;
    }
  
    // Process the main menu and toolbar's messages

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case ID_OPENFILE:
      {
        char curDir[_MAX_DIR];
        static char filename[_MAX_PATH];
        OPENFILENAME ofn;
        memset((void *) &ofn, 0, sizeof(OPENFILENAME));
        GetCurrentDirectory(sizeof(curDir), curDir);
       
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = getMainHandle();
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename);
        ofn.lpstrInitialDir = curDir;
        ofn.lpstrFilter = "Rfb Session files (*.rfb)\0*.rfb\0" \
                          "All files (*.*)\0*.*\0";
        ofn.lpstrDefExt = "rfb";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn))
          openSessionFile(filename);
      }
      break;
    case ID_PLAY:
      setPaused(false);
      break;
    case ID_PAUSE:
      setPaused(true);
      break;
    case ID_STOP:
      if (getTimeOffset() != 0) {
        stopPlayback();
      }
      break;
    case ID_PLAYPAUSE:
      if (isPaused()) {
        setPaused(false);
      } else {
        setPaused(true);
      }
      break;
    case ID_FULLSCREEN:
      MessageBox(getMainHandle(), "It is not working yet!", "RfbPlayer", MB_OK);
      break;
    case ID_LOOP:
      loopPlayback = !loopPlayback;
      if (loopPlayback) CheckMenuItem(hMenu, ID_LOOP, MF_CHECKED);
      else CheckMenuItem(hMenu, ID_LOOP, MF_UNCHECKED);
      break;
    case ID_RETURN:
        // Update the speed if return pressed in speedEdit
      if (speedEdit == GetFocus()) {
        char speedStr[20], *stopStr;
        GetWindowText(speedEdit, speedStr, sizeof(speedStr));
        double speed = strtod(speedStr, &stopStr);
        if (speed > 0) {
          speed = min(MAX_SPEED, speed);
          // Update speedUpDown position
          SendMessage(speedUpDown, UDM_SETPOS, 
            0, MAKELONG((short)(speed / 0.5), 0));
        } else {
          speed = getSpeed();
        }
        setSpeed(speed);
        sprintf(speedStr, "%.2f", speed);
        SetWindowText(speedEdit, speedStr);
      }
      break;
    case ID_EXIT:
      is->resumePlayback();
      PostQuitMessage(0);
      break;
    }
    break;

    // Update frame's window size and add scrollbars if required

  case WM_SIZE:
    {
    
      Point old_offset = bufferToClient(Point(0, 0));

      // Update the cached sizing information
      RECT r;
      GetClientRect(getMainHandle(), &r);
      MoveWindow(getFrameHandle(), 0, CTRL_BAR_HEIGHT, r.right - r.left,
                 r.bottom - r.top - CTRL_BAR_HEIGHT, TRUE);

      GetWindowRect(getFrameHandle(), &r);
      window_size = Rect(r.left, r.top, r.right, r.bottom);
      GetClientRect(getFrameHandle(), &r);
      client_size = Rect(r.left, r.top, r.right, r.bottom);

      // Determine whether scrollbars are required
      calculateScrollBars();
         
      // Resize the ToolBar
      tb.autoSize();

      // Redraw if required
      if (!old_offset.equals(bufferToClient(Point(0, 0))))
        InvalidateRect(getFrameHandle(), 0, TRUE);
    } 
    break;

    // Process messages from posTrackBar

  case WM_HSCROLL:
    {
      long Pos = SendMessage(posTrackBar, TBM_GETPOS, 0, 0);
      Pos *= sliderStepMs;

      switch (LOWORD(wParam)) { 
      case TB_PAGEUP:
      case TB_PAGEDOWN:
      case TB_LINEUP:
      case TB_LINEDOWN:
      case TB_THUMBTRACK:
        sliderDraging = true;
        updatePos(Pos);
        return 0;
      case TB_ENDTRACK: 
        setPos(Pos);
        sliderDraging = false;
        return 0;
      default:
        break;
      }
    }
    break;
  
  case WM_NOTIFY:
    switch (((NMHDR*)lParam)->code) {
    case UDN_DELTAPOS:
      if ((int)wParam == ID_SPEED_UPDOWN) {
        BOOL lResult = FALSE;
        char speedStr[20] = "\0";
        DWORD speedRange = SendMessage(speedUpDown, UDM_GETRANGE, 0, 0);
        LPNM_UPDOWN upDown = (LPNM_UPDOWN)lParam;
        double speed;

        // The out of range checking
        if (upDown->iDelta > 0) {
          speed = min(upDown->iPos + upDown->iDelta, LOWORD(speedRange)) * 0.5;
        } else {
          // It's need to round the UpDown position
          if ((upDown->iPos * 0.5) != getSpeed()) {
            upDown->iDelta = 0;
            lResult = TRUE;
          }
          speed = max(upDown->iPos + upDown->iDelta, HIWORD(speedRange)) * 0.5;
        }
        _gcvt(speed, 5, speedStr);
        sprintf(speedStr, "%.2f", speed);
        SetWindowText(speedEdit, speedStr);
        setSpeed(speed);
        return lResult;
      }
    }
    return 0;

  case WM_CLOSE:
    vlog.debug("WM_CLOSE %x", getMainHandle());
    PostQuitMessage(0);
    break;
  }

  return rfb::win32::SafeDefWindowProc(getMainHandle(), msg, wParam, lParam);
}

LRESULT RfbPlayer::processFrameMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

  case WM_PAINT:
    {
      if (is->isSeeking()) {
        seekMode = true;
        return 0;
      } else {
        if (seekMode) {
          seekMode = false;
          InvalidateRect(getFrameHandle(), 0, true);
          UpdateWindow(getFrameHandle());
          return 0;
        }
      }

      PAINTSTRUCT ps;
      HDC paintDC = BeginPaint(getFrameHandle(), &ps);
      if (!paintDC)
        throw SystemException("unable to BeginPaint", GetLastError());
      Rect pr = Rect(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);

      if (!pr.is_empty()) {

        if (buffer->bitmap) {

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
    }
    return 0;

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
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RfbPlayer::setOptions(long _initTime = 0, double _playbackSpeed = 1.0,
                           bool _autoplay = false) {
  autoplay = _autoplay;
  playbackSpeed = _playbackSpeed;
  initTime = _initTime;
}

void RfbPlayer::applyOptions() {
  if (initTime >= 0)
    setPos(initTime);
  setSpeed(playbackSpeed);
  setPaused(!autoplay);
}

void RfbPlayer::createToolBar(HWND parentHwnd) {
  RECT tRect;
  InitCommonControls();

  tb.create(ID_TOOLBAR, parentHwnd);
  tb.addBitmap(4, IDB_TOOLBAR);

  // Create the control buttons
  tb.addButton(0, ID_PLAY);
  tb.addButton(1, ID_PAUSE);
  tb.addButton(2, ID_STOP);
  tb.addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  tb.addButton(3, ID_FULLSCREEN);
  tb.addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);

  // Create the static control for the time output
  tb.addButton(125, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  tb.getButtonRect(6, &tRect);
  timeStatic = CreateWindowEx(0, "Static", "00m:00s (00m:00s)", 
    WS_CHILD | WS_VISIBLE, tRect.left, tRect.top+2, tRect.right-tRect.left, 
    tRect.bottom-tRect.top, tb.getHandle(), (HMENU)ID_TIME_STATIC, 
    GetModuleHandle(0), 0);
  tb.addButton(0, 10, TBSTATE_ENABLED, TBSTYLE_SEP);
    
  // Create the trackbar control for the time position
  tb.addButton(200, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  tb.getButtonRect(8, &tRect);
  posTrackBar = CreateWindowEx(0, TRACKBAR_CLASS, "Trackbar Control", 
    WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_ENABLESELRANGE,
    tRect.left, tRect.top, tRect.right-tRect.left, tRect.bottom-tRect.top,
    parentHwnd, (HMENU)ID_POS_TRACKBAR, GetModuleHandle(0), 0);
  // It's need to send notify messages to toolbar parent window
  SetParent(posTrackBar, tb.getHandle());
  tb.addButton(0, 10, TBSTATE_ENABLED, TBSTYLE_SEP);

  // Create the label with "Speed:" caption
  tb.addButton(50, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  tb.getButtonRect(10, &tRect);
  CreateWindowEx(0, "Static", "Speed:", WS_CHILD | WS_VISIBLE, 
    tRect.left, tRect.top+2, tRect.right-tRect.left, tRect.bottom-tRect.top,
    tb.getHandle(), (HMENU)ID_SPEED_STATIC, GetModuleHandle(0), 0);

  // Create the edit control and the spin for the speed managing
  tb.addButton(60, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  tb.getButtonRect(11, &tRect);
  speedEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "1.00", 
    WS_CHILD | WS_VISIBLE | ES_RIGHT, tRect.left, tRect.top, 
    tRect.right-tRect.left, tRect.bottom-tRect.top, parentHwnd,
    (HMENU)ID_SPEED_EDIT, GetModuleHandle(0), 0);
  // It's need to send notify messages to toolbar parent window
  SetParent(speedEdit, tb.getHandle());

  speedUpDown = CreateUpDownControl(WS_CHILD | WS_VISIBLE  
    | WS_BORDER | UDS_ALIGNRIGHT, 0, 0, 0, 0, tb.getHandle(),
    ID_SPEED_UPDOWN, GetModuleHandle(0), speedEdit, 20, 1, 2);
}

void RfbPlayer::setVisible(bool visible) {
  ShowWindow(getMainHandle(), visible ? SW_SHOW : SW_HIDE);
  if (visible) {
    // When the window becomes visible, make it active
    SetForegroundWindow(getMainHandle());
    SetActiveWindow(getMainHandle());
  }
}

void RfbPlayer::setTitle(const char *title) {
  char _title[256];
  strcpy(_title, AppName);
  strcat(_title, " - ");
  strcat(_title, title);
  SetWindowText(getMainHandle(), _title);
}

void RfbPlayer::setFrameSize(int width, int height) {
  // Calculate and set required size for main window
  RECT r = {0, 0, width, height};
  AdjustWindowRectEx(&r, GetWindowLong(getFrameHandle(), GWL_STYLE), FALSE, 
    GetWindowLong(getFrameHandle(), GWL_EXSTYLE));
  r.bottom += CTRL_BAR_HEIGHT; // Include RfbPlayr's controls area
  AdjustWindowRect(&r, GetWindowLong(getMainHandle(), GWL_STYLE), FALSE);
  SetWindowPos(getMainHandle(), 0, 0, 0, r.right-r.left, r.bottom-r.top,
    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

  // Enable/disable scrollbars as appropriate
  calculateScrollBars(); 
}

void RfbPlayer::calculateScrollBars() {
  // Calculate the required size of window
  DWORD current_style = GetWindowLong(getFrameHandle(), GWL_STYLE);
  DWORD style = current_style & ~(WS_VSCROLL | WS_HSCROLL);
  DWORD old_style;
  RECT r;
  SetRect(&r, 0, 0, buffer->width(), buffer->height());
  AdjustWindowRectEx(&r, style, FALSE, GetWindowLong(getFrameHandle(), GWL_EXSTYLE));
  Rect reqd_size = Rect(r.left, r.top, r.right, r.bottom);

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
}

bool RfbPlayer::setViewportOffset(const Point& tl) {
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

void RfbPlayer::close(const char* reason) {
  setVisible(false);
  if (reason) {
    vlog.info("closing - %s", reason);
    MessageBox(NULL, TStr(reason), "RfbPlayer", MB_ICONINFORMATION | MB_OK);
  }
  SendMessage(getFrameHandle(), WM_CLOSE, 0, 0);
}

void RfbPlayer::blankBuffer() {
  fillRect(buffer->getRect(), 0);
}

void RfbPlayer::rewind() {
  bool paused = isPaused();
  blankBuffer();
  newSession(fileName);
  skipHandshaking();
  setSpeed(playbackSpeed);
  if (paused) is->pausePlayback();
  else is->resumePlayback();
}

void RfbPlayer::processMsg() {
  static long update_time = GetTickCount();
  try {
    if ((!isSeeking()) && ((GetTickCount() - update_time) > 250)
      && (!sliderDraging)) {
      // Update pos in the toolbar 4 times in 1 second
      updatePos(getTimeOffset());
      update_time = GetTickCount();
    }
    RfbProto::processMsg();
  } catch (rdr::Exception e) {
    if (strcmp(e.str(), "[End Of File]") == 0) {
      rewind();
      setPaused(!loopPlayback);
      updatePos(getTimeOffset());
      SendMessage(posTrackBar, TBM_SETPOS, TRUE, 0);
      return;
    }
    // It's a special exception to perform backward seeking.
    // We only rewind the stream and seek the offset
    if (strcmp(e.str(), "[REWIND]") == 0) {
      long initTime = getSeekOffset();
      rewind();
      setPos(initTime);
      updatePos(getTimeOffset());
    } else {
      MessageBox(getMainHandle(), e.str(), e.type(), MB_OK | MB_ICONERROR);
      return;
    }
  }
}

void RfbPlayer::serverInit() {
  RfbProto::serverInit();

  // Save the server init time for using in setPos()
  serverInitTime = getTimeOffset() / getSpeed();
  
  // Resize the backing buffer
  buffer->setSize(cp.width, cp.height);

  // Check on the true colour mode
  if (!(cp.pf()).trueColour)
    throw rdr::Exception("This version plays only true color session!");

  // Set the session pixel format
  buffer->setPF(cp.pf()); 

  // If the window is not maximised then resize it
  if (!(GetWindowLong(getMainHandle(), GWL_STYLE) & WS_MAXIMIZE))
    setFrameSize(cp.width, cp.height);

  // Set the window title and show it
  setTitle(cp.name());

  // Calculate the full session time and update posTrackBar control
  sessionTimeMs = calculateSessionTime(fileName);
  sprintf(fullSessionTime, "%.2um:%.2us", 
    sessionTimeMs / 1000 / 60, sessionTimeMs / 1000 % 60);
  SendMessage(posTrackBar, TBM_SETRANGE, 
    TRUE, MAKELONG(0, min(sessionTimeMs / 1000, MAX_POS_TRACKBAR_RANGE)));
  sliderStepMs = sessionTimeMs / SendMessage(posTrackBar, TBM_GETRANGEMAX, 0, 0);
  updatePos(getTimeOffset());

  setPaused(!autoplay);
}

void RfbPlayer::setColourMapEntries(int first, int count, U16* rgbs) {
  vlog.debug("setColourMapEntries: first=%d, count=%d", first, count);
  throw rdr::Exception("Can't handle SetColourMapEntries message", "RfbPlayer");
/*  int i;
  for (i=0;i<count;i++) {
    buffer->setColour(i+first, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);
  }
  // *** change to 0, 256?
  refreshWindowPalette(first, count);
  palette_changed = true;
  InvalidateRect(getFrameHandle(), 0, FALSE);*/
} 

void RfbPlayer::bell() {
  if (acceptBell)
    MessageBeep(-1);
}

void RfbPlayer::serverCutText(const char* str, int len) {
  if (cutText != NULL)
    delete [] cutText;
  cutText = new char[len + 1];
  memcpy(cutText, str, len);
  cutText[len] = '\0';
}

void RfbPlayer::frameBufferUpdateEnd() {
};

void RfbPlayer::beginRect(const Rect& r, unsigned int encoding) {
}

void RfbPlayer::endRect(const Rect& r, unsigned int encoding) {
}


void RfbPlayer::fillRect(const Rect& r, Pixel pix) {
  buffer->fillRect(r, pix);
  invalidateBufferRect(r);
}

void RfbPlayer::imageRect(const Rect& r, void* pixels) {
  buffer->imageRect(r, pixels);
  invalidateBufferRect(r);
}

void RfbPlayer::copyRect(const Rect& r, int srcX, int srcY) {
  buffer->copyRect(r, Point(r.tl.x-srcX, r.tl.y-srcY));
  invalidateBufferRect(r);
} 

bool RfbPlayer::invalidateBufferRect(const Rect& crect) {
  Rect rect = bufferToClient(crect);
  if (rect.intersect(client_size).is_empty()) return false;
  RECT invalid = {rect.tl.x, rect.tl.y, rect.br.x, rect.br.y};
  InvalidateRect(getFrameHandle(), &invalid, FALSE);
  return true;
}

long RfbPlayer::calculateSessionTime(char *filename) {
  FbsInputStream sessionFile(filename);
  sessionFile.setTimeOffset(100000000);
  try {
    while (TRUE) {
      sessionFile.skip(1024);
    }
  } catch (rdr::Exception e) {
    if (strcmp(e.str(), "[End Of File]") == 0) {
      return sessionFile.getTimeOffset();
    } else {
      MessageBox(getMainHandle(), e.str(), e.type(), MB_OK | MB_ICONERROR);
      return 0;
    }
  }
  return 0;
}

void RfbPlayer::openSessionFile(char *_fileName) {
  fileName = strDup(_fileName);

  // Close the previous reading thread
  if (rfbReader) {
    is->resumePlayback();
    delete rfbReader->join();
  }
  blankBuffer();
  newSession(fileName);
  setSpeed(playbackSpeed);
  rfbReader = new rfbSessionReader(this);
  rfbReader->start();
  SendMessage(posTrackBar, TBM_SETPOS, TRUE, 0);
}

void RfbPlayer::setPaused(bool paused) {
  if (paused) {
    is->pausePlayback();
    tb.checkButton(ID_PAUSE, true);
    tb.checkButton(ID_PLAY, false);
    tb.checkButton(ID_STOP, false);
    CheckMenuItem(hMenu, ID_PLAYPAUSE, MF_CHECKED);
    CheckMenuItem(hMenu, ID_STOP, MF_UNCHECKED);
  } else {
    is->resumePlayback();
    tb.checkButton(ID_PLAY, true);
    tb.checkButton(ID_STOP, false);
    tb.checkButton(ID_PAUSE, false);
    CheckMenuItem(hMenu, ID_PLAYPAUSE, MF_CHECKED);
    CheckMenuItem(hMenu, ID_STOP, MF_UNCHECKED);
  }
}

void RfbPlayer::stopPlayback() {
  setPos(0);
  is->pausePlayback();
  tb.checkButton(ID_STOP, true);
  tb.checkButton(ID_PLAY, false);
  tb.checkButton(ID_PAUSE, false);
  CheckMenuItem(hMenu, ID_STOP, MF_CHECKED);
  CheckMenuItem(hMenu, ID_PLAYPAUSE, MF_UNCHECKED);
  SendMessage(posTrackBar, TBM_SETPOS, TRUE, 0);
}

void RfbPlayer::setSpeed(double speed) {
  serverInitTime = serverInitTime * getSpeed() / speed;
  is->setSpeed(speed);
  playbackSpeed = speed;
}

double RfbPlayer::getSpeed() {
  return is->getSpeed();
}

void RfbPlayer::setPos(long pos) {
  is->setTimeOffset(max(pos, serverInitTime));
}

long RfbPlayer::getSeekOffset() {
  return is->getSeekOffset();
}

bool RfbPlayer::isSeeking() {
  return is->isSeeking();
}

bool RfbPlayer::isSeekMode() {
  return seekMode;
}

bool RfbPlayer::isPaused() {
  return is->isPaused();
}

long RfbPlayer::getTimeOffset() {
  return max(is->getTimeOffset(), is->getSeekOffset());
}

void RfbPlayer::updatePos(long newPos) {
  // Update time pos in static control
  char timePos[30] = "\0";
  long sliderPos = newPos;
  newPos /= 1000;
  sprintf(timePos, "%.2um:%.2us (%s)", newPos/60, newPos%60, fullSessionTime);
  SetWindowText(timeStatic, timePos);

  // Update the position of slider
  if (!sliderDraging) {
    sliderPos /= sliderStepMs;
    if (sliderPos > SendMessage(posTrackBar, TBM_GETPOS, 0, 0))
      SendMessage(posTrackBar, TBM_SETPOS, TRUE, sliderPos);
  }
}

void RfbPlayer::skipHandshaking() {
  int skipBytes = 12 + 4 + 24 + strlen(cp.name());
  is->skip(skipBytes);
  state_ = RFBSTATE_NORMAL;
}

void programInfo() {
  win32::FileVersionInfo inf;
  _tprintf(_T("%s - %s, Version %s\n"),
    inf.getVerString(_T("ProductName")),
    inf.getVerString(_T("FileDescription")),
    inf.getVerString(_T("FileVersion")));
  printf("%s\n", buildTime);
  _tprintf(_T("%s\n\n"), inf.getVerString(_T("LegalCopyright")));
}

void programUsage() {
  MessageBox(0, usage_msg, "RfbPlayer", MB_OK | MB_ICONINFORMATION);
}

double playbackSpeed = 1.0;
long initTime = -1;
bool autoplay = false;
char *fileName;
bool print_usage = false;
bool acceptBell = false;

bool processParams(int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    if ((strcasecmp(argv[i], "-help") == 0) ||
        (strcasecmp(argv[i], "--help") == 0) ||
        (strcasecmp(argv[i], "/help") == 0) ||
        (strcasecmp(argv[i], "-h") == 0) ||
        (strcasecmp(argv[i], "/h") == 0) ||
        (strcasecmp(argv[i], "/?") == 0) ||
        (strcasecmp(argv[i], "-?") == 0)) {
      print_usage = true;
      return true;
    }

    if ((strcasecmp(argv[i], "-speed") == 0) ||
        (strcasecmp(argv[i], "/speed") == 0) && (i < argc-1)) {
      playbackSpeed = atof(argv[++i]);
      if (playbackSpeed <= 0) {
        return false;
      }
      continue;
    }

    if ((strcasecmp(argv[i], "-pos") == 0) ||
        (strcasecmp(argv[i], "/pos") == 0) && (i < argc-1)) {
      initTime = atol(argv[++i]);
      if (initTime <= 0)
        return false;
      continue;
    }

    if ((strcasecmp(argv[i], "-autoplay") == 0) ||
        (strcasecmp(argv[i], "/autoplay") == 0) && (i < argc-1)) {
      autoplay = true;
      continue;
    }

    if ((strcasecmp(argv[i], "-bell") == 0) ||
        (strcasecmp(argv[i], "/bell") == 0) && (i < argc-1)) {
      acceptBell  = true;
      continue;
    }

    if (i != argc - 1)
      return false;
  }

  fileName = strDup(argv[argc-1]);
  return true;
}

//
// -=- WinMain
//

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prevInst, char* cmdLine, int cmdShow) {
  
  // - Process the command-line

  int argc = __argc;
  char** argv = __argv;
  if ((argc > 1) && (!processParams(argc, argv))) {
    MessageBox(0, wrong_cmd_msg, "RfbPlayer", MB_OK | MB_ICONWARNING);
    return 0;
  }
  
  if (print_usage) {
    programUsage();
    return 0;
  }

  // Create the player
  RfbPlayer *player = NULL;
  try {
    player = new RfbPlayer(fileName, initTime, playbackSpeed, autoplay, 
                           acceptBell);
  } catch (rdr::Exception e) {
    MessageBox(NULL, e.str(), e.type(), MB_OK | MB_ICONERROR);
    delete player;
    return 0;
  }

  // Run the player
  HACCEL hAccel = LoadAccelerators(inst, MAKEINTRESOURCE(IDR_ACCELERATOR));
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    if(!TranslateAccelerator(player->getMainHandle(), hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // Destroy the player
  try{
    if (player) delete player;
  } catch (rdr::Exception e) {
    MessageBox(NULL, e.str(), e.type(), MB_OK | MB_ICONERROR);
  }

  return 0;
};
