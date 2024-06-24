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

class CConn : public rfb::CConnection
{
public:
  CConn(const char* vncServerName, network::Socket* sock);
  ~CConn();

  const char *connectionInfo();

  unsigned getUpdateCount();
  unsigned getPixelCount();
  unsigned getPosition();

  // Callback when socket is ready (or broken)
  static void socketEvent(FL_SOCKET fd, void *data);

  // CConnection callback methods
  void initDone() override;

  void setDesktopSize(int w, int h) override;
  void setExtendedDesktopSize(unsigned reason, unsigned result,
                              int w, int h,
                              const rfb::ScreenSet& layout) override;

  void setName(const char* name) override;

  void setColourMapEntries(int firstColour, int nColours,
                           uint16_t* rgbs) override;

  void bell() override;

  void framebufferUpdateStart() override;
  void framebufferUpdateEnd() override;
  bool dataRect(const rfb::Rect& r, int encoding) override;

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 const uint8_t* data) override;
  void setCursorPos(const rfb::Point& pos) override;

  void fence(uint32_t flags, unsigned len,
             const uint8_t data[]) override;

  void setLEDState(unsigned int state) override;

  void handleClipboardRequest() override;
  void handleClipboardAnnounce(bool available) override;
  void handleClipboardData(const char* data) override;

private:

  void resizeFramebuffer() override;

  void autoSelectFormatAndEncoding();
  void updatePixelFormat();

  static void handleOptions(void *data);

  static void handleUpdateTimeout(void *data);

private:
  std::string serverHost;
  int serverPort;
  network::Socket* sock;

  DesktopWindow *desktop;

  unsigned updateCount;
  unsigned pixelCount;

  rfb::PixelFormat serverPF;
  rfb::PixelFormat fullColourPF;

  int lastServerEncoding;

  struct timeval updateStartTime;
  size_t updateStartPos;
  unsigned long long bpsEstimate;
};

#endif
