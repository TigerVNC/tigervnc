/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include <map>

#include <rfb/Rect.h>

#include <FL/Fl_Widget.H>

#include "EmulateMB.h"

class Fl_Menu_Button;
class Fl_RGB_Image;

class CConn;
class PlatformPixelBuffer;
class Surface;

class Viewport : public Fl_Widget, public EmulateMB {
public:

  Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_);
  ~Viewport();

  // Most efficient format (from Viewport's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // New image for the locally rendered cursor
  void setCursor(int width, int height, const rfb::Point& hotspot,
                 const uint8_t* data);

  // Change client LED state
  void setLEDState(unsigned int state);

  void draw(Surface* dst);

  // Clipboard events
  void handleClipboardRequest();
  void handleClipboardAnnounce(bool available);
  void handleClipboardData(const char* data);

  // Fl_Widget callback methods

  void draw() override;

  void resize(int x, int y, int w, int h) override;

  int handle(int event) override;

protected:
  void sendPointerEvent(const rfb::Point& pos, uint8_t buttonMask) override;

private:
  bool hasFocus();

  unsigned int getModifierMask(unsigned int keysym);

  static void handleClipboardChange(int source, void *data);

  void flushPendingClipboard();

  void handlePointerEvent(const rfb::Point& pos, uint8_t buttonMask);
  static void handlePointerTimeout(void *data);

  void resetKeyboard();

  void handleKeyPress(int keyCode, uint32_t keySym);
  void handleKeyRelease(int keyCode);

  static int handleSystemEvent(void *event, void *data);

#ifdef WIN32
  static void handleAltGrTimeout(void *data);
  void resolveAltGrDetection(bool isAltGrSequence);
#endif

  void pushLEDState();

  void initContextMenu();
  void popupContextMenu();

  void setMenuKey();

  static void handleOptions(void *data);

private:
  CConn* cc;

  PlatformPixelBuffer* frameBuffer;

  rfb::Point lastPointerPos;
  uint8_t lastButtonMask;

  typedef std::map<int, uint32_t> DownMap;
  DownMap downKeySym;

#ifdef WIN32
  bool altGrArmed;
  unsigned int altGrCtrlTime;
#endif

  bool firstLEDState;

  bool pendingClientClipboard;

  int clipboardSource;

  uint32_t menuKeySym;
  int menuKeyCode, menuKeyFLTK;
  Fl_Menu_Button *contextMenu;

  bool menuCtrlKey;
  bool menuAltKey;

  Fl_RGB_Image *cursor;
  rfb::Point cursorHotspot;
};

#endif
