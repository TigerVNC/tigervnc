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

// -=- CView.h

// An instance of the CView class is created for each VNC Viewer connection.

#ifndef __RFB_WIN32_CVIEW_H__
#define __RFB_WIN32_CVIEW_H__

#include <network/Socket.h>

#include <rfb/CConnection.h>
#include <rfb/Cursor.h>
#include <rfb/UserPasswdGetter.h>

#include <rfb_win32/Clipboard.h>
#include <rfb_win32/DIBSectionBuffer.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/CKeyboard.h>
#include <rfb_win32/CPointer.h>

#include <vncviewer/InfoDialog.h>
#include <vncviewer/OptionsDialog.h>
#include <vncviewer/ViewerToolBar.h>
#include <vncviewer/CViewOptions.h>
#include <vncviewer/CViewManager.h>
#include <vncviewer/FileTransfer.h>
#include <list>


namespace rfb {

  namespace win32 {

    class CView : public CConnection,
                  public UserPasswdGetter,
                  rfb::win32::Clipboard::Notifier,
                  rdr::FdInStreamBlockCallback
    {
    public:
      CView();
      virtual ~CView();

      bool initialise(network::Socket* s);

      void setManager(CViewManager* m) {manager = m;}

      void applyOptions(CViewOptions& opt);
      const CViewOptions& getOptions() const {return options;};

      // -=- Window Message handling

      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);
      virtual LRESULT processFrameMessage(UINT msg, WPARAM wParam, LPARAM lParam);

      // -=- Socket blocking handling
      //     blockCallback will throw QuitMessage(result) when
      //     it processes a WM_QUIT message.
      //     The caller may catch that to cope gracefully with
      //     a request to quit.

      class QuitMessage : public rdr::Exception {
      public:
        QuitMessage(WPARAM wp) : rdr::Exception("QuitMessage") {}
        WPARAM wParam;
      };
      virtual void blockCallback();

      // -=- Window interface

      void postQuitOnDestroy(bool qod) {quit_on_destroy = qod;}
      PixelFormat getNativePF() const;
      void setVisible(bool visible);
      void close(const char* reason=0);
      HWND getHandle() const {return hwnd;}
      HWND getFrameHandle() const {return frameHwnd;}

      void notifyClipboardChanged(const char* text, int len);

      // -=- Coordinate conversions

      inline Point bufferToClient(const Point& p) {
        Point pos = p;
        if (client_size.width() > buffer->width())
          pos.x += (client_size.width() - buffer->width()) / 2;
        else if (client_size.width() < buffer->width())
          pos.x -= scrolloffset.x;
        if (client_size.height() > buffer->height())
          pos.y += (client_size.height() - buffer->height()) / 2;
        else if (client_size.height() < buffer->height())
          pos.y -= scrolloffset.y;
        return pos;
      }
      inline Rect bufferToClient(const Rect& r) {
        return Rect(bufferToClient(r.tl), bufferToClient(r.br));
      }

      inline Point clientToBuffer(const Point& p) {
        Point pos = p;
        if (client_size.width() > buffer->width())
          pos.x -= (client_size.width() - buffer->width()) / 2;
        else if (client_size.width() < buffer->width())
          pos.x += scrolloffset.x;
        if (client_size.height() > buffer->height())
          pos.y -= (client_size.height() - buffer->height()) / 2;
        else if (client_size.height() < buffer->height())
          pos.y += scrolloffset.y;
        return pos;
      }
      inline Rect clientToBuffer(const Rect& r) {
        return Rect(clientToBuffer(r.tl), clientToBuffer(r.br));
      }

      void setFullscreen(bool fs);

      bool setViewportOffset(const Point& tl);

      bool processBumpScroll(const Point& cursorPos);
      void setBumpScroll(bool on);

      int lastUsedEncoding() const { return lastUsedEncoding_; }

      // -=- CConnection interface overrides

      virtual CSecurity* getCSecurity(int secType);

      virtual void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);
      virtual void bell();

      virtual void framebufferUpdateEnd();

      virtual void setDesktopSize(int w, int h);
      virtual void setCursor(const Point& hotspot, const Point& size, void* data, void* mask);
      virtual void setName(const char* name);
      virtual void serverInit();

      virtual void serverCutText(const char* str, int len);

      virtual void beginRect(const Rect& r, unsigned int encoding);
      virtual void endRect(const Rect& r, unsigned int encoding);

      virtual void fillRect(const Rect& r, Pixel pix);
      virtual void imageRect(const Rect& r, void* pixels);
      virtual void copyRect(const Rect& r, int srcX, int srcY);

      void invertRect(const Rect& r);

      // VNCviewer dialog objects

      OptionsDialog optionsDialog;

      friend class InfoDialog;
      InfoDialog infoDialog;

      // UserPasswdGetter overrides, used to support a pre-supplied VNC password
      virtual bool getUserPasswd(char** user, char** password);

      // Global user-config registry key
      static RegKey userConfigKey;

      bool processFTMsg(int type);

    protected:

      // Locally-rendered VNC cursor
      void hideLocalCursor();
      void showLocalCursor();
      void renderLocalCursor();

      // The system-rendered cursor
      void hideSystemCursor();
      void showSystemCursor();

      // Grab AltTab?
      void setAltTabGrab(bool grab);

      // cursorOutsideBuffer() is called whenever we detect that the mouse has
      // moved outside the desktop.  It restores the system arrow cursor.
      void cursorOutsideBuffer();

      // Returns true if part of the supplied rect is visible, false otherwise
      bool invalidateBufferRect(const Rect& crect);

      // Auto-encoding selector
      void autoSelectFormatAndEncoding();

      // Request an update with appropriate setPixelFormat and setEncodings calls
      void requestNewUpdate();

      // Update the window palette if the display is palette-based.
      // Colours are pulled from the DIBSectionBuffer's ColourMap.
      // Only the specified range of indexes is dealt with.
      // After the update, the entire window is redrawn.
      void refreshWindowPalette(int start, int count);

      // Determine whether or not we need to enable/disable scrollbars and set the
      // window style accordingly
      void calculateScrollBars();

      // Recalculate the most suitable full-colour pixel format
      void calculateFullColourPF();

      // Enable/disable/check/uncheck the F8 menu items as appropriate.
      void updateF8Menu(bool hideSystemCommands);

      // VNCviewer options

      CViewOptions options;

      // Input handling
      void writeKeyEvent(rdr::U8 vkey, rdr::U32 flags, bool down);
      void writePointerEvent(int x, int y, int buttonMask);
      rfb::win32::CKeyboard kbd;
      rfb::win32::CPointer ptr;
      Point oldpos;

      // Clipboard handling
      rfb::win32::Clipboard clipboard;

      // Pixel format and encoding
      PixelFormat serverDefaultPF;
      PixelFormat fullColourPF;
      bool sameMachine;
      bool encodingChange;
      bool formatChange;
      int lastUsedEncoding_;

      // Networking and RFB protocol
      network::Socket* sock;
      bool readyToRead;
      bool requestUpdate;

      // Palette handling
      LogicalPalette windowPalette;
      bool palette_changed;

      // - Full-screen mode
      Rect fullScreenOldRect;
      DWORD fullScreenOldFlags;
      bool fullScreenActive;

      // Bump-scrolling (used in full-screen mode)
      bool bumpScroll;
      Point bumpScrollDelta;
      IntervalTimer bumpScrollTimer;

      // Cursor handling
      Cursor cursor;
      bool systemCursorVisible;  // Should system-cursor be drawn?
      bool trackingMouseLeave;
      bool cursorInBuffer;    // Is cursor position within server buffer? (ONLY for LocalCursor)
      bool cursorVisible;     // Is cursor currently rendered?
      bool cursorAvailable;   // Is cursor available for rendering?
      Point cursorPos;
      ManagedPixelBuffer cursorBacking;
      Rect cursorBackingRect;

      // ** Debugging/logging
      /*
      int update_rect_count;
      int update_pixel_count;
      Rect update_extent;
      */
      std::list<Rect> debugRects;

      // ToolBar handling
      ViewerToolBar tb;
      bool toolbar;

      // Local window state
      win32::DIBSectionBuffer* buffer;
      bool has_focus;
      bool quit_on_destroy;
      Rect window_size;
      Rect client_size;
      Point scrolloffset;
      Point maxscrolloffset;
      HWND hwnd;
      HWND frameHwnd;

      // Handle back to CViewManager instance, if any
      CViewManager* manager;

      FileTransfer m_fileTransfer;

    };

  };

};

#endif


