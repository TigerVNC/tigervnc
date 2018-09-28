/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

//
// CMsgHandler
//

package com.tigervnc.rfb;

abstract public class CMsgHandler {

  public CMsgHandler() {
    cp = new ConnParams();
  }

  public void setDesktopSize(int width, int height)
  {
    cp.width = width;
    cp.height = height;
  }

  public void setExtendedDesktopSize(int reason, int result,
                                     int width, int height,
                                     ScreenSet layout)
  {
    cp.supportsSetDesktopSize = true;

    if ((reason == screenTypes.reasonClient) && (result != screenTypes.resultSuccess))
      return;

    if (!layout.validate(width, height))
      vlog.error("Server sent us an invalid screen layout");

    cp.width = width;
    cp.height = height;
    cp.screenLayout = layout;
  }

  public void setPixelFormat(PixelFormat pf)
  {
    cp.setPF(pf);
  }

  public void setName(String name)
  {
    cp.setName(name);
  }

  public void fence(int flags, int len, byte[] data)
  {
    cp.supportsFence = true;
  }

  public void endOfContinuousUpdates()
  {
    cp.supportsContinuousUpdates = true;
  }

  abstract public void clientRedirect(int port, String host,
                                      String x509subject);

  abstract public void setCursor(int width, int height, Point hotspot,
                                 byte[] data);
  abstract public void serverInit();

  abstract public void readAndDecodeRect(Rect r, int encoding,
                                         ModifiablePixelBuffer pb);

  public void framebufferUpdateStart() {};
  public void framebufferUpdateEnd() {};
  abstract public void dataRect(Rect r, int encoding);

  abstract public void setColourMapEntries(int firstColour, int nColours,
    int[] rgbs);
  abstract public void bell();
  abstract public void serverCutText(String str, int len);

  public ConnParams cp;

  static LogWriter vlog = new LogWriter("CMsgHandler");
}
