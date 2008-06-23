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

  /** Index data loaded from the .fbi file. */
  FbsEntryPoint[] indexData;
  int numIndexRecords;

  /** RFB initialization data loaded from the .fbi file. */
  byte[] rfbInitData;

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
    indexData = null;
    numIndexRecords = 0;
    rfbInitData = null;
    loadIndex();
  }

  FbsInputStream connect(long timeOffset) throws IOException {
    FbsInputStream fbs = null;

    // Try efficient seeking first.
    int i = indexForTimeOffset(timeOffset);
    if (i >= 0) {
      FbsEntryPoint entryPoint = indexData[i];
      if (entryPoint.key_size < entryPoint.fbs_fpos) {
        try {
          fbs = openFbsFile(entryPoint);
        } catch (IOException e) {
          System.err.println(e);
        }
        if (fbs == null) {
          System.err.println("Could not open FBS file at entry point " +
                             entryPoint.timestamp + " ms");
        }
      }
    }

    // Fallback to the dumb version of openFbsFile().
    if (fbs == null) {
      fbs = openFbsFile();
    }

    // Seek to the specified position.
    fbs.setTimeOffset(timeOffset, false);
    return fbs;
  }

  /**
   * Load index data from .fbi file to {@link #indexData}.
   */
  private void loadIndex() {
    // Loading .fbi makes sense only if both .fbi and .fbk files are available.
    if (fbiURL != null && fbkURL != null) {
      FbsEntryPoint[] newIndex;
      int numRecordsRead = 0;
      byte[] newInitData;
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
          System.err.println("Could not load index: bad .fbi file signature");
          return;
        }

        // Read the record counter and allocate index array.
        int numRecords = is.readInt();
        if (numRecords <= 0) {
          System.err.println("Could not load index: bad .fbi record counter");
          return;
        }
        newIndex = new FbsEntryPoint[numRecords];

        // Read byte counter and allocate byte array for RFB initialization.
        int initSize = is.readInt();
        if (initSize <= 0) {
          System.err.println("Could not load index: bad RFB init data size");
          return;
        }
        newInitData = new byte[initSize];

        // Load index from the .fbi file.
        try {
          for (int i = 0; i < numRecords; i++) {
            FbsEntryPoint record = new FbsEntryPoint();
            record.timestamp = (long)is.readInt() & 0xFFFFFFFFL;
            record.key_fpos = (long)is.readInt() & 0xFFFFFFFFL;
            record.key_size = (long)is.readInt() & 0xFFFFFFFFL;
            record.fbs_fpos = (long)is.readInt() & 0xFFFFFFFFL;
            record.fbs_skip = (long)is.readInt() & 0xFFFFFFFFL;
            newIndex[i] = record;
            numRecordsRead++;
          }
        } catch (EOFException e) {
          System.err.println("Preliminary end of .fbi file");
        } catch (IOException e) {
          System.err.println("Ignored exception: " + e);
        }
        if (numRecordsRead == 0) {
          System.err.println("Could not load index: failed to read .fbi data");
          return;
        } else if (numRecordsRead != numRecords) {
          System.err.println("Warning: read not as much .fbi data as expected");
        }
        is.readFully(newInitData);
      } catch (FileNotFoundException e) {
        System.err.println("Could not load index: .fbi file not found: " +
                           e.getMessage());
        return;
      } catch (IOException e) {
        System.err.println(e);
        System.err.println("Could not load index: failed to load .fbi file");
        return;
      }
      // Check correctness of the data read.
      for (int i = 1; i < numRecordsRead; i++) {
        if (newIndex[i].timestamp <= newIndex[i - 1].timestamp) {
          System.err.println("Could not load index: wrong .fbi file contents");
          return;
        }
      }
      // Loaded successfully.
      indexData = newIndex;
      numIndexRecords = numRecordsRead;
      rfbInitData = newInitData;
      System.err.println("Loaded index data, " + numRecordsRead + " records");
    }
  }

  private int indexForTimeOffset(long timeOffset) {
    if (timeOffset > 0 && indexData != null && numIndexRecords > 0) {
      int i = 0;
      while (i < numIndexRecords && indexData[i].timestamp <= timeOffset) {
        i++;
      }
      return i - 1;
    } else {
      return -1;
    }
  }

  /**
   * Open FBS file identified by {@link #fbsURL}. The file is open at its very
   * beginning, no seek is performed.
   *
   * @return a newly created FBS input stream.
   * @throws java.io.IOException if an I/O exception occurs.
   */
  private FbsInputStream openFbsFile() throws IOException {
    return new FbsInputStream(fbsURL.openStream());
  }

  /**
   * Open FBS file identified by {@link #fbsURL}. The stream is
   * positioned at the entry point described by <code>entryPoint</code>.
   *
   * @param entryPoint entry point information.
   *
   * @return a newly created FBS input stream on success, <code>null</code> if
   *   any error occured and the FBS stream is not opened.
   * @throws java.io.IOException if an I/O exception occurs.
   */
  private FbsInputStream openFbsFile(FbsEntryPoint entry)
      throws IOException {

    System.err.println("Entering FBS at " + entry.timestamp + " ms");

    // Make sure the protocol is HTTP.
    if (!fbkURL.getProtocol().equalsIgnoreCase("http") ||
        !fbsURL.getProtocol().equalsIgnoreCase("http")) {
      System.err.println("Indexed access requires HTTP protocol in URLs");
      return null;
    }

    // Seek to the keyframe.
    InputStream is = openHttpByteRange(fbkURL, entry.key_fpos, entry.key_size);
    if (is == null) {
      return null;
    }

    // Load keyframe data from the .fbk file, prepend RFB initialization data.
    DataInputStream data = new DataInputStream(is);
    byte[] keyData = new byte[rfbInitData.length + (int)entry.key_size];
    System.arraycopy(rfbInitData, 0, keyData, 0, rfbInitData.length);
    data.readFully(keyData, rfbInitData.length, (int)entry.key_size);
    data.close();

    // Open the FBS stream.
    is = openHttpByteRange(fbsURL, entry.fbs_fpos, -1);
    if (is == null) {
      return null;
    }
    return new FbsInputStream(is, entry.timestamp, keyData, entry.fbs_skip);
  }

  private static InputStream openHttpByteRange(URL url, long offset, long len)
      throws IOException {
    HttpURLConnection conn = (HttpURLConnection)url.openConnection();
    String rangeSpec = "bytes=" + offset + "-";
    if (len != -1) {
      long lastByteOffset = offset + len - 1;
      rangeSpec += lastByteOffset;
    }
    conn.setRequestProperty("Range", rangeSpec);
    conn.connect();
    InputStream is = conn.getInputStream();
    if (conn.getResponseCode() != HttpURLConnection.HTTP_PARTIAL) {
      System.err.println("HTTP server does not support Range request headers");
      is.close();
      return null;
    }
    return is;
  }

}
