/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2008 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
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

#ifndef __XDESKTOP_H__
#define __XDESKTOP_H__

#include <rfb/SDesktop.h>
#include <tx/TXWindow.h>
#include <unixcommon.h>

#include <X11/XKBlib.h>
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#include <vncconfig/QueryConnectDialog.h>

class Geometry;
class XPixelBuffer;

struct AddedKeySym
{
  KeySym keysym;
  KeyCode keycode;
};

// number of XKb indicator leds to handle
#define XDESKTOP_N_LEDS 3

class XDesktop : public rfb::SDesktop,
                 public TXGlobalEventHandler,
                 public QueryResultCallback
{
public:
  XDesktop(Display* dpy_, Geometry *geometry);
  virtual ~XDesktop();
  void poll();
  // -=- SDesktop interface
  void init(rfb::VNCServer* vs) override;
  void start() override;
  void stop() override;
  void terminate() override;
  bool isRunning();
  void queryConnection(network::Socket* sock,
                       const char* userName) override;
  void pointerEvent(const rfb::Point& pos, uint8_t buttonMask) override;
  void keyEvent(uint32_t keysym, uint32_t xtcode, bool down) override;
  unsigned int setScreenLayout(int fb_width, int fb_height,
                               const rfb::ScreenSet& layout) override;

  // -=- TXGlobalEventHandler interface
  bool handleGlobalEvent(XEvent* ev) override;

  // -=- QueryResultCallback interface
  void queryApproved() override;
  void queryRejected() override;

protected:
  Display* dpy;
  Geometry* geometry;
  XPixelBuffer* pb;
  rfb::VNCServer* server;
  QueryConnectDialog* queryConnectDialog;
  network::Socket* queryConnectSock;
  uint8_t oldButtonMask;
  bool haveXtest;
  bool haveDamage;
  int maxButtons;
  std::list<AddedKeySym> addedKeysyms;
  std::map<KeySym, KeyCode> pressedKeys;
  bool running;
#ifdef HAVE_XDAMAGE
  Damage damage;
  int xdamageEventBase;
#endif
  int xkbEventBase;
#ifdef HAVE_XFIXES
  int xfixesEventBase;
#endif
#ifdef HAVE_XRANDR
  int xrandrEventBase;
  OutputIdMap outputIdMap;
  unsigned long randrSyncSerial;
#endif
  int ledMasks[XDESKTOP_N_LEDS];
  unsigned ledState;
  const unsigned short *codeMap;
  unsigned codeMapLen;

protected:
#ifdef HAVE_XTEST
  KeyCode XkbKeysymToKeycode(KeySym keysym);
  KeyCode getReusableKeycode(XkbDescPtr xkb);
  KeyCode addKeysym(KeySym keysym);
  void deleteAddedKeysyms();
  KeyCode keysymToKeycode(KeySym keysym);
#endif
#ifdef HAVE_XFIXES
  bool setCursor();
#endif
  rfb::ScreenSet computeScreenLayout();
};

#endif // __XDESKTOP_H__
