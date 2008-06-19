//
//  Copyright (C) 2008 Wimba, Inc.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// FbsConnection.java
//

package com.tightvnc.rfbplayer;

import java.io.*;
import java.net.*;
import java.applet.Applet;

public class FbsConnection {

  URL fbsURL;
  URL fbiURL;
  URL fbkURL;

  FbsConnection(String fbsLocation, String indexLocationPrefix, Applet applet)
      throws MalformedURLException {

    URL base = null;
    if (applet != null) {
      base = applet.getCodeBase();
    }
    fbsURL = new URL(base, fbsLocation);
    fbiURL = null;
    fbkURL = null;
    if (indexLocationPrefix != null) {
      fbiURL = new URL(base, indexLocationPrefix + ".fbi");
      fbkURL = new URL(base, indexLocationPrefix + ".fbk");
    }
  }

  FbsInputStream connect(long timeOffset) throws IOException {
    URLConnection connection = fbsURL.openConnection();
    FbsInputStream fbs = new FbsInputStream(connection.getInputStream());
    fbs.setTimeOffset(timeOffset);

    return fbs;
  }

}
