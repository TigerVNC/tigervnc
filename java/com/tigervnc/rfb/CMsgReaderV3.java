/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;

public class CMsgReaderV3 extends CMsgReader {

  public CMsgReaderV3(CMsgHandler handler_, InStream is_) 
  {
    super(handler_, is_);
    nUpdateRectsLeft = 0;
  }

  public void readServerInit() 
  {
    int width = is.readU16();
    int height = is.readU16();
    handler.setDesktopSize(width, height);
    PixelFormat pf = new PixelFormat();
    pf.read(is);
    handler.setPixelFormat(pf);
    String name = is.readString();
    handler.setName(name);
    handler.serverInit();
  }

  public void readMsg() 
  {
    if (nUpdateRectsLeft == 0) {

      int type = is.readU8();
      switch (type) {
      case MsgTypes.msgTypeFramebufferUpdate:   readFramebufferUpdate(); break;
      case MsgTypes.msgTypeSetColourMapEntries: readSetColourMapEntries(); break;
      case MsgTypes.msgTypeBell:                readBell(); break;
      case MsgTypes.msgTypeServerCutText:       readServerCutText(); break;
      case MsgTypes.msgTypeServerFence:         readFence(); break;
      case MsgTypes.msgTypeEndOfContinuousUpdates:  readEndOfContinuousUpdates(); break;
      default:
        vlog.error("unknown message type "+type);
        throw new Exception("unknown message type");
      }

    } else {

      int x = is.readU16();
      int y = is.readU16();
      int w = is.readU16();
      int h = is.readU16();
      int encoding = is.readS32();

      switch (encoding) {
      case Encodings.pseudoEncodingDesktopSize:
        handler.setDesktopSize(w, h);
        break;
      case Encodings.pseudoEncodingExtendedDesktopSize:
        readExtendedDesktopSize(x, y, w, h);
        break;
      case Encodings.pseudoEncodingDesktopName:
        readSetDesktopName(x, y, w, h);
        break;
      case Encodings.pseudoEncodingCursor:
        readSetCursor(w, h, new Point(x,y));
        break;
      case Encodings.pseudoEncodingLastRect:
        nUpdateRectsLeft = 1;     // this rectangle is the last one
        break;
      case Encodings.pseudoEncodingClientRedirect:
        readClientRedirect(x, y, w, h);
        break;
      default:
        readRect(new Rect(x, y, x+w, y+h), encoding);
        break;
      }

      nUpdateRectsLeft--;
      if (nUpdateRectsLeft == 0) handler.framebufferUpdateEnd();
    }
  }

  void readFramebufferUpdate() 
  {
    is.skip(1);
    nUpdateRectsLeft = is.readU16();
    handler.framebufferUpdateStart();
  }

  void readSetDesktopName(int x, int y, int w, int h)
  {
    String name = is.readString();
  
    if (x != 0 || y != 0 || w != 0 || h != 0) {
      vlog.error("Ignoring DesktopName rect with non-zero position/size");
    } else {
      handler.setName(name);
    }
  
  }
  
  void readExtendedDesktopSize(int x, int y, int w, int h)
  {
    int screens, i;
    int id, flags;
    int sx, sy, sw, sh;
    ScreenSet layout = new ScreenSet();
  
    screens = is.readU8();
    is.skip(3);
  
    for (i = 0;i < screens;i++) {
      id = is.readU32();
      sx = is.readU16();
      sy = is.readU16();
      sw = is.readU16();
      sh = is.readU16();
      flags = is.readU32();
  
      layout.add_screen(new Screen(id, sx, sy, sw, sh, flags));
    }
  
    handler.setExtendedDesktopSize(x, y, w, h, layout);
  }

  void readFence()
  {
    int flags;
    int len;
    byte[] data = new byte[64];
  
    is.skip(3);
  
    flags = is.readU32();
  
    len = is.readU8();
    if (len > data.length) {
      System.out.println("Ignoring fence with too large payload\n");
      is.skip(len);
      return;
    }
  
    is.readBytes(data, 0, len);
    
    handler.fence(flags, len, data);
  }
  
  void readEndOfContinuousUpdates()
  {
    handler.endOfContinuousUpdates();
  }

  void readClientRedirect(int x, int y, int w, int h) 
  {
    int port = is.readU16();
    String host = is.readString();
    String x509subject = is.readString();

    if (x != 0 || y != 0 || w != 0 || h != 0) {
      vlog.error("Ignoring ClientRedirect rect with non-zero position/size");
    } else {
      handler.clientRedirect(port, host, x509subject);
    }
  }

  int nUpdateRectsLeft;

  static LogWriter vlog = new LogWriter("CMsgReaderV3");
}
