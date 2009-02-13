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

#include <windows.h>
#include <winsock2.h>
#include <vncviewer/UserPasswdDialog.h>
#include <vncviewer/CConn.h>
#include <vncviewer/CConnThread.h>
#include <vncviewer/resource.h>
#include <rfb/encodings.h>
#include <rfb/secTypes.h>
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/CMsgWriter.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>
#include <rfb_win32/AboutDialog.h>

using namespace rfb;
using namespace rfb::win32;
using namespace rdr;

// - Statics & consts

static LogWriter vlog("CConn");


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
const int IDM_ZOOM_IN = ID_ZOOM_IN;
const int IDM_ZOOM_OUT = ID_ZOOM_OUT;
const int IDM_ACTUAL_SIZE = ID_ACTUAL_SIZE;
const int IDM_AUTO_SIZE = ID_AUTO_SIZE;


static IntParameter debugDelay("DebugDelay","Milliseconds to display inverted "
                               "pixel data - a debugging feature", 0);

const int scaleValues[9] = {10, 25, 50, 75, 90, 100, 125, 150, 200};
const int scaleCount = 9;


//
// -=- CConn implementation
//

RegKey            CConn::userConfigKey;


CConn::CConn() 
  : window(0), sameMachine(false), encodingChange(false), formatChange(false), 
    lastUsedEncoding_(encodingRaw), sock(0), sockEvent(CreateEvent(0, TRUE, FALSE, 0)), 
    reverseConnection(false), requestUpdate(false), isClosed_(false) {
}

CConn::~CConn() {
  delete window;
}

bool CConn::initialise(network::Socket* s, bool reverse) {
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

  // - Set which auth schemes we support, in order of preference
  addSecType(secTypeVncAuth);
  addSecType(secTypeNone);

  // Start the RFB protocol
  sock = s;
  reverseConnection = reverse;
  initialiseProtocol();

  m_fileTransfer.initialize(&s->inStream(), &s->outStream());

  return true;
}


void
CConn::applyOptions(CConnOptions& opt) {
  // - If any encoding-related settings have changed then we must
  //   notify the server of the new settings
  encodingChange |= ((options.useLocalCursor != opt.useLocalCursor) ||
                     (options.useDesktopResize != opt.useDesktopResize) ||
                     (options.customCompressLevel != opt.customCompressLevel) ||
                     (options.compressLevel != opt.compressLevel) ||
                     (options.noJpeg != opt.noJpeg) ||
                     (options.qualityLevel != opt.qualityLevel) ||
                     (options.preferredEncoding != opt.preferredEncoding));

  // - If the preferred pixel format has changed then notify the server
  formatChange |= (options.fullColour != opt.fullColour);
  if (!opt.fullColour)
    formatChange |= (options.lowColourLevel != opt.lowColourLevel);

  // - Save the new set of options
  options = opt;

  // - Set optional features in ConnParams
  cp.supportsLocalCursor = options.useLocalCursor;
  cp.supportsDesktopResize = options.useDesktopResize;
  cp.supportsDesktopRename = true;
  cp.customCompressLevel = options.customCompressLevel;
  cp.compressLevel = options.compressLevel;
  cp.noJpeg = options.noJpeg;
  cp.qualityLevel = options.qualityLevel;

  // - Configure connection sharing on/off
  setShared(options.shared);

  // - Whether to use protocol 3.3 for legacy compatibility
  setProtocol3_3(options.protocol3_3);

  // - Apply settings that affect the window, if it is visible
  if (window) {
    window->setMonitor(options.monitor.buf);
    window->setFullscreen(options.fullScreen);
    window->setEmulate3(options.emulate3);
    window->setPointerEventInterval(options.pointerEventInterval);
    window->setMenuKey(options.menuKey);
    window->setDisableWinKeys(options.disableWinKeys);
    window->setShowToolbar(options.showToolbar);
    window->printScale();
    if (options.autoScaling) {
      window->setAutoScaling(true);
    } else {
      window->setAutoScaling(false);
      window->setDesktopScale(options.scale);
    }
    if (!options.useLocalCursor)
      window->setCursor(0, 0, Point(), 0, 0);
  }
}


void
CConn::displayChanged() {
  // Display format has changed - recalculate the full-colour pixel format
  calculateFullColourPF();
}

void
CConn::paintCompleted() {
  // A repaint message has just completed - request next update if necessary
  requestNewUpdate();
}

bool
CConn::sysCommand(WPARAM wParam, LPARAM lParam) {
  // - If it's one of our (F8 Menu) messages
  switch (wParam) {
  case IDM_FULLSCREEN:
    options.fullScreen = !window->isFullscreen();
    window->setFullscreen(options.fullScreen);
    return true;
  case IDM_ZOOM_IN:
  case IDM_ZOOM_OUT:
    {
      if (options.autoScaling) {
        options.scale = window->getDesktopScale();
        options.autoScaling = false;
        window->setAutoScaling(false);
      }
      if (wParam == IDM_ZOOM_IN) {
        for (int i = 0; i < scaleCount; i++)
          if (options.scale < scaleValues[i]) { 
            options.scale = scaleValues[i];
            break;
          }
      } else {
        for (int i = scaleCount-1; i >= 0; i--)
          if (options.scale > scaleValues[i]) { 
            options.scale = scaleValues[i];
            break;
          }
      }
      if (options.scale != window->getDesktopScale()) 
        window->setDesktopScale(options.scale);
    }
    return true;
  case IDM_ACTUAL_SIZE:
    if (options.autoScaling) {
      options.autoScaling = false;
      window->setAutoScaling(false);
    }
    options.scale = 100;
    window->setDesktopScale(100);
    return true;
  case IDM_AUTO_SIZE:
    options.autoScaling = !options.autoScaling;
    window->setAutoScaling(options.autoScaling);
    if (!options.autoScaling) options.scale = window->getDesktopScale();
    return true;
  case IDM_SHOW_TOOLBAR:
    options.showToolbar = !window->isToolbarEnabled();
    window->setShowToolbar(options.showToolbar);
    return true;
  case IDM_CTRL_KEY:
    window->kbd.keyEvent(this, VK_CONTROL, 0, !window->kbd.keyPressed(VK_CONTROL));
    return true;
  case IDM_ALT_KEY:
    window->kbd.keyEvent(this, VK_MENU, 0, !window->kbd.keyPressed(VK_MENU));
    return true;
  case IDM_SEND_MENU_KEY:
    window->kbd.keyEvent(this, options.menuKey, 0, true);
    window->kbd.keyEvent(this, options.menuKey, 0, false);
    return true;
  case IDM_SEND_CAD:
    window->kbd.keyEvent(this, VK_CONTROL, 0, true);
    window->kbd.keyEvent(this, VK_MENU, 0, true);
    window->kbd.keyEvent(this, VK_DELETE, 0x1000000, true);
    window->kbd.keyEvent(this, VK_DELETE, 0x1000000, false);
    window->kbd.keyEvent(this, VK_MENU, 0, false);
    window->kbd.keyEvent(this, VK_CONTROL, 0, false);
    return true;
  case IDM_SEND_CTLESC:
    window->kbd.keyEvent(this, VK_CONTROL, 0, true);
    window->kbd.keyEvent(this, VK_ESCAPE, 0, true);
    window->kbd.keyEvent(this, VK_ESCAPE, 0, false);
    window->kbd.keyEvent(this, VK_CONTROL, 0, false);
    return true;
  case IDM_REQUEST_REFRESH:
    try {
      writer()->writeFramebufferUpdateRequest(Rect(0,0,cp.width,cp.height), false);
      requestUpdate = false;
    } catch (rdr::Exception& e) {
      close(e.str());
    }
    return true;
  case IDM_NEWCONN:
    {
      Thread* newThread = new CConnThread;
    }
    return true;
  case IDM_OPTIONS:
    // Update the monitor device name in the CConnOptions instance
    options.monitor.replaceBuf(window->getMonitor());
    showOptionsDialog();
    return true;
  case IDM_INFO:
    infoDialog.showDialog(this);
    return true;
  case IDM_ABOUT:
    AboutDialog::instance.showDialog();
    return true;
  case IDM_FILE_TRANSFER:
    m_fileTransfer.show(window->getHandle());
    return true;
  case IDM_CONN_SAVE_AS:
    return true;
  };
  return false;
}


void
CConn::closeWindow() {
  vlog.info("window closed");
  close();
}


void
CConn::refreshMenu(bool enableSysItems) {
  HMENU menu = GetSystemMenu(window->getHandle(), FALSE);

  if (!enableSysItems) {
    // Gray out menu items that might cause a World Of Pain
    EnableMenuItem(menu, SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(menu, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(menu, SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(menu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(menu, SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
  }

  // Update the modifier key menu items
  UINT ctrlCheckFlags = window->kbd.keyPressed(VK_CONTROL) ? MF_CHECKED : MF_UNCHECKED;
  UINT altCheckFlags = window->kbd.keyPressed(VK_MENU) ? MF_CHECKED : MF_UNCHECKED;
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

  // Set the menu fullscreen option tick
  CheckMenuItem(menu, IDM_FULLSCREEN, (window->isFullscreen() ? MF_CHECKED : 0) | MF_BYCOMMAND);

  // Set the menu toolbar option tick
  int toolbarFlags = window->isToolbarEnabled() ? MF_CHECKED : 0;
  CheckMenuItem(menu, IDM_SHOW_TOOLBAR, MF_BYCOMMAND | toolbarFlags);

  // In the full-screen mode, "Show toolbar" should be grayed.
  toolbarFlags = window->isFullscreen() ? MF_GRAYED : MF_ENABLED;
  EnableMenuItem(menu, IDM_SHOW_TOOLBAR, MF_BYCOMMAND | toolbarFlags);
}


void
CConn::blockCallback() {
  // - An InStream has blocked on I/O while processing an RFB message
  //   We re-enable socket event notifications, so we'll know when more
  //   data is available, then we sit and dispatch window events until
  //   the notification arrives.
  if (!isClosed()) {
    if (WSAEventSelect(sock->getFd(), sockEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
      throw rdr::SystemException("Unable to wait for sokcet data", WSAGetLastError());
  }
  while (true) {
    // If we have closed then we can't block waiting for data
    if (isClosed())
      throw rdr::EndOfStream();

    // Wait for socket data, or a message to process
    DWORD result = MsgWaitForMultipleObjects(1, &sockEvent.h, FALSE, INFINITE, QS_ALLINPUT);
    if (result == WAIT_OBJECT_0) {
      // - Network event notification.  Return control to I/O routine.
      break;
    } else if (result == WAIT_FAILED) {
      // - The wait operation failed - raise an exception
      throw rdr::SystemException("blockCallback wait error", GetLastError());
    }

    // - There should be a message in the message queue
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // IMPORTANT: We mustn't call TranslateMessage() here, because instead we
      // call ToAscii() in CKeyboard::keyEvent().  ToAscii() stores dead key
      // state from one call to the next, which would be messed up by calls to
      // TranslateMessage() (actually it looks like TranslateMessage() calls
      // ToAscii() internally).
      DispatchMessage(&msg);
    }
  }

  // Before we return control to the InStream, reset the network event
  WSAEventSelect(sock->getFd(), sockEvent, 0);
  ResetEvent(sockEvent);
}


void CConn::keyEvent(rdr::U32 key, bool down) {
  if (!options.sendKeyEvents) return;
  try {
    writer()->keyEvent(key, down);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}
void CConn::pointerEvent(const Point& pos, int buttonMask) {
  if (!options.sendPtrEvents) return;
  try {
    writer()->pointerEvent(pos, buttonMask);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}
void CConn::clientCutText(const char* str, int len) {
  if (!options.clientCutText) return;
  if (state() != RFBSTATE_NORMAL) return;
  try {
    writer()->clientCutText(str, len);
  } catch (rdr::Exception& e) {
    close(e.str());
  }
}


CSecurity* CConn::getCSecurity(int secType)
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
CConn::setColourMapEntries(int first, int count, U16* rgbs) {
  vlog.debug("setColourMapEntries: first=%d, count=%d", first, count);
  int i;
  for (i=0;i<count;i++)
    window->setColour(i+first, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);
  // *** change to 0, 256?
  window->refreshWindowPalette(first, count);
}

void
CConn::bell() {
  if (options.acceptBell)
    MessageBeep((UINT)-1);
}


void
CConn::setDesktopSize(int w, int h) {
  vlog.debug("setDesktopSize %dx%d", w, h);

  // Resize the window's buffer
  if (window)
    window->setSize(w, h);

  // Tell the underlying CConnection
  CConnection::setDesktopSize(w, h);
}

void
CConn::setCursor(int w, int h, const Point& hotspot, void* data, void* mask) {
  if (!options.useLocalCursor) return;

  // Set the window to use the new cursor
  window->setCursor(w, h, hotspot, data, mask);
}


void
CConn::close(const char* reason) {
  // If already closed then ignore this
  if (isClosed())
    return;

  // Hide the window, if it exists
  if (window)
    ShowWindow(window->getHandle(), SW_HIDE);

  // Save the reason & flag that we're closed & shutdown the socket
  isClosed_ = true;
  closeReason_.replaceBuf(strDup(reason));
  sock->shutdown();
}


void
CConn::showOptionsDialog() {
  optionsDialog.showDialog(this);
}


void
CConn::framebufferUpdateEnd() {
  if (debugDelay != 0) {
    vlog.debug("debug delay %d",(int)debugDelay);
    UpdateWindow(window->getHandle());
    Sleep(debugDelay);
    std::list<rfb::Rect>::iterator i;
    for (i = debugRects.begin(); i != debugRects.end(); i++) {
      window->invertRect(*i);
    }
    debugRects.clear();
  }
  if (options.autoSelect)
    autoSelectFormatAndEncoding();

  // Always request the next update
  requestUpdate = true;

  // Check that at least part of the window has changed
  if (!GetUpdateRect(window->getHandle(), 0, FALSE)) {
    if (!(GetWindowLong(window->getHandle(), GWL_STYLE) & WS_MINIMIZE))
      requestNewUpdate();
  }

  // Make sure the local cursor is shown
  window->showCursor();
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
CConn::autoSelectFormatAndEncoding() {
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
CConn::requestNewUpdate() {
  if (!requestUpdate) return;

  if (formatChange) {
    // Select the required pixel format
    if (options.fullColour) {
      window->setPF(fullColourPF);
    } else {
      switch (options.lowColourLevel) {
      case 0:
        window->setPF(PixelFormat(8,3,0,1,1,1,1,2,1,0));
        break;
      case 1:
        window->setPF(PixelFormat(8,6,0,1,3,3,3,4,2,0));
        break;
      case 2:
        window->setPF(PixelFormat(8,8,0,0,0,0,0,0,0,0));
        break;
      }
    }

    // Print the current pixel format
    char str[256];
    window->getPF().print(str, 256);
    vlog.info("Using pixel format %s",str);

    // Save the connection pixel format and tell server to use it
    cp.setPF(window->getPF());
    writer()->writeSetPixelFormat(cp.pf());

    // Correct the local window's palette
    if (!window->getNativePF().trueColour)
      window->refreshWindowPalette(0, 1 << cp.pf().depth);
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
CConn::calculateFullColourPF() {
  // If the server is palette based then use palette locally
  // Also, don't bother doing bgr222
  if (!serverDefaultPF.trueColour || (serverDefaultPF.depth < 6)) {
    fullColourPF = serverDefaultPF;
    options.fullColour = true;
  } else {
    // If server is trueColour, use lowest depth PF
    PixelFormat native = window->getNativePF();
    if ((serverDefaultPF.bpp < native.bpp) ||
        ((serverDefaultPF.bpp == native.bpp) &&
        (serverDefaultPF.depth < native.depth)))
      fullColourPF = serverDefaultPF;
    else
      fullColourPF = window->getNativePF();
  }
  formatChange = true;
}


void
CConn::setName(const char* name) {
  if (window)
    window->setName(name);
  CConnection::setName(name);
}


void CConn::serverInit() {
  CConnection::serverInit();

  // If using AutoSelect with old servers, start in FullColor
  // mode. See comment in autoSelectFormatAndEncoding. 
  if (cp.beforeVersion(3, 8) && options.autoSelect) {
    options.fullColour = true;
  }

  // Show the window
  window = new DesktopWindow(this);
  window->setName(cp.name());
  window->setShowToolbar(options.showToolbar);
  window->setSize(cp.width, cp.height);
  applyOptions(options);

  // Save the server's current format
  serverDefaultPF = cp.pf();

  // Calculate the full-colour format to use
  calculateFullColourPF();

  // Request the initial update
  vlog.info("requesting initial update");
  formatChange = encodingChange = requestUpdate = true;
  requestNewUpdate();

  // Update the window menu
  HMENU wndmenu = GetSystemMenu(window->getHandle(), FALSE);
  int toolbarChecked = options.showToolbar ? MF_CHECKED : 0;

  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  AppendMenu(wndmenu, MF_STRING, IDM_FULLSCREEN, _T("&Full screen"));
  AppendMenu(wndmenu, MF_STRING | toolbarChecked, IDM_SHOW_TOOLBAR,
             _T("Show tool&bar"));
  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  AppendMenu(wndmenu, MF_STRING, IDM_CTRL_KEY, _T("Ctr&l"));
  AppendMenu(wndmenu, MF_STRING, IDM_ALT_KEY, _T("Al&t"));
  AppendMenu(wndmenu, MF_STRING, IDM_SEND_CAD, _T("Send Ctrl-Alt-&Del"));
  AppendMenu(wndmenu, MF_STRING, IDM_SEND_CTLESC, _T("Send Ctrl-&Esc"));
  AppendMenu(wndmenu, MF_STRING, IDM_REQUEST_REFRESH, _T("Refres&h Screen"));
  AppendMenu(wndmenu, MF_SEPARATOR, 0, 0);
  AppendMenu(wndmenu, MF_STRING, IDM_NEWCONN, _T("Ne&w Connection..."));
  AppendMenu(wndmenu, MF_STRING, IDM_OPTIONS, _T("&Options..."));
  AppendMenu(wndmenu, MF_STRING, IDM_INFO, _T("Connection &Info..."));
  AppendMenu(wndmenu, MF_STRING, IDM_ABOUT, _T("&About..."));
}

void
CConn::serverCutText(const char* str, rdr::U32 len) {
  if (!options.serverCutText) return;
  window->serverCutText(str, len);
}


void CConn::beginRect(const Rect& r, unsigned int encoding) {
  sock->inStream().startTiming();
}

void CConn::endRect(const Rect& r, unsigned int encoding) {
  sock->inStream().stopTiming();
  lastUsedEncoding_ = encoding;
  if (debugDelay != 0) {
    window->invertRect(r);
    debugRects.push_back(r);
  }
}

void CConn::fillRect(const Rect& r, Pixel pix) {
  window->fillRect(r, pix);
}
void CConn::imageRect(const Rect& r, void* pixels) {
  window->imageRect(r, pixels);
}
void CConn::copyRect(const Rect& r, int srcX, int srcY) {
  window->copyRect(r, srcX, srcY);
}

void CConn::getUserPasswd(char** user, char** password) {
  if (!user && options.passwordFile.buf[0]) {
    FILE* fp = fopen(options.passwordFile.buf, "rb");
    if (fp) {
      char data[256];
      int datalen = fread(data, 1, 256, fp);
      fclose(fp);
      if (datalen == 8) {
	ObfuscatedPasswd obfPwd;
	obfPwd.buf = data;
	obfPwd.length  = datalen; 
	PlainPasswd passwd(obfPwd);
	obfPwd.takeBuf();
	*password = strDup(passwd.buf);
	memset(data, 0, sizeof(data));
      }
    }
  }
  if (user && options.userName.buf)
    *user = strDup(options.userName.buf);
  if (password && options.password.buf)
    *password = strDup(options.password.buf);
  if ((user && !*user) || (password && !*password)) {
    // Missing username or password - prompt the user
    UserPasswdDialog userPasswdDialog;
    userPasswdDialog.setCSecurity(getCurrentCSecurity());
    userPasswdDialog.getUserPasswd(user, password);
  }
  if (user) options.setUserName(*user);
  if (password) options.setPassword(*password);
}

bool CConn::processFTMsg(int type) {
  return m_fileTransfer.processFTMsg(type);
}
