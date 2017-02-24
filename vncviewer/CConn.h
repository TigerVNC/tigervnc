/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
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

#ifndef __CCONN_H__
#define __CCONN_H__

#include <FL/Fl.H>

#include <rfb/CConnection.h>
#include <rdr/FdInStream.h>

namespace network { class Socket; }

class DesktopWindow;

class CConn : public rfb::CConnection,
              public rdr::FdInStreamBlockCallback
{
public:
  CConn(const char* vncServerName, network::Socket* sock);
  ~CConn();

  void refreshFramebuffer();

  const char *connectionInfo();

  unsigned getFrameCount();
  unsigned getPixelCount();
  unsigned getPosition();

  // FdInStreamBlockCallback methods
  void blockCallback();

  // Callback when socket is ready (or broken)
  static void socketEvent(FL_SOCKET fd, void *data);

  // CConnection callback methods
  void serverInit();

  void setDesktopSize(int w, int h);
  void setExtendedDesktopSize(unsigned reason, unsigned result,
                              int w, int h, const rfb::ScreenSet& layout);

  void setName(const char* name);

  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);

  void bell();

  void serverCutText(const char* str, rdr::U32 len);

  void framebufferUpdateStart();
  void framebufferUpdateEnd();
  void dataRect(const rfb::Rect& r, int encoding);

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 const rdr::U8* data);

  void fence(rdr::U32 flags, unsigned len, const char data[]);

private:

  void resizeFramebuffer();

  void autoSelectFormatAndEncoding();
  void checkEncodings();
  void requestNewUpdate();

  static void handleOptions(void *data);

  static void handleUpdateTimeout(void *data);

private:
  char* serverHost;
  int serverPort;
  network::Socket* sock;

  DesktopWindow *desktop;

  unsigned frameCount;
  unsigned pixelCount;

  rfb::PixelFormat serverPF;
  rfb::PixelFormat fullColourPF;

  bool pendingPFChange;
  rfb::PixelFormat pendingPF;

  int currentEncoding, lastServerEncoding;

  bool formatChange;
  bool encodingChange;

  bool firstUpdate;
  bool pendingUpdate;
  bool continuousUpdates;

  bool forceNonincremental;

  bool supportsSyncFence;
};

#endif
