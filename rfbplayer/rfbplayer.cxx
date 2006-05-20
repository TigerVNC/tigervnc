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
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <rfb/LogWriter.h>

#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/DeviceContext.h>
#include <rfb_win32/MsgBox.h>
#include <rfb_win32/WMShatter.h>
#include <rfb_win32/Win32Util.h>

#include <rfbplayer/rfbplayer.h>
#include <rfbplayer/ChoosePixelFormatDialog.h>
#include <rfbplayer/GotoPosDialog.h>
#include <rfbplayer/InfoDialog.h>
#include <rfbplayer/SessionInfoDialog.h>

using namespace rfb;
using namespace rfb::win32;

// -=- Variables & consts

static LogWriter vlog("RfbPlayer");

TStr rfb::win32::AppName("RfbPlayer");
extern const char* buildTime;
HFONT hFont = 0;

char wrong_cmd_msg[] = 
 "Wrong command-line parameters!\n"
 "Use for help: rfbplayer -help";

char usage_msg[] = 
 "usage: rfbplayer <options> <filename>\r\n"
 "Command-line options:\r\n"
 "  -help         \t- Provide usage information.\r\n"
 "  -pf <mode>    \t- Forces the pixel format for the session.\r\n"
 "                \t  <mode>=r<r_bits>g<g_bits>b<b_bits>[le|be],\r\n"
 "                \t  r_bits - size the red component, in bits,\r\n"
 "                \t  g_bits - size the green component, in bits,\r\n"
 "                \t  b_bits - size the blue component, in bits,\r\n"
 "                \t  le - little endian byte order (default),\r\n"
 "                \t  be - big endian byte order.\r\n"
 "                \t  The r, g, b component is in any order.\r\n"
 "                \t  Default: auto detect the pixel format.\r\n"
 "  -upf <name>   \t- Forces the user defined pixel format for\r\n"
 "                \t  the session. If <name> is empty then application\r\n"
 "                \t  shows the list of user defined pixel formats.\r\n"
 "                \t  Don't use this option with -pf.\r\n"
 "  -speed <value>\t- Sets playback speed, where 1 is normal speed,\r\n"
 "                \t  is double speed, 0.5 is half speed. Default: 1.0.\r\n"
 "  -pos <ms>     \t- Sets initial time position in the session file,\r\n"
 "                \t  in milliseconds. Default: 0.\r\n"
 "  -autoplay     \t- Runs the player in the playback mode.\r\n"
 "  -loop         \t- Replays the rfb session.";

// -=- RfbPlayer's defines and consts

#define strcasecmp _stricmp
#define DEFAULT_PLAYER_WIDTH 640
#define DEFAULT_PLAYER_HEIGHT 480 

//
// -=- AboutDialog global values
//

const WORD rfb::win32::AboutDialog::DialogId = IDD_ABOUT;
const WORD rfb::win32::AboutDialog::Copyright = IDC_COPYRIGHT;
const WORD rfb::win32::AboutDialog::Version = IDC_VERSION;
const WORD rfb::win32::AboutDialog::BuildTime = IDC_BUILDTIME;
const WORD rfb::win32::AboutDialog::Description = IDC_DESCRIPTION;

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

RfbPlayer::RfbPlayer(char *_fileName, PlayerOptions *_options)
: RfbProto(_fileName), fileName(_fileName), buffer(0), client_size(0, 0, 32, 32),
  window_size(0, 0, 32, 32), cutText(0), seekMode(false), lastPos(0), 
  rfbReader(0), sessionTimeMs(0), sliderStepMs(0), imageDataStartTime(0), 
  rewindFlag(false), stopped(false), currentEncoding(-1) {

  // Save the player options
  memcpy(&options, _options, sizeof(options));

  // Reset the full session time
  strcpy(fullSessionTime, "00m:00s");

  // Load the user defined pixel formats from the registry
  supportedPF.readUserDefinedPF(HKEY_CURRENT_USER, UPF_REGISTRY_PATH);

  // Create the main window
  const TCHAR* name = _T("RfbPlayer");
  int x = max(0, (GetSystemMetrics(SM_CXSCREEN) - DEFAULT_PLAYER_WIDTH) / 2);
  int y = max(0, (GetSystemMetrics(SM_CYSCREEN) - DEFAULT_PLAYER_HEIGHT) / 2);
  mainHwnd = CreateWindow((const TCHAR*)baseClass.classAtom, name, WS_OVERLAPPEDWINDOW,
    x, y, DEFAULT_PLAYER_WIDTH, DEFAULT_PLAYER_HEIGHT, 0, 0, baseClass.instance, this);
  if (!mainHwnd) {
    throw rdr::SystemException("unable to create WMNotifier window instance", GetLastError());
  }
  vlog.debug("created window \"%s\" (%x)", (const char*)CStr(name), getMainHandle());

  // Create the backing buffer
  buffer = new win32::DIBSectionBuffer(getFrameHandle());
  setVisible(true);

  // If run with command-line parameters,
  // open the session file with default settings, otherwise
  // restore player settings from the registry
  if (fileName) {
    openSessionFile(fileName);
    if (options.initTime > 0) setPos(options.initTime);
    setSpeed(options.playbackSpeed);
  } else {
    options.readFromRegistry();
    disableTBandMenuItems();
    setTitle("None");
  }
  init();
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
  if (buffer) delete buffer;
  if (cutText) delete [] cutText;
  if (fileName) delete [] fileName;
  if (hFont) DeleteObject(hFont);
  vlog.debug("~RfbPlayer done"); 
}

LRESULT 
RfbPlayer::processMainMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

  switch (msg) {

    // -=- Process standard window messages
  
  case WM_CREATE:
    {
      tb.create(this, hwnd);

      // Create the frame window
      frameHwnd = CreateWindowEx(WS_EX_CLIENTEDGE, (const TCHAR*)frameClass.classAtom,
        0, WS_CHILD | WS_VISIBLE, 0, tb.getHeight(), 10, tb.getHeight() + 10,
        hwnd, 0, frameClass.instance, this);

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
        ofn.lpstrFilter = "Rfb Session files (*.rfb, *.fbs)\0*.rfb;*.fbs\0" \
                          "All files (*.*)\0*.*\0";
        ofn.lpstrDefExt = "rfb";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn)) {
          openSessionFile(filename);
        }
      }
      break;
    case ID_CLOSEFILE:
      closeSessionFile();
      break;
    case ID_SESSION_INFO:
      {
        SessionInfoDialog sessionInfo(&cp, currentEncoding);
        sessionInfo.showDialog(getMainHandle());
      }
      break;
    case ID_PLAY:
      setPaused(false);
      break;
    case ID_PAUSE:
      setPaused(true);
      break;
    case ID_STOP:
      stopPlayback();
      break;
    case ID_PLAYPAUSE:
      if (rfbReader) {
        if (isPaused()) {
          setPaused(false);
        } else {
         setPaused(true);
        }
      }
      break;
    case ID_GOTO:
      {
        GotoPosDialog gotoPosDlg;
        if (gotoPosDlg.showDialog(getMainHandle())) {
          long gotoTime = min(gotoPosDlg.getPos(), sessionTimeMs);
          setPos(gotoTime);
          tb.updatePos(gotoTime);
          setPaused(isPaused());;
        }
      }
      break;
    case ID_LOOP:
      options.loopPlayback = !options.loopPlayback;
      if (options.loopPlayback) CheckMenuItem(hMenu, ID_LOOP, MF_CHECKED);
      else CheckMenuItem(hMenu, ID_LOOP, MF_UNCHECKED);
      break;
    case ID_RETURN:
      tb.processWM_COMMAND(wParam, lParam);
      break;
    case ID_OPTIONS:
      {
        OptionsDialog optionsDialog(&options, &supportedPF);
        optionsDialog.showDialog(getMainHandle());
      }
      break;
    case ID_EXIT:
      PostQuitMessage(0);
      break;
    case ID_HOMEPAGE:
      ShellExecute(getMainHandle(), _T("open"), _T("http://www.tightvnc.com/"),
        NULL, NULL, SW_SHOWDEFAULT);
      break;
    case ID_HELP_COMMANDLINESWITCHES:
      {
        InfoDialog usageDialog(usage_msg);
        usageDialog.showDialog(getMainHandle());
      }
      break;
    case ID_ABOUT:
      AboutDialog::instance.showDialog();
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
      MoveWindow(getFrameHandle(), 0, tb.getHeight(), r.right - r.left,
                 r.bottom - r.top - tb.getHeight(), TRUE);

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

  case WM_HSCROLL:
    tb.processWM_HSCROLL(wParam, lParam);
    break;
  
  case WM_NOTIFY:
    return tb.processWM_NOTIFY(wParam, lParam);

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
      if (isSeeking() || rewindFlag) {
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
    
    // Process play/pause by the left mouse button
  case WM_LBUTTONDOWN:
    SendMessage(getMainHandle(), WM_COMMAND, ID_PLAYPAUSE, 0);
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

void RfbPlayer::disableTBandMenuItems() {
  // Disable the menu items
  EnableMenuItem(hMenu, ID_CLOSEFILE, MF_GRAYED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_SESSION_INFO, MF_GRAYED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_FULLSCREEN, MF_GRAYED | MF_BYCOMMAND);
  ///EnableMenuItem(GetSubMenu(hMenu, 1), 1, MF_GRAYED | MF_BYPOSITION);
  EnableMenuItem(hMenu, ID_PLAYPAUSE, MF_GRAYED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_STOP, MF_GRAYED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_GOTO, MF_GRAYED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_LOOP, MF_GRAYED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_COPYTOCLIPBOARD, MF_GRAYED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_FRAMEEXTRACT, MF_GRAYED | MF_BYCOMMAND);
  
  // Disable the toolbar buttons and child controls
  tb.disable();
}

void RfbPlayer::enableTBandMenuItems() {
  // Enable the menu items
  EnableMenuItem(hMenu, ID_CLOSEFILE, MF_ENABLED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_SESSION_INFO, MF_ENABLED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_FULLSCREEN, MF_ENABLED | MF_BYCOMMAND);
  ///EnableMenuItem(GetSubMenu(hMenu, 1), 1, MF_ENABLED | MF_BYPOSITION);
  EnableMenuItem(hMenu, ID_PLAYPAUSE, MF_ENABLED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_STOP, MF_ENABLED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_GOTO, MF_ENABLED | MF_BYCOMMAND);
  EnableMenuItem(hMenu, ID_LOOP, MF_ENABLED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_COPYTOCLIPBOARD, MF_ENABLED | MF_BYCOMMAND);
  ///EnableMenuItem(hMenu, ID_FRAMEEXTRACT, MF_ENABLED | MF_BYCOMMAND);
  
  // Enable the toolbar buttons and child controls
  tb.enable();
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
  AdjustWindowRectEx(&r, GetWindowLong(getFrameHandle(), GWL_STYLE), TRUE, 
    GetWindowLong(getFrameHandle(), GWL_EXSTYLE));
  r.bottom += tb.getHeight(); // Include RfbPlayr's controls area
  AdjustWindowRect(&r, GetWindowLong(getMainHandle(), GWL_STYLE), FALSE);
  int x = max(0, (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2);
  int y = max(0, (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2);
  SetWindowPos(getMainHandle(), 0, x, y, r.right-r.left, r.bottom-r.top,
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

  // Update the cached client size
  GetClientRect(getFrameHandle(), &r);
  client_size = Rect(r.left, r.top, r.right, r.bottom);
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
  is->setSpeed(options.playbackSpeed);
  if (paused) is->pausePlayback();
  else is->resumePlayback();
}

void RfbPlayer::processMsg() {
  // Perform return if waitWhilePaused processed because 
  // rfbReader thread could receive the signal to close
  if (waitWhilePaused()) return;
  
  static long update_time = GetTickCount();
  try {
    if ((!isSeeking()) && ((GetTickCount() - update_time) > 250)
      && (!tb.isPosSliderDragging())) {
      // Update pos in the toolbar 4 times in 1 second
      tb.updatePos(getTimeOffset());
      update_time = GetTickCount();
    }
    RfbProto::processMsg();
  } catch (rdr::Exception e) {
    if (strcmp(e.str(), "[End Of File]") == 0) {
      rewind();
      setPaused(!options.loopPlayback);
      tb.updatePos(getTimeOffset());
      return;
    }
    // It's a special exception to perform backward seeking.
    // We only rewind the stream and seek the offset
    if (strcmp(e.str(), "[REWIND]") == 0) {
      rewindFlag = true; 
      long seekOffset = max(getSeekOffset(), imageDataStartTime);
      rewind();
      if (!stopped) setPos(seekOffset);
      else stopped = false;
      tb.updatePos(seekOffset);
      rewindFlag = false;
      return;
    }
    // It's a special exception which is used to terminate the playback
    if (strcmp(e.str(), "[TERMINATE]") == 0) {
      sessionTerminateThread *terminate = new sessionTerminateThread(this);
      terminate->start();
    } else {
      // Show the exception message and close the session playback
      is->pausePlayback();
      char message[256] = "\0";
      strcat(message, e.str());
      strcat(message, "\nMaybe you force wrong the pixel format for this session");
      MessageBox(getMainHandle(), message, "RFB Player", MB_OK | MB_ICONERROR);
      sessionTerminateThread *terminate = new sessionTerminateThread(this);
      terminate->start();
      return;
    }
  }
}

long ChoosePixelFormatDialog::pfIndex = DEFAULT_PF_INDEX;
bool ChoosePixelFormatDialog::bigEndian = false;

void RfbPlayer::serverInit() {
  RfbProto::serverInit();

  // Save the image data start time
  imageDataStartTime = is->getTimeOffset();

  // Resize the backing buffer
  buffer->setSize(cp.width, cp.height);

  // Check on the true colour mode
  if (!(cp.pf()).trueColour)
    throw rdr::Exception("This version plays only true color session!");

  // Set the session pixel format
  if (options.askPixelFormat) {
    ChoosePixelFormatDialog choosePixelFormatDialog(&supportedPF);
    if (choosePixelFormatDialog.showDialog(getMainHandle())) {
      long pixelFormatIndex = choosePixelFormatDialog.getPFIndex();
      if (pixelFormatIndex < 0) {
        options.autoDetectPF = true;
        options.setPF((PixelFormat *)&cp.pf());
      } else {
        options.autoDetectPF = false;
        options.setPF(&supportedPF[pixelFormatIndex]->PF);
        options.pixelFormat.bigEndian = choosePixelFormatDialog.isBigEndian();
      }
    } else {
      is->pausePlayback();
      throw rdr::Exception("[TERMINATE]");
    }
  } else {
    if (!options.commandLineParam) {
      if (options.autoDetectPF) {
        options.setPF((PixelFormat *)&cp.pf());
      } else {
        options.setPF(&supportedPF[options.pixelFormatIndex]->PF);
        options.pixelFormat.bigEndian = options.bigEndianFlag;
      }
    } else if (options.autoDetectPF) {
      options.setPF((PixelFormat *)&cp.pf());
    }
  }
  cp.setPF(options.pixelFormat);
  buffer->setPF(options.pixelFormat);

  // If the window is not maximised then resize it
  if (!(GetWindowLong(getMainHandle(), GWL_STYLE) & WS_MAXIMIZE))
    setFrameSize(cp.width, cp.height);

  // Set the window title and show it
  setTitle(cp.name());

  // Calculate the full session time and update posTrackBar control in toolbar
  sessionTimeMs = calculateSessionTime(fileName);
  tb.init(sessionTimeMs);
  tb.updatePos(getTimeOffset());

  setPaused(!options.autoPlay);
  // Restore the parameters from registry,
  // which was replaced by command-line parameters.
  if (options.commandLineParam) {
    options.readFromRegistry();
    options.commandLineParam = false;
  }
}

void RfbPlayer::setColourMapEntries(int first, int count, U16* rgbs) {
  vlog.debug("setColourMapEntries: first=%d, count=%d", first, count);
  throw rdr::Exception("Can't handle SetColourMapEntries message");
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
  if (options.acceptBell)
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
  currentEncoding = encoding;
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

bool RfbPlayer::waitWhilePaused() {
  bool result = false;
  while(isPaused() && !isSeeking()) {
    Sleep(20);
    result = true;
  }
  return result;
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
      MessageBox(getMainHandle(), e.str(), "RFB Player", MB_OK | MB_ICONERROR);
      return 0;
    }
  }
  return 0;
}

void RfbPlayer::init() {
  if (options.loopPlayback) CheckMenuItem(hMenu, ID_LOOP, MF_CHECKED);
  else CheckMenuItem(hMenu, ID_LOOP, MF_UNCHECKED);
}

void RfbPlayer::closeSessionFile() {
  DWORD dwStyle;
  RECT r;

  // Uncheck all toolbar buttons
  if (tb.getHandle()) {
    tb.checkButton(ID_PLAY, false);
    tb.checkButton(ID_PAUSE, false);
    tb.checkButton(ID_STOP, false);
  }

  // Stop playback and update the player state
  disableTBandMenuItems();
  if (rfbReader) {
    delete rfbReader->join();
    rfbReader = 0;
    delete [] fileName;
    fileName = 0;
  }
  blankBuffer();
  setTitle("None");
  options.playbackSpeed = 1.0;
  tb.init(0);
    
  // Change the player window size and frame size to default
  if ((dwStyle = GetWindowLong(getMainHandle(), GWL_STYLE)) & WS_MAXIMIZE) {
    dwStyle &= ~WS_MAXIMIZE;
    SetWindowLong(getMainHandle(), GWL_STYLE, dwStyle);
  }
  int x = max(0, (GetSystemMetrics(SM_CXSCREEN) - DEFAULT_PLAYER_WIDTH) / 2);
  int y = max(0, (GetSystemMetrics(SM_CYSCREEN) - DEFAULT_PLAYER_HEIGHT) / 2);
  SetWindowPos(getMainHandle(), 0, x, y, 
    DEFAULT_PLAYER_WIDTH, DEFAULT_PLAYER_HEIGHT, 
    SWP_NOZORDER | SWP_FRAMECHANGED);
  buffer->setSize(32, 32);
  calculateScrollBars();
  
  // Update the cached sizing information and repaint the frame window
  GetWindowRect(getFrameHandle(), &r);
  window_size = Rect(r.left, r.top, r.right, r.bottom);
  GetClientRect(getFrameHandle(), &r);
  client_size = Rect(r.left, r.top, r.right, r.bottom);
  InvalidateRect(getFrameHandle(), 0, TRUE);
  UpdateWindow(getFrameHandle());
}

void RfbPlayer::openSessionFile(char *_fileName) {
  fileName = strDup(_fileName);

  // Close the previous reading thread
  if (rfbReader) {
    delete rfbReader->join();
    rfbReader = 0;
  }
  blankBuffer();
  newSession(fileName);
  setSpeed(options.playbackSpeed);
  rfbReader = new rfbSessionReader(this);
  rfbReader->start();
  tb.setTimePos(0);
  enableTBandMenuItems();
}

void RfbPlayer::setPaused(bool paused) {
  if (paused) {
    is->pausePlayback();
    tb.checkButton(ID_PAUSE, true);
    tb.checkButton(ID_PLAY, false);
    tb.checkButton(ID_STOP, false);
  } else {
    if (is) is->resumePlayback();
    tb.checkButton(ID_PLAY, true);
    tb.checkButton(ID_STOP, false);
    tb.checkButton(ID_PAUSE, false);
  }
  tb.enableButton(ID_PAUSE, true);
  EnableMenuItem(hMenu, ID_STOP, MF_ENABLED | MF_BYCOMMAND);
}

void RfbPlayer::stopPlayback() {
  stopped = true;
  setPos(0);
  if (is) { 
    is->pausePlayback();
    is->interruptFrameDelay();
  }
  tb.checkButton(ID_STOP, true);
  tb.checkButton(ID_PLAY, false);
  tb.checkButton(ID_PAUSE, false);
  tb.enableButton(ID_PAUSE, false);
  tb.setTimePos(0);
  EnableMenuItem(hMenu, ID_STOP, MF_GRAYED | MF_BYCOMMAND);
}

void RfbPlayer::setSpeed(double speed) {
  if (speed > 0) {
    char speedStr[20] = "\0";
    double newSpeed = min(speed, MAX_SPEED);
    is->setSpeed(newSpeed);
    options.playbackSpeed = newSpeed;
    SendMessage(tb.getSpeedUpDownHwnd(), UDM_SETPOS, 
      0, MAKELONG((short)(newSpeed / 0.5), 0));
    sprintf(speedStr, "%.2f", newSpeed);
    SetWindowText(tb.getSpeedEditHwnd(), speedStr);
  }
}

double RfbPlayer::getSpeed() {
  return is->getSpeed();
}

void RfbPlayer::setPos(long pos) {
  is->setTimeOffset(max(pos, imageDataStartTime));
}

long RfbPlayer::getSeekOffset() {
  return is->getSeekOffset();
}

bool RfbPlayer::isSeeking() {
  if (is) return is->isSeeking();
  else return false;
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
  InfoDialog usageDialog(usage_msg);
  usageDialog.showDialog();
}

char *fileName = 0;

// playerOptions is the player options with default parameters values,
// it is used only for run the player with command-line parameters
PlayerOptions playerOptions;
bool print_usage = false;
bool print_upf_list = false;

bool processParams(int argc, char* argv[]) {
  playerOptions.commandLineParam = true;
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

    if ((strcasecmp(argv[i], "-pf") == 0) ||
        (strcasecmp(argv[i], "/pf") == 0) && (i < argc-1)) {
      char *pf = argv[++i];
      char rgb_order[4] = "\0";
      int order = RGB_ORDER;
      int r = -1, g = -1, b = -1;
      bool big_endian = false;
      if (strlen(pf) < 6) return false;
      while (strlen(pf)) {
        if ((pf[0] == 'r') || (pf[0] == 'R')) {
          if (r >=0 ) return false;
          r = atoi(++pf);
          strcat(rgb_order, "r");
          continue;
        }
        if ((pf[0] == 'g') || (pf[0] == 'G')) {
          if (g >=0 ) return false;
          g = atoi(++pf);
          strcat(rgb_order, "g");
          continue;
        }
        if (((pf[0] == 'b') || (pf[0] == 'B')) && 
             (pf[1] != 'e') && (pf[1] != 'E')) {
          if (b >=0 ) return false;
          b = atoi(++pf);
          strcat(rgb_order, "b");
          continue;
        }
        if ((pf[0] == 'l') || (pf[0] == 'L') || 
            (pf[0] == 'b') || (pf[0] == 'B')) {
          if (strcasecmp(pf, "le") == 0) break;
          if (strcasecmp(pf, "be") == 0) { big_endian = true; break;}
          return false;
        }
        pf++;
      }
      if ((r < 0) || (g < 0) || (b < 0) || (r + g + b > 32)) return false;
      if (strcasecmp(rgb_order, "rgb") == 0) { order = RGB_ORDER; }
      else if (strcasecmp(rgb_order, "rbg") == 0) { order = RBG_ORDER; }
      else if (strcasecmp(rgb_order, "grb") == 0) { order = GRB_ORDER; }
      else if (strcasecmp(rgb_order, "gbr") == 0) { order = GBR_ORDER; }
      else if (strcasecmp(rgb_order, "bgr") == 0) { order = BGR_ORDER; }
      else if (strcasecmp(rgb_order, "brg") == 0) { order = BRG_ORDER; }
      else return false;
      playerOptions.autoDetectPF = false;
      playerOptions.setPF(order, r, g, b, big_endian);
      continue;
    }

    if ((strcasecmp(argv[i], "-upf") == 0) ||
        (strcasecmp(argv[i], "/upf") == 0) && (i < argc-1)) {
      if ((i == argc - 1) || (argv[++i][0] == '-')) {
        print_upf_list = true;
        return true;
      }
      PixelFormatList userPfList;
      userPfList.readUserDefinedPF(HKEY_CURRENT_USER, UPF_REGISTRY_PATH);
      int index = userPfList.getIndexByPFName(argv[i]);
      if (index > 0) {
        playerOptions.autoDetectPF = false;
        playerOptions.setPF(&userPfList[index]->PF);
      } else {
        return false;
      }
      continue;
    }

    if ((strcasecmp(argv[i], "-speed") == 0) ||
        (strcasecmp(argv[i], "/speed") == 0) && (i < argc-1)) {
      double playbackSpeed = atof(argv[++i]);
      if (playbackSpeed <= 0) {
        return false;
      }
      playerOptions.playbackSpeed = playbackSpeed;
      continue;
    }

    if ((strcasecmp(argv[i], "-pos") == 0) ||
        (strcasecmp(argv[i], "/pos") == 0) && (i < argc-1)) {
      long initTime = atol(argv[++i]);
      if (initTime <= 0)
        return false;
      playerOptions.initTime = initTime;
      continue;
    }

    if ((strcasecmp(argv[i], "-autoplay") == 0) ||
        (strcasecmp(argv[i], "/autoplay") == 0) && (i < argc-1)) {
      playerOptions.autoPlay = true;
      continue;
    }

    if ((strcasecmp(argv[i], "-loop") == 0) ||
        (strcasecmp(argv[i], "/loop") == 0) && (i < argc-1)) {
      playerOptions.loopPlayback = true;
      continue;
    }

    if (i != argc - 1)
      return false;
  }

  fileName = strDup(argv[argc-1]);
  if (fileName[0] == '-') return false;
  else return true;
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
  // Show the user defined pixel formats if required
  if (print_upf_list) {
    int list_size = 256;
    char *upf_list = new char[list_size];
    PixelFormatList userPfList;
    userPfList.readUserDefinedPF(HKEY_CURRENT_USER, UPF_REGISTRY_PATH);
    strcpy(upf_list, "The list of the user defined pixel formats:\r\n");
    for (int i = userPfList.getDefaultPFCount(); i < userPfList.count(); i++) {
      if ((list_size - strlen(upf_list) - 1) < 
          (strlen(userPfList[i]->format_name) + 2)) {
        char *tmpStr = new char[list_size = 
          list_size + strlen(userPfList[i]->format_name) + 2];
        strcpy(tmpStr, upf_list);
        delete [] upf_list;
        upf_list = new char[list_size];
        strcpy(upf_list, tmpStr);
        delete [] tmpStr;
      }
      strcat(upf_list, userPfList[i]->format_name);
      strcat(upf_list, "\r\n");
    }
    InfoDialog upfInfoDialog(upf_list);
    upfInfoDialog.showDialog();
    delete [] upf_list;
    return 0;
  }

  // Create the player
  RfbPlayer *player = NULL;
  try {
    player = new RfbPlayer(fileName, &playerOptions);
  } catch (rdr::Exception e) {
    MessageBox(NULL, e.str(), "RFB Player", MB_OK | MB_ICONERROR);
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
    MessageBox(NULL, e.str(), "RFB Player", MB_OK | MB_ICONERROR);
  }

  return 0;
};
