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

#include <FL/Fl_Widget.H>

#include <rfb/Region.h>
#include <rfb/Pixel.h>
#include <rfb/ColourMap.h>

class Fl_Menu_Button;
class Fl_RGB_Image;

namespace rfb { class PixelTransformer; }

class CConn;
class PlatformPixelBuffer;

class Viewport : public Fl_Widget {
public:

  Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_);
  ~Viewport();

  // PixelFormat of incoming write operations
  void setServerPF(const rfb::PixelFormat& pf);
  // Most efficient format (from Viewport's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // Methods forwarded from CConn

  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);

  void fillRect(const rfb::Rect& r, rfb::Pixel pix);
  void imageRect(const rfb::Rect& r, void* pixels);
  void copyRect(const rfb::Rect& r, int srcX, int srcY);

  rdr::U8* getBufferRW(const rfb::Rect& r, int* stride);

  void damageRect(const rfb::Rect& r);

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // Fl_Widget callback methods

  void draw();

  void resize(int x, int y, int w, int h);

  int handle(int event);

private:

  static void handleUpdateTimeout(void *data);

  void commitColourMap();

  static void handleClipboardChange(int source, void *data);

  void handlePointerEvent(const rfb::Point& pos, int buttonMask);
  static void handlePointerTimeout(void *data);

  rdr::U32 translateKeyEvent(int keyCode, int origKeyCode, const char *keyText);
  void handleKeyEvent(int keyCode, int origKeyCode, const char *keyText, bool down);

  void initContextMenu();
  void popupContextMenu();

  void setMenuKey();

  static void handleOptions(void *data);

private:
  CConn* cc;

  PlatformPixelBuffer* frameBuffer;

  rfb::PixelTransformer *pixelTrans;
  rfb::SimpleColourMap colourMap;
  bool colourMapChange;

  rfb::Region damage;

  rfb::Point lastPointerPos;
  int lastButtonMask;

  typedef std::map<int, rdr::U32> DownMap;
  DownMap downKeySym;

  int menuKeyCode;
  Fl_Menu_Button *contextMenu;

  bool menuCtrlKey;
  bool menuAltKey;

  Fl_RGB_Image *cursor;
  rfb::Point cursorHotspot;
};

#endif
