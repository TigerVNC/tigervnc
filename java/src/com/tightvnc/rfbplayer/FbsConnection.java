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

    // Construct URLs from strings.
    URL base = null;
    if (applet != null) {
      base = applet.getCodeBase();
    }
    fbsURL = new URL(base, fbsLocation);
    fbiURL = fbkURL = null;
    if (indexLocationPrefix != null) {
      try {
        fbiURL = new URL(base, indexLocationPrefix + ".fbi");
        fbkURL = new URL(base, indexLocationPrefix + ".fbk");
      } catch (MalformedURLException e) {
        fbiURL = fbkURL = null;
      }
    }

    // Try to load the .fbi index file.
    loadIndex();
  }

  FbsInputStream connect(long timeOffset) throws IOException {
    URLConnection connection = fbsURL.openConnection();
    FbsInputStream fbs = new FbsInputStream(connection.getInputStream());
    fbs.setTimeOffset(timeOffset);

    return fbs;
  }

  private void loadIndex() {
    // Loading .fbi makes sense only if both .fbi and .fbk files are available.
    if (fbiURL != null && fbkURL != null) {
      try {
        // Connect.
        URLConnection connection = fbiURL.openConnection();
        connection.connect();
        DataInputStream is = new DataInputStream(connection.getInputStream());

        // Check file signature.
        byte[] b = new byte[12];
        is.readFully(b);
        if (b[0] != 'F' || b[1] != 'B' || b[2] != 'I' || b[3] != ' ' ||
            b[4] != '0' || b[5] != '0' || b[6] != '1' || b[7] != '.' ||
            b[8] < '0' || b[8] > '9' || b[9] < '0' || b[9] > '9' ||
            b[10] < '0' || b[10] > '9' || b[11] != '\n') {
          System.err.println("Warning: bad .fbi file data, not using index");
          fbiURL = null;
        }

        // Load index from the .fbi file.
        // FIXME: Real loading is not implemented yet.
        int numRecords = 0;
        try {
          while (true) {
            is.readInt();
            is.readInt();
            is.readInt();
            is.readInt();
            is.readInt();
            numRecords++;
          }
        } catch (EOFException e) {
        } catch (IOException e) {
          System.err.println("Warning: Index data may be incomplete");
        }
        System.err.println("Loaded index data, " + numRecords + " records");
      } catch (IOException e) {
        System.err.println("Warning: I/O exception while loading index: " + e);
        System.err.println("Warning: failed to load .fbi, not using index");
      }
    }
  }

}
