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

// -=- Win32Util.h

// Miscellaneous but useful Win32 API utility functions & classes.
// In particular, a set of classes which wrap GDI objects,
// and some to handle palettes.

#ifndef __RFB_WIN32_GDIUTIL_H__
#define __RFB_WIN32_GDIUTIL_H__

#include <rfb/ColourMap.h>
#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>
#include <rfb_win32/TCharArray.h>

namespace rfb {

  namespace win32 {

    class LogicalPalette {
    public:
      LogicalPalette();
      ~LogicalPalette();
      void setEntries(int start, int count, const Colour* cols);
      HPALETTE getHandle() {return palette;}
    protected:
      HPALETTE palette;
      int numEntries;
    };

    class DeviceContext {
    public:
      DeviceContext() : dc(0) {}
      virtual ~DeviceContext() {}
      operator HDC() const {return dc;}
      PixelFormat getPF() const;
      static PixelFormat getPF(HDC dc);
    protected:
      HDC dc;
    };

    class WindowDC : public DeviceContext {
    public:
      WindowDC(HWND wnd);
      virtual ~WindowDC();
    protected:
      HWND hwnd;
    };

    class CompatibleDC : public DeviceContext {
    public:
      CompatibleDC(HDC existing);
      virtual ~CompatibleDC();
    };

    class BitmapDC : public CompatibleDC {
    public:
      BitmapDC(HDC hdc, HBITMAP hbitmap);
      ~BitmapDC();
    protected:
      HBITMAP oldBitmap;
    };

    class CompatibleBitmap {
    public:
      CompatibleBitmap(HDC hdc, int width, int height);
      virtual ~CompatibleBitmap();
      operator HBITMAP() const {return hbmp;}
    protected:
      HBITMAP hbmp;
    };

    struct BitmapInfo {
      BITMAPINFOHEADER bmiHeader;
      union {
        struct {
          DWORD red;
          DWORD green;
          DWORD blue;
        } mask;
        RGBQUAD color[256];
      };
    };

    inline void initMaxAndShift(DWORD mask, int* max, int* shift) {
      for ((*shift) = 0; (mask & 1) == 0; (*shift)++) mask >>= 1;
        (*max) = (rdr::U16)mask;
    }

    class PaletteSelector {
    public:
      PaletteSelector(HDC dc, HPALETTE pal);
      ~PaletteSelector();
      bool isRedrawRequired() {return redrawRequired;}
    protected:
      HPALETTE oldPal;
      HDC device;
      bool redrawRequired;
    };

    struct IconInfo : public ICONINFO {
      IconInfo(HICON icon);
      ~IconInfo();
    };

    struct ModuleFileName : public TCharArray {
      ModuleFileName(HMODULE module=0);
    };

    struct FileVersionInfo : public TCharArray {
      FileVersionInfo(const TCHAR* filename=0);
      const TCHAR* getVerString(const TCHAR* name, DWORD langId = 0x080904b0);
    };

    bool splitPath(const TCHAR* path, TCHAR** dir, TCHAR** file);

    class DynamicFnBase {
    public:
      DynamicFnBase(const TCHAR* dllName, const char* fnName);
      ~DynamicFnBase();
      bool isValid() const {return fnPtr != 0;}
    protected:
      void* fnPtr;
      HMODULE dllHandle;
    };

    template<class T> class DynamicFn : public DynamicFnBase {
    public:
      DynamicFn(const TCHAR* dllName, const char* fnName) : DynamicFnBase(dllName, fnName) {}
      T operator *() const {return (T)fnPtr;};
    };

    // Structure containing info on the monitor nearest the window.
    // Copes with multi-monitor OSes and older ones.
#if (WINVER >= 0x0500)
    struct MonitorInfo : MONITORINFOEXA {
      MonitorInfo(HWND hwnd);
    };
#else
    struct MonitorInfo {
      MonitorInfo(HWND hwnd);
      DWORD cbSize;
      RECT rcMonitor;
      RECT rcWork;
      DWORD dwFlags;
      char szDevice[1]; // Always null...
    };
#endif
    void moveToMonitor(HWND handle, const char* device);

    class Handle {
    public:
      Handle(HANDLE h_=0) : h(h_) {}
      ~Handle() {
        if (h) CloseHandle(h);
      }
      operator HANDLE() {return h;}
      HANDLE h;
    };

    // Center the window to a rectangle, or to a parent window.
    // Optionally, resize the window to lay within the rect or parent window
    // If the parent window is NULL then the working area if the window's
    // current monitor is used instead.
    void centerWindow(HWND handle, const RECT& r, bool clipToRect=false);
    void centerWindow(HWND handle, HWND parent, bool clipToRect=false);

    // MsgBox helper function.  Define rfb::win32::AppName somewhere in your
    // code and MsgBox will use its value in informational messages.
    extern TStr AppName;
    int MsgBox(HWND parent, const TCHAR* message, UINT flags);

    // Get the computer name
    struct ComputerName : TCharArray {
      ComputerName() : TCharArray(MAX_COMPUTERNAME_LENGTH+1) {
        ULONG namelength = MAX_COMPUTERNAME_LENGTH+1;
        if (!GetComputerName(buf, &namelength))
          _tcscpy(buf, _T(""));
      }
    };

    // Allocate and/or manage LocalAlloc memory.
    struct LocalMem {
      LocalMem(int size) : ptr(LocalAlloc(LMEM_FIXED, size)) {
        if (!ptr) throw rdr::SystemException("LocalAlloc", GetLastError());
      }
      LocalMem(void* p) : ptr(p) {}
      ~LocalMem() {LocalFree(ptr);}
      operator void*() {return ptr;}
      void* takePtr() {
        void* t = ptr; ptr = 0; return t;
      }
      void* ptr;
    };

  };

};

#endif
