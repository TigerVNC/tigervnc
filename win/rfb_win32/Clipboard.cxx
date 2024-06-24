/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2012-2019 Pierre Ossman for Cendio AB
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

// -=- Clipboard.cxx

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb_win32/Clipboard.h>
#include <rfb_win32/WMShatter.h>
#include <rfb/util.h>

#include <rfb/LogWriter.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("Clipboard");

//
// -=- Clipboard object
//

Clipboard::Clipboard()
  : MsgWindow("Clipboard"), notifier(nullptr), next_window(nullptr) {
  next_window = SetClipboardViewer(getHandle());
  vlog.debug("registered clipboard handler");
}

Clipboard::~Clipboard() {
  vlog.debug("removing %p from chain (next is %p)", getHandle(), next_window);
  ChangeClipboardChain(getHandle(), next_window);
}

LRESULT
Clipboard::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {

  case WM_CHANGECBCHAIN:
    vlog.debug("change clipboard chain (%I64x, %I64x)",
               (long long)wParam, (long long)lParam);
    if ((HWND) wParam == next_window)
      next_window = (HWND) lParam;
    else if (next_window != nullptr)
      SendMessage(next_window, msg, wParam, lParam);
    else
      vlog.error("bad clipboard chain change!");
    break;

  case WM_DRAWCLIPBOARD:
    {
      HWND owner = GetClipboardOwner();
      if (owner == getHandle()) {
        vlog.debug("local clipboard changed by me");
      } else {
        vlog.debug("local clipboard changed by %p", owner);

        if (notifier == nullptr)
          vlog.debug("no clipboard notifier registered");
        else
          notifier->notifyClipboardChanged(IsClipboardFormatAvailable(CF_UNICODETEXT));
			}
    }
    if (next_window)
		  SendMessage(next_window, msg, wParam, lParam);
    return 0;

  };
  return MsgWindow::processMessage(msg, wParam, lParam);
};

std::string
Clipboard::getClipText() {
  HGLOBAL cliphandle;
  wchar_t* clipdata;
  std::string utf8;

  // Open the clipboard
  if (!OpenClipboard(getHandle()))
    return nullptr;

  // Get the clipboard data
  cliphandle = GetClipboardData(CF_UNICODETEXT);
  if (!cliphandle) {
    CloseClipboard();
    return nullptr;
  }

  clipdata = (wchar_t*) GlobalLock(cliphandle);
  if (!clipdata) {
    CloseClipboard();
    return nullptr;
  }

  // Convert it to UTF-8
  utf8 = utf16ToUTF8(clipdata);

  // Release the buffer and close the clipboard
  GlobalUnlock(cliphandle);
  CloseClipboard();

  return convertLF(utf8.c_str());
}

void
Clipboard::setClipText(const char* text) {
  HANDLE clip_handle = nullptr;

  try {

    // - Firstly, we must open the clipboard
    if (!OpenClipboard(getHandle()))
      throw rdr::SystemException("unable to open Win32 clipboard", GetLastError());

    // - Convert the supplied clipboard text into UTF-16 format with CRLF
    std::string filtered(convertCRLF(text));
    std::wstring utf16(utf8ToUTF16(filtered.c_str()));

    // - Allocate global memory for the data
    clip_handle = ::GlobalAlloc(GMEM_MOVEABLE, (utf16.size() + 1) * 2);

    wchar_t* data = (wchar_t*) GlobalLock(clip_handle);
    wcscpy(data, utf16.c_str());
    GlobalUnlock(clip_handle);

    // - Next, we must clear out any existing data
    if (!EmptyClipboard())
      throw rdr::SystemException("unable to empty Win32 clipboard", GetLastError());

    // - Set the new clipboard data
    if (!SetClipboardData(CF_UNICODETEXT, clip_handle))
      throw rdr::SystemException("unable to set Win32 clipboard", GetLastError());
    clip_handle = nullptr;

    vlog.debug("set clipboard");
  } catch (rdr::Exception& e) {
    vlog.debug("%s", e.str());
  }

  // - Close the clipboard
  if (!CloseClipboard())
    vlog.debug("unable to close Win32 clipboard: %lu", GetLastError());
  else
    vlog.debug("closed clipboard");
  if (clip_handle) {
    vlog.debug("freeing clipboard handle");
    GlobalFree(clip_handle);
  }
}
