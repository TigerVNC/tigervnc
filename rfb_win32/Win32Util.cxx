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

// Win32Util.cxx

#include <rfb_win32/Win32Util.h>
#include <rdr/Exception.h>
#include <rdr/HexOutStream.h>


namespace rfb {
namespace win32 {

LogicalPalette::LogicalPalette() : palette(0), numEntries(0) {
  BYTE buf[sizeof(LOGPALETTE)+256*sizeof(PALETTEENTRY)];
  LOGPALETTE* logpal = (LOGPALETTE*)buf;
  logpal->palVersion = 0x300;
  logpal->palNumEntries = 256;
  for (int i=0; i<256;i++) {
    logpal->palPalEntry[i].peRed = 0;
    logpal->palPalEntry[i].peGreen = 0;
    logpal->palPalEntry[i].peBlue = 0;
    logpal->palPalEntry[i].peFlags = 0;
  }
  palette = CreatePalette(logpal);
  if (!palette)
    throw rdr::SystemException("failed to CreatePalette", GetLastError());
}

LogicalPalette::~LogicalPalette() {
  if (palette)
    if (!DeleteObject(palette))
      throw rdr::SystemException("del palette failed", GetLastError());
}

void LogicalPalette::setEntries(int start, int count, const Colour* cols) {
  if (numEntries < count) {
    ResizePalette(palette, start+count);
    numEntries = start+count;
  }
  PALETTEENTRY* logpal = new PALETTEENTRY[count];
  for (int i=0; i<count; i++) {
    logpal[i].peRed = cols[i].r >> 8;
    logpal[i].peGreen = cols[i].g >> 8;
    logpal[i].peBlue = cols[i].b >> 8;
    logpal[i].peFlags = 0;
  }
  UnrealizeObject(palette);
  SetPaletteEntries(palette, start, count, logpal);
  delete [] logpal;
}


static LogWriter dcLog("DeviceContext");

PixelFormat DeviceContext::getPF() const {
  return getPF(dc);
}

PixelFormat DeviceContext::getPF(HDC dc) {
  PixelFormat format;
  CompatibleBitmap bitmap(dc, 1, 1);

  // -=- Get the bitmap format information
  BitmapInfo bi;
  memset(&bi, 0, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biBitCount = 0;

  if (!::GetDIBits(dc, bitmap, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
    throw rdr::SystemException("unable to determine device pixel format", GetLastError());
  }
  if (!::GetDIBits(dc, bitmap, 0, 1, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
    throw rdr::SystemException("unable to determine pixel shifts/palette", GetLastError());
  }

  // -=- Munge the bitmap info here
  switch (bi.bmiHeader.biBitCount) {
  case 1:
  case 4:
    bi.bmiHeader.biBitCount = 8;
    break;
  case 24:
    bi.bmiHeader.biBitCount = 32;
    break;
  }
  bi.bmiHeader.biPlanes = 1;

  format.trueColour = bi.bmiHeader.biBitCount > 8;
  format.bigEndian = 0;
  format.bpp = format.depth = bi.bmiHeader.biBitCount;

  if (format.trueColour) {
    DWORD rMask=0, gMask=0, bMask=0;

    // Which true colour format is the DIB section using?
    switch (bi.bmiHeader.biCompression) {
    case BI_RGB:
      // Default RGB layout
      switch (bi.bmiHeader.biBitCount) {
      case 16:
        // RGB 555 - High Colour
        dcLog.info("16-bit High Colour");
        rMask = 0x7c00;
        bMask = 0x001f;
        gMask = 0x03e0;
        format.depth = 15;
        break;
      case 24:
      case 32:
        // RGB 888 - True Colour
        dcLog.info("24/32-bit High Colour");
        rMask = 0xff0000;
        gMask = 0x00ff00;
        bMask = 0x0000ff;
        format.depth = 24;
        break;
      default:
        dcLog.error("bits per pixel %u not supported", bi.bmiHeader.biBitCount);
        throw rdr::Exception("unknown bits per pixel specified");
      };
      break;
    case BI_BITFIELDS:
      // Custom RGB layout
      rMask = bi.mask.red;
      gMask = bi.mask.green;
      bMask = bi.mask.blue;
      dcLog.info("BitFields format: %lu, (%lx, %lx, %lx)",
        bi.bmiHeader.biBitCount, rMask, gMask, bMask);
      if (format.bpp == 32)
        format.depth = 24; // ...probably
      break;
    };

    // Convert the data we just retrieved
    initMaxAndShift(rMask, &format.redMax, &format.redShift);
    initMaxAndShift(gMask, &format.greenMax, &format.greenShift);
    initMaxAndShift(bMask, &format.blueMax, &format.blueShift);
  }

  return format;
}


WindowDC::WindowDC(HWND wnd) : hwnd(wnd) {
  dc = GetDC(wnd);
  if (!dc)
    throw rdr::SystemException("GetDC failed", GetLastError());
}
WindowDC::~WindowDC() {
  if (dc)
    ReleaseDC(hwnd, dc);
}


CompatibleDC::CompatibleDC(HDC existing) {
  dc = CreateCompatibleDC(existing);
  if (!dc)
    throw rdr::SystemException("CreateCompatibleDC failed", GetLastError());
}
CompatibleDC::~CompatibleDC() {
  if (dc)
    DeleteDC(dc);
}


BitmapDC::BitmapDC(HDC hdc, HBITMAP hbitmap) : CompatibleDC(hdc){
  oldBitmap = (HBITMAP)SelectObject(dc, hbitmap);
  if (!oldBitmap)
    throw rdr::SystemException("SelectObject to CompatibleDC failed",
    GetLastError());
}
BitmapDC::~BitmapDC() {
  SelectObject(dc, oldBitmap);
}


CompatibleBitmap::CompatibleBitmap(HDC hdc, int width, int height) {
  hbmp = CreateCompatibleBitmap(hdc, width, height);
  if (!hbmp)
    throw rdr::SystemException("CreateCompatibleBitmap() failed", 
    GetLastError());
}
CompatibleBitmap::~CompatibleBitmap() {
  if (hbmp) DeleteObject(hbmp);
}


PaletteSelector::PaletteSelector(HDC dc, HPALETTE pal) : device(dc), redrawRequired(false) {
  oldPal = SelectPalette(dc, pal, FALSE);
  redrawRequired = RealizePalette(dc) > 0;
}
PaletteSelector::~PaletteSelector() {
  if (oldPal) SelectPalette(device, oldPal, TRUE);
}


IconInfo::IconInfo(HICON icon) {
  if (!GetIconInfo(icon, this))
    throw rdr::SystemException("GetIconInfo() failed", GetLastError());
}
IconInfo::~IconInfo() {
  if (hbmColor)
    DeleteObject(hbmColor);
  if (hbmMask)
    DeleteObject(hbmMask);
}


ModuleFileName::ModuleFileName(HMODULE module) : TCharArray(MAX_PATH) {
  if (!module) module = GetModuleHandle(0);
  if (!GetModuleFileName(module, buf, MAX_PATH))
    buf[0] = 0;
}


FileVersionInfo::FileVersionInfo(const TCHAR* filename) {
  // Get executable name
  ModuleFileName exeName;
  if (!filename) filename = exeName.buf;

  // Get version info size
  DWORD handle;
  int size = GetFileVersionInfoSize((TCHAR*)filename, &handle);
  if (!size)
    throw rdr::SystemException("GetVersionInfoSize failed", GetLastError());

  // Get version info
  buf = new TCHAR[size];
  if (!GetFileVersionInfo((TCHAR*)filename, handle, size, buf))
    throw rdr::SystemException("GetVersionInfo failed", GetLastError());
}

const TCHAR* FileVersionInfo::getVerString(const TCHAR* name, DWORD langId) {
  char langIdBuf[sizeof(langId)];
  for (int i=sizeof(langIdBuf)-1; i>=0; i--) {
    langIdBuf[i] = langId & 0xff;
    langId = langId >> 8;
  }

  TCharArray langIdStr = rdr::HexOutStream::binToHexStr(langIdBuf, sizeof(langId));
  TCharArray infoName(_tcslen(_T("StringFileInfo")) + 4 + _tcslen(name) + _tcslen(langIdStr.buf));
  _stprintf(infoName.buf, _T("\\StringFileInfo\\%s\\%s"), langIdStr.buf, name);

  // Locate the required version string within the version info
  TCHAR* buffer = 0;
  UINT length = 0;
  if (!VerQueryValue(buf, infoName.buf, (void**)&buffer, &length)) {
    printf("unable to find %s version string", CStr(infoName.buf));
    throw rdr::Exception("VerQueryValue failed");
  }
  return buffer;
}


bool splitPath(const TCHAR* path, TCHAR** dir, TCHAR** file) {
  return tstrSplit(path, '\\', dir, file, true);
}


static LogWriter dfbLog("DynamicFn");

DynamicFnBase::DynamicFnBase(const TCHAR* dllName, const char* fnName) : dllHandle(0), fnPtr(0) {
  dllHandle = LoadLibrary(dllName);
  if (!dllHandle) {
    dfbLog.info("DLL %s not found (%d)", (const char*)CStr(dllName), GetLastError());
    return;
  }
  fnPtr = GetProcAddress(dllHandle, fnName);
  if (!fnPtr)
    dfbLog.info("proc %s not found in %s (%d)", fnName, (const char*)CStr(dllName), GetLastError());
}

DynamicFnBase::~DynamicFnBase() {
  if (dllHandle)
    FreeLibrary(dllHandle);
}


static LogWriter miLog("MonitorInfo");

MonitorInfo::MonitorInfo(HWND window) {
#if (WINVER >= 0x0500)
  typedef HMONITOR (WINAPI *_MonitorFromWindow_proto)(HWND,DWORD);
  rfb::win32::DynamicFn<_MonitorFromWindow_proto> _MonitorFromWindow(_T("user32.dll"), "MonitorFromWindow");
  typedef BOOL (WINAPI *_GetMonitorInfo_proto)(HMONITOR,LPMONITORINFO);
  rfb::win32::DynamicFn<_GetMonitorInfo_proto> _GetMonitorInfo(_T("user32.dll"), "GetMonitorInfoA");

  // Can we dynamically link to the monitor functions?
  if (_MonitorFromWindow.isValid()) {
    if (_GetMonitorInfo.isValid()) {
      HMONITOR monitor = (*_MonitorFromWindow)(window, MONITOR_DEFAULTTONEAREST);
      miLog.debug("monitor=%lx", monitor);
      if (monitor) {
        memset(this, 0, sizeof(MONITORINFOEXA));
        cbSize = sizeof(MONITORINFOEXA);
        if ((*_GetMonitorInfo)(monitor, this)) {
          miLog.debug("monitor is %d,%d-%d,%d", rcMonitor.left, rcMonitor.top, rcMonitor.right, rcMonitor.bottom);
          miLog.debug("work area is %d,%d-%d,%d", rcWork.left, rcWork.top, rcWork.right, rcWork.bottom);
          miLog.debug("device is \"%s\"", szDevice);
          return;
        }
        miLog.error("failed to get monitor info: %ld", GetLastError());
      }
    } else {
      miLog.debug("GetMonitorInfo not found");
    }
  } else {
      miLog.debug("MonitorFromWindow not found");
  }
#else
#pragma message ("not building in GetMonitorInfo")
  cbSize = sizeof(MonitorInfo);
  szDevice[0] = 0;
#endif

  // Legacy fallbacks - just return the desktop settings
  miLog.debug("using legacy fall-backs");
  HWND desktop = GetDesktopWindow();
  GetWindowRect(desktop, &rcMonitor);
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
  dwFlags = 0;
}


#if (WINVER >= 0x0500)

struct moveToMonitorData {
  HWND window;
  const char* monitorName;
};

typedef BOOL (WINAPI *_GetMonitorInfo_proto)(HMONITOR,LPMONITORINFO);
static rfb::win32::DynamicFn<_GetMonitorInfo_proto> _GetMonitorInfo(_T("user32.dll"), "GetMonitorInfoA");

static BOOL CALLBACK moveToMonitorEnumProc(HMONITOR monitor,
                                    HDC dc,
                                    LPRECT pos,
                                    LPARAM d) {
  moveToMonitorData* data = (moveToMonitorData*)d;
  MONITORINFOEXA info;
  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);

  if ((*_GetMonitorInfo)(monitor, &info)) {
    if (stricmp(data->monitorName, info.szDevice) == 0) {
      SetWindowPos(data->window, 0,
        info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right, info.rcMonitor.bottom,
        SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
      return FALSE;
    }
  }

  return TRUE;
}

#endif

void moveToMonitor(HWND handle, const char* device) {
  miLog.debug("moveToMonitor %s", device);

#if (WINVER >= 0x500)
  typedef BOOL (WINAPI *_EnumDisplayMonitors_proto)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
  rfb::win32::DynamicFn<_EnumDisplayMonitors_proto> _EnumDisplayMonitors(_T("user32.dll"), "EnumDisplayMonitors");
  if (!_EnumDisplayMonitors.isValid()) {
    miLog.debug("EnumDisplayMonitors not found");
    return;
  }

  moveToMonitorData data;
  data.window = handle;
  data.monitorName = device;

  (*_EnumDisplayMonitors)(0, 0, &moveToMonitorEnumProc, (LPARAM)&data);
#endif
}


void centerWindow(HWND handle, HWND parent, bool clipToParent) {
  RECT r;
  if (parent && IsWindowVisible(parent)) {
    if (!GetWindowRect(parent, &r)) return;
  } else {
    MonitorInfo mi(handle);
    r=mi.rcWork;
  }
  centerWindow(handle, r, clipToParent);
}

void centerWindow(HWND handle, const RECT& r, bool clipToRect) {
  RECT wr;
  if (!GetWindowRect(handle, &wr)) return;
  int w = wr.right-wr.left;
  int h = wr.bottom-wr.top;
  if (clipToRect) {
    w = min(r.right-r.left, w);
    h = min(r.bottom-r.top, h);
  }
  int x = (r.left + r.right - w)/2;
  int y = (r.top + r.bottom - h)/2;
  UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | (clipToRect ? 0 : SWP_NOSIZE);
  SetWindowPos(handle, 0, x, y, w, h, flags);
}


int MsgBox(HWND parent, const TCHAR* msg, UINT flags) {
  const TCHAR* msgType = 0;
  UINT tflags = flags & 0x70;
  if (tflags == MB_ICONHAND)
    msgType = _T("Error");
  else if (tflags == MB_ICONQUESTION)
    msgType = _T("Question");
  else if (tflags == MB_ICONEXCLAMATION)
    msgType = _T("Warning");
  else if (tflags == MB_ICONASTERISK)
    msgType = _T("Information");
  flags |= MB_TOPMOST | MB_SETFOREGROUND;
  int len = _tcslen(AppName.buf) + 1;
  if (msgType) len += _tcslen(msgType) + 3;
  TCharArray title = new TCHAR[len];
  _tcscpy(title.buf, AppName.buf);
  if (msgType) {
    _tcscat(title.buf, _T(" : "));
    _tcscat(title.buf, msgType);
  }
  return MessageBox(parent, msg, title.buf, flags);
}


};
};
