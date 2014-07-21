/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

namespace rfb { class ModifiablePixelBuffer; }

#include <FL/Fl_Widget.H>

class Fl_Menu_Button;
class Fl_RGB_Image;

namespace rfb { class PixelTransformer; }

class CConn;
class PlatformPixelBuffer;

class Viewport : public Fl_Widget {
public:

  Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_);
  ~Viewport();

  // Most efficient format (from Viewport's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // Return a pointer to the framebuffer for decoders to write into
  rfb::ModifiablePixelBuffer* getFramebuffer(void);

  // New image for the locally rendered cursor
  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // Fl_Widget callback methods

  void draw();

  void resize(int x, int y, int w, int h);

  int handle(int event);

private:

  PlatformPixelBuffer* createFramebuffer(int w, int h);

  static void handleClipboardChange(int source, void *data);

  void handlePointerEvent(const rfb::Point& pos, int buttonMask);
  static void handlePointerTimeout(void *data);

  void handleKeyPress(int keyCode, rdr::U32 keySym);
  void handleKeyRelease(int keyCode);

  rdr::U32 translateKeyEvent(void);
  void handleFLTKKeyPress(void);

  void initContextMenu();
  void popupContextMenu();

  void setMenuKey();

  static void handleOptions(void *data);

private:
  CConn* cc;

  PlatformPixelBuffer* frameBuffer;

  rfb::Point lastPointerPos;
  int lastButtonMask;

  typedef std::map<int, rdr::U32> DownMap;
  DownMap downKeySym;

  rdr::U32 menuKeySym;
  int menuKeyCode;
  Fl_Menu_Button *contextMenu;

  bool menuCtrlKey;
  bool menuAltKey;

  Fl_RGB_Image *cursor;
  rfb::Point cursorHotspot;
};

#endif
