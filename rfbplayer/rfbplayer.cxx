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

#include <conio.h>

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

// -=- RfbPlayer's defines

#define strcasecmp _stricmp

#define ID_START_BTN 0
#define ID_POS_EDT 1
#define ID_SPEED_EDT 2
#define ID_POS_TXT 3
#define ID_SPEED_TXT 4
#define ID_CLIENT_STC 5

// -=- Custom thread class used to reading the rfb data

class CRfbThread : public Thread {
  public:
    CRfbThread(RfbPlayer *_player) {
      p = _player;
      setDeleteAfterRun();
    };
    ~CRfbThread() {};

    void run() {
      long initTime = -1;

      // Process the rfb messages
      while (p->run) {
        try {
          if (initTime >= 0) {
            p->setPos(initTime);
            initTime = -1;
          }
          if (!p->isSeeking())
            p->updatePos();
          p->processMsg();
        } catch (rdr::Exception e) {
          if (strcmp(e.str(), "[End Of File]") == 0) {
            p->rewind();
            p->setPaused(true);
            continue;
          }
          // It's a special exception to perform backward seeking.
          // We only rewind the stream and seek the offset
          if (strcmp(e.str(), "[REWIND]") == 0) {
            initTime = p->getSeekOffset();
            double speed = p->getSpeed();
            bool play = !p->isPaused();
            p->rewind();
            p->setSpeed(speed);
            p->setPaused(!play);
          } else {
            MessageBox(p->getMainHandle(), e.str(), e.type(), MB_OK | MB_ICONERROR);
            return;
          }
        }
      }
    }

  private:
    RfbPlayer *p;
};

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
    _this->run = false;

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
    MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_SHARED);
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
                     bool _autoplay = false, bool _showControls = true, 
                     bool _acceptBell = false)
: RfbProto(_fileName), initTime(_initTime), playbackSpeed(_playbackSpeed),
  autoplay(_autoplay), showControls(_showControls), buffer(0), client_size(0, 0, 32, 32), 
  window_size(0, 0, 32, 32), cutText(0), seekMode(false), fileName(_fileName), run(true), 
  serverInitTime(0), btnStart(0), txtPos(0), editPos(0), txtSpeed(0), editSpeed(0), 
  lastPos(0), acceptBell(_acceptBell) {

  if (showControls)
    CTRL_BAR_HEIGHT = 30;
  else
    CTRL_BAR_HEIGHT = 0;

  // Create the main window
  const TCHAR* name = _T("RfbPlayer");
  mainHwnd = CreateWindow((const TCHAR*)baseClass.classAtom, name, WS_OVERLAPPEDWINDOW,
    0, 0, 10, 10, 0, 0, baseClass.instance, this);
  if (!mainHwnd) {
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", (const char*)CStr(name), getMainHandle());

  // Create the backing buffer
  buffer = new win32::DIBSectionBuffer(getFrameHandle());
}

RfbPlayer::~RfbPlayer() {
  vlog.debug("~RfbPlayer");
  if (mainHwnd) {
    setVisible(false);
    DestroyWindow(mainHwnd);
    mainHwnd = 0;
  }
  delete buffer;
  delete cutText;
  vlog.debug("~RfbPlayer done"); 
}

// RfbPlayer control's tabstop processing

WNDPROC OldProc[3];
static HWND focusHwnd = 0;

LRESULT CALLBACK TabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  int CONTROL_ID = GetWindowLong(hwnd, GWL_ID);
  
  if (msg == WM_DESTROY)
    SetWindowLong(hwnd, GWL_USERDATA, 0);

  RfbPlayer* _this = (RfbPlayer*) GetWindowLong(hwnd, GWL_USERDATA);
  if (!_this)
    return CallWindowProc(OldProc[CONTROL_ID], hwnd, msg, wParam, lParam);

  switch (msg) {
  case WM_KEYDOWN:
    
    // Process tab pressing
    if (wParam == VK_TAB) {
      switch (CONTROL_ID){
      case ID_START_BTN:
        focusHwnd = _this->getPosEdit();
        break;
      case ID_POS_EDT:
        focusHwnd = _this->getSpeedEdit();
        break;
      case ID_SPEED_EDT:
        focusHwnd = _this->getStartBtn();
        break;
      }
      SetFocus(focusHwnd);
    }
    
    // Process return/enter pressing (set the speed and the position)
    if (wParam == VK_RETURN) {
      switch (CONTROL_ID){

          // Change the position

      case ID_POS_EDT:
        {
          char posTxt[20];
          long pos;
          GetWindowText(_this->getPosEdit(), posTxt, 10);
          pos = atol(posTxt);
          if (pos != long(_this->getTimeOffset() / 1000) && (pos >= 0))
            _this->setPos(pos * 1000);
          else
            SetWindowText(_this->getPosEdit(), LongToStr(_this->getTimeOffset() / 1000));
        }
        break;

          // Change the playback speed

      case ID_SPEED_EDT:
        {
          char speedTxt[20];
          double speed;
          GetWindowText(_this->getSpeedEdit(), speedTxt, 10);
          speed = atof(speedTxt);
          if ((speed != _this->getSpeed()) && (speed != 0))
            _this->setSpeed(speed);
          else
            SetWindowText(_this->getSpeedEdit(), DoubleToStr(_this->getSpeed()));
        }
        break;
      }
    }
    break;

  case WM_SETFOCUS:
    focusHwnd = hwnd;
    break;
  }

  return CallWindowProc(OldProc[CONTROL_ID], hwnd, msg, wParam, lParam);
}

LRESULT 
RfbPlayer::processMainMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

    // -=- Process standard window messages

  case WM_SETFOCUS:
    {
      if (focusHwnd)
        SetFocus(focusHwnd);
      else 
        SetFocus(btnStart);
    }
    break;
  
  case WM_CREATE:
    {
      // Create the player controls
      if (showControls) {
        btnStart = CreateWindow("BUTTON", "Stop", WS_CHILD | WS_VISIBLE |
          BS_PUSHBUTTON | BS_NOTIFY, 5, 5, 60, 20, hwnd, (HMENU)ID_START_BTN,
          baseClass.instance, NULL);
        txtPos = CreateWindow("STATIC", "Position:", WS_CHILD | WS_VISIBLE,
          70, 5, 60, 20, hwnd, (HMENU)ID_POS_TXT, baseClass.instance, NULL);
        editPos = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD |
          WS_VISIBLE | ES_NUMBER |ES_RIGHT, 135, 5, 60, 20, hwnd,
          (HMENU)ID_POS_EDT, baseClass.instance, this);
        txtSpeed = CreateWindow("STATIC", "Speed:", WS_CHILD | WS_VISIBLE,
          200, 5, 50, 20, hwnd, (HMENU)ID_SPEED_TXT, baseClass.instance, NULL);
        editSpeed = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1.0", WS_CHILD |
          WS_VISIBLE | ES_RIGHT, 255, 5, 60, 20, hwnd, (HMENU)ID_SPEED_EDT, 
          baseClass.instance, this);

        // Windows subclassing (It's used to implement "TabStop")
        OldProc[0] = (WNDPROC)SetWindowLong(btnStart, GWL_WNDPROC, (LONG)TabProc);
        OldProc[1] = (WNDPROC)SetWindowLong(editPos, GWL_WNDPROC, (LONG)TabProc);
        OldProc[2] = (WNDPROC)SetWindowLong(editSpeed, GWL_WNDPROC, (LONG)TabProc);

        SetWindowLong(btnStart, GWL_USERDATA, (long)this);
        SetWindowLong(editPos, GWL_USERDATA, (long)this);
        SetWindowLong(editSpeed, GWL_USERDATA, (long)this);
      }

      // Create the frame window
      frameHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const TCHAR*)frameClass.classAtom,
        0, WS_CHILD | WS_VISIBLE, 0, CTRL_BAR_HEIGHT, 10, CTRL_BAR_HEIGHT + 10,
        hwnd, 0, frameClass.instance, this);

      return 0;
    }
  
    // Process the start button messages

  case WM_COMMAND:
    {
      if ((LOWORD(wParam) == ID_START_BTN) && (HIWORD(wParam) == BN_CLICKED)) {
        if (is->isPaused()) {
          long pos;
          double speed;
          char str[20];

          // Change the playback speed
          GetWindowText(editSpeed, str, 10);
          speed = atof(str);
          if ((speed != getSpeed()) && (speed != 0))
            setSpeed(speed);

          // Change the position
          GetWindowText(editPos, str, 10);
          pos = atol(str);
          if (pos != long(getTimeOffset() / 1000))
            setPos(pos * 1000);
          setPaused(false);
        } else {
          setPaused(true);
          updatePos();
        }
      }
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

      // Redraw if required
      if (!old_offset.equals(bufferToClient(Point(0, 0))))
        InvalidateRect(getFrameHandle(), 0, TRUE);
    } 
    break;

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
                           bool _autoplay = false, bool _showControls = true) {
  showControls = _showControls;
  autoplay = _autoplay;
  playbackSpeed = _playbackSpeed;
  initTime = _initTime;
}

void RfbPlayer::applyOptions() {
  if (initTime >= 0)
    setPos(initTime);
  setSpeed(playbackSpeed);
  setPaused(!autoplay);

  // Update the position
  SetWindowText(editPos, LongToStr(initTime / 1000));
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
  blankBuffer();
  newSession(fileName);
  skipHandshaking();
}

void RfbPlayer::serverInit() {
  RfbProto::serverInit();

  // Save the server init time for using in setPos()
  serverInitTime = getTimeOffset() / getSpeed();
  
  // Resize the backing buffer
  buffer->setSize(cp.width, cp.height);

  // Check on the true colour mode
  if (!(cp.pf()).trueColour)
    throw rdr::Exception("This version plays only true colour session!");

  // Set the session pixel format
  buffer->setPF(cp.pf()); 

  // If the window is not maximised then resize it
  if (!(GetWindowLong(getMainHandle(), GWL_STYLE) & WS_MAXIMIZE))
    setFrameSize(cp.width, cp.height);

  // Set the window title and show it
  setTitle(cp.name());
  setVisible(true);

  // Set the player's param
  applyOptions();
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

void RfbPlayer::setPaused(bool paused) {
  if (paused) {
    if (btnStart) SetWindowText(btnStart, "Start");
    is->pausePlayback();
  } else {
    if (btnStart) SetWindowText(btnStart, "Stop");
    is->resumePlayback();
  }
}

void RfbPlayer::setSpeed(double speed) {
  serverInitTime = serverInitTime * getSpeed() / speed;
  is->setSpeed(speed);
  if (editSpeed)
    SetWindowText(editSpeed, DoubleToStr(speed, 1));
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
  return is->getTimeOffset();
}

void RfbPlayer::updatePos() {
  long newPos = is->getTimeOffset() / 1000;
  if (editPos && lastPos != newPos) {
    lastPos = newPos;
    SetWindowText(editPos, LongToStr(lastPos));
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
  printf("usage: rfbplayer <options> <filename>\n");
  printf("Command-line options:\n");
  printf("  -help               - Provide usage information.\n");
  printf("  -speed <value>      - Sets playback speed, where 1 is normal speed,\n");
  printf("                        2 is double speed, 0.5 is half speed. Default: 1.0.\n");
  printf("  -pos <ms>           - Sets initial time position in the session file,\n"); 
  printf("                        in milliseconds. Default: 0.\n");
  printf("  -autoplay <yes|no>  - Runs the player in the playback mode. Default: \"no\".\n");
  printf("  -controls <yes|no>  - Shows the control panel at the top. Default: \"yes\".\n");
  printf("  -bell <yes|no>      - Accepts the bell. Default: \"no\".\n");
}

double playbackSpeed = 1.0;
long initTime = -1;
bool autoplay = false;
bool showControls = true;
char *fileName;
bool console = false;
bool wrong_param = false;
bool print_usage = false;
bool acceptBell = false;

bool processParams(int argc, char* argv[]) {
  for (int i = 1; i < argc; i++) {
    if ((strcasecmp(argv[i], "-help") == 0) ||
        (strcasecmp(argv[i], "--help") == 0) ||
        (strcasecmp(argv[i], "/help") == 0) ||
        (strcasecmp(argv[i], "-h") == 0) ||
        (strcasecmp(argv[i], "/h") == 0) ||
        (strcasecmp(argv[i], "/?") == 0)) {
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
      i++;
      if (strcasecmp(argv[i], "yes") == 0) {
        autoplay = true;
        continue;
      }
      if (strcasecmp(argv[i], "no") == 0) {
        autoplay = false;
        continue;
      }
      return false;
    }

    if ((strcasecmp(argv[i], "-controls") == 0) ||
        (strcasecmp(argv[i], "/controls") == 0) && (i < argc-1)) {
      i++;
      if (strcasecmp(argv[i], "yes") == 0) {
        showControls  = true;
        continue;
      }
      if (strcasecmp(argv[i], "no") == 0) {
        showControls = false;
        continue;
      }
      return false;
    }

    if ((strcasecmp(argv[i], "-bell") == 0) ||
        (strcasecmp(argv[i], "/bell") == 0) && (i < argc-1)) {
      i++;
      if (strcasecmp(argv[i], "yes") == 0) {
        acceptBell  = true;
        continue;
      }
      if (strcasecmp(argv[i], "no") == 0) {
        acceptBell = false;
        continue;
      }
      return false;
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
  if (argc > 1) {
    wrong_param = !processParams(argc, argv);
    console = print_usage | wrong_param;
  } else {
    console = true;
  }

  if (console) {
    AllocConsole();
    freopen("CONOUT$","wb",stdout);

    programInfo();
    if (wrong_param)
      printf("Wrong a command line.\n");
    else
      programUsage();

    printf("\nPress Enter/Return key to continue\n");
    char c = getch();
    FreeConsole();

    return 0;
  } 

  // Create the player and the thread which reading the rfb data
  RfbPlayer *player = NULL;
  CRfbThread *rfbThread = NULL;
  try {
    player = new RfbPlayer(fileName, initTime, playbackSpeed, autoplay, 
                           showControls, acceptBell);
    rfbThread = new CRfbThread(player);
    rfbThread->start();
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

  // Wait while the thread destroying and then destroy the player
  try{
    while (rfbThread->getState() == ThreadStarted) {}
    if (player) delete player;
  } catch (rdr::Exception e) {
    MessageBox(NULL, e.str(), e.type(), MB_OK | MB_ICONERROR);
  }

  return 0;
};
