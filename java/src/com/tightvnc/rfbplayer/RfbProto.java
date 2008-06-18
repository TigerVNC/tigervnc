//
//  Copyright (C) 2008 Wimba, Inc.  All Rights Reserved.
//  Copyright (C) 2001,2002 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001 Constantin Kaplinsky.  All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
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
// RfbProto.java
//

package com.tightvnc.rfbplayer;

import java.io.*;

class RfbProto {

  final static String versionMsg = "RFB 003.003\n";

  final static int ConnFailed = 0;
  final static int NoAuth = 1;
  final static int VncAuth = 2;

  final static int VncAuthOK = 0;
  final static int VncAuthFailed = 1;
  final static int VncAuthTooMany = 2;

  final static int FramebufferUpdate = 0;
  final static int SetColourMapEntries = 1;
  final static int Bell = 2;
  final static int ServerCutText = 3;

  final static int SetPixelFormat = 0;
  final static int FixColourMapEntries = 1;
  final static int SetEncodings = 2;
  final static int FramebufferUpdateRequest = 3;
  final static int KeyboardEvent = 4;
  final static int PointerEvent = 5;
  final static int ClientCutText = 6;

  final static int EncodingRaw = 0;
  final static int EncodingCopyRect = 1;
  final static int EncodingRRE = 2;
  final static int EncodingCoRRE = 4;
  final static int EncodingHextile = 5;
  final static int EncodingZlib = 6;
  final static int EncodingTight = 7;
  final static int EncodingCompressLevel0 = 0xFFFFFF00;
  final static int EncodingQualityLevel0 = 0xFFFFFFE0;
  final static int EncodingXCursor = 0xFFFFFF10;
  final static int EncodingRichCursor = 0xFFFFFF11;
  final static int EncodingPointerPos = 0xFFFFFF18;
  final static int EncodingLastRect = 0xFFFFFF20;
  final static int EncodingNewFBSize = 0xFFFFFF21;

  final static int MaxNormalEncoding = 7;

  final static int HextileRaw = (1 << 0);
  final static int HextileBackgroundSpecified = (1 << 1);
  final static int HextileForegroundSpecified = (1 << 2);
  final static int HextileAnySubrects = (1 << 3);
  final static int HextileSubrectsColoured = (1 << 4);

  final static int TightExplicitFilter = 0x04;
  final static int TightFill = 0x08;
  final static int TightJpeg = 0x09;
  final static int TightMaxSubencoding = 0x09;
  final static int TightFilterCopy = 0x00;
  final static int TightFilterPalette = 0x01;
  final static int TightFilterGradient = 0x02;

  final static int TightMinToCompress = 12;

  DataInputStream is;

  /**
   * Constructor. It calls <code>{@link #newSession(InputStream)}</code> to open
   * new session.
   *
   * @param is the input stream from which RFB data will be read.
   * @throws java.lang.Exception
   * @throws java.io.IOException
   */
  RfbProto(InputStream is) throws Exception {
    newSession(is);
  }

  /**
   * Open new session that can be read from the specified InputStream.
   *
   * @param is the input stream.
   * @throws java.lang.Exception
   * @throws java.io.IOException
   */
  public void newSession(InputStream is) throws Exception {

    this.is = new DataInputStream(is);

    readVersionMsg();
    if (readAuthScheme() != NoAuth) {
      throw new Exception("Wrong authentication type in the session file");
    }
    readServerInit();
  }

  //
  // Read server's protocol version message.
  //
  int serverMajor, serverMinor;

  void readVersionMsg() throws IOException {

    byte[] b = new byte[12];

    for (int i = 0; i < b.length; i++)
      b[i] = (byte)'0';

    is.readFully(b);

    if ((b[0] != 'R') || (b[1] != 'F') || (b[2] != 'B') || (b[3] != ' ') ||
        (b[4] < '0') || (b[4] > '9') || (b[5] < '0') || (b[5] > '9') || (b[6] <
        '0') || (b[6] > '9') || (b[7] != '.') || (b[8] < '0') || (b[8] > '9') ||
        (b[9] < '0') || (b[9] > '9') || (b[10] < '0') || (b[10] > '9') ||
        (b[11] != '\n')) {
      throw new IOException("Incorrect protocol version");
    }

    serverMajor = (b[4] - '0') * 100 + (b[5] - '0') * 10 + (b[6] - '0');
    serverMinor = (b[8] - '0') * 100 + (b[9] - '0') * 10 + (b[10] - '0');
  }


  //
  // Find out the authentication scheme.
  //
  int readAuthScheme() throws IOException {
    int authScheme = is.readInt();

    switch (authScheme) {

    case ConnFailed:
      int reasonLen = is.readInt();
      byte[] reason = new byte[reasonLen];
      is.readFully(reason);
      throw new IOException(new String(reason));

    case NoAuth:
    case VncAuth:
      return authScheme;

    default:
      throw new IOException("Unknown authentication scheme " + authScheme);

    }
  }


  //
  // Read the server initialisation message
  //
  String desktopName;
  int framebufferWidth, framebufferHeight;
  int bitsPerPixel, depth;
  boolean bigEndian, trueColour;
  int redMax, greenMax, blueMax, redShift, greenShift, blueShift;

  void readServerInit() throws Exception {
    framebufferWidth = is.readUnsignedShort();
    framebufferHeight = is.readUnsignedShort();
    bitsPerPixel = is.readUnsignedByte();
    depth = is.readUnsignedByte();
    bigEndian = (is.readUnsignedByte() != 0);
    trueColour = (is.readUnsignedByte() != 0);
    redMax = is.readUnsignedShort();
    greenMax = is.readUnsignedShort();
    blueMax = is.readUnsignedShort();
    redShift = is.readUnsignedByte();
    greenShift = is.readUnsignedByte();
    blueShift = is.readUnsignedByte();
    byte[] pad = new byte[3];
    is.readFully(pad);
    int nameLength = is.readInt();
    byte[] name = new byte[nameLength];
    is.readFully(name);
    desktopName = new String(name);
  }


  //
  // Set new framebuffer size
  //
  void setFramebufferSize(int width, int height) {
    framebufferWidth = width;
    framebufferHeight = height;
  }


  //
  // Read the server message type
  //
  int readServerMessageType() throws IOException {
    return is.readUnsignedByte();
  }


  //
  // Read a FramebufferUpdate message
  //
  int updateNRects;

  void readFramebufferUpdate() throws IOException {
    is.readByte();
    updateNRects = is.readUnsignedShort();
  }

  // Read a FramebufferUpdate rectangle header
  int updateRectX, updateRectY, updateRectW, updateRectH, updateRectEncoding;

  void readFramebufferUpdateRectHdr() throws IOException {
    updateRectX = is.readUnsignedShort();
    updateRectY = is.readUnsignedShort();
    updateRectW = is.readUnsignedShort();
    updateRectH = is.readUnsignedShort();
    updateRectEncoding = is.readInt();

    if (updateRectEncoding < 0 || updateRectEncoding > MaxNormalEncoding)
      return;

    if ((updateRectX + updateRectW > framebufferWidth) ||
        (updateRectY + updateRectH > framebufferHeight)) {
      throw new IOException("Framebuffer update rectangle too large: " +
                            updateRectW + "x" + updateRectH + " at (" +
                            updateRectX + "," + updateRectY + ")");
    }
  }

  // Read CopyRect source X and Y.
  int copyRectSrcX, copyRectSrcY;

  void readCopyRect() throws IOException {
    copyRectSrcX = is.readUnsignedShort();
    copyRectSrcY = is.readUnsignedShort();
  }


  //
  // Read a ServerCutText message
  //
  String readServerCutText() throws IOException {
    byte[] pad = new byte[3];
    is.readFully(pad);
    int len = is.readInt();
    byte[] text = new byte[len];
    is.readFully(text);
    return new String(text);
  }


  //
  // Read integer in compact representation
  //
  int readCompactLen() throws IOException {
    int portion = is.readUnsignedByte();
    int len = portion & 0x7F;
    if ((portion & 0x80) != 0) {
      portion = is.readUnsignedByte();
      len |= (portion & 0x7F) << 7;
      if ((portion & 0x80) != 0) {
        portion = is.readUnsignedByte();
        len |= (portion & 0xFF) << 14;
      }
    }
    return len;
  }

}
