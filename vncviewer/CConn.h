/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <rfb/CConnection.h>
#include <network/Socket.h>

#include "DesktopWindow.h"

class CConn : public rfb::CConnection,
              public rdr::FdInStreamBlockCallback
{
public:
  CConn(const char* vncServerName);
  ~CConn();

  void refreshFramebuffer();

  const char *connectionInfo();

  // FdInStreamBlockCallback methods
  void blockCallback();

  // Callback when socket is ready (or broken)
  static void socketEvent(int fd, void *data);

  // CConnection callback methods
  void serverInit();

  void setDesktopSize(int w, int h);
  void setExtendedDesktopSize(int reason, int result, int w, int h,
                              const rfb::ScreenSet& layout);

  void setName(const char* name);

  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);

  void bell();

  void serverCutText(const char* str, rdr::U32 len);

  void framebufferUpdateStart();
  void framebufferUpdateEnd();

  void beginRect(const rfb::Rect& r, int encoding);
  void endRect(const rfb::Rect& r, int encoding);

  void fillRect(const rfb::Rect& r, rfb::Pixel p);
  void imageRect(const rfb::Rect& r, void* p);
  void copyRect(const rfb::Rect& r, int sx, int sy);

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

private:

  void resizeFramebuffer();

  void autoSelectFormatAndEncoding();
  void checkEncodings();
  void requestNewUpdate();

private:
  char* serverHost;
  int serverPort;
  network::Socket* sock;

  DesktopWindow *desktop;

  rfb::PixelFormat serverPF;
  rfb::PixelFormat fullColourPF;

  int currentEncoding, lastServerEncoding;

  bool formatChange;
  bool encodingChange;

  bool firstUpdate;
  bool pendingUpdate;

  bool forceNonincremental;
};

#endif
