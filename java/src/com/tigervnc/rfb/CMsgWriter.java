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

abstract public class CMsgWriter {

  abstract public void writeClientInit(boolean shared);

  public void writeSetPixelFormat(PixelFormat pf) 
  {
    startMsg(MsgTypes.msgTypeSetPixelFormat);                                 
    os.pad(3);
    pf.write(os);
    endMsg();
  }

  public void writeSetEncodings(int nEncodings, int[] encodings) 
  {
    startMsg(MsgTypes.msgTypeSetEncodings);
    os.skip(1);
    os.writeU16(nEncodings);
    for (int i = 0; i < nEncodings; i++)
      os.writeU32(encodings[i]);
    endMsg();
  }

  // Ask for encodings based on which decoders are supported.  Assumes higher
  // encoding numbers are more desirable.

  public void writeSetEncodings(int preferredEncoding, boolean useCopyRect) 
  {
    int nEncodings = 0;
    int[] encodings = new int[Encodings.encodingMax+3];
    if (cp.supportsLocalCursor)
      encodings[nEncodings++] = Encodings.pseudoEncodingCursor;
    if (cp.supportsDesktopResize)
      encodings[nEncodings++] = Encodings.pseudoEncodingDesktopSize;
    if (cp.supportsExtendedDesktopSize)
      encodings[nEncodings++] = Encodings.pseudoEncodingExtendedDesktopSize;
    if (cp.supportsDesktopRename)
      encodings[nEncodings++] = Encodings.pseudoEncodingDesktopName;
    if (Decoder.supported(preferredEncoding)) {
      encodings[nEncodings++] = preferredEncoding;
    }
    if (useCopyRect) {
      encodings[nEncodings++] = Encodings.encodingCopyRect;
    }

    /*
     * Prefer encodings in this order:
     *
     *   Tight, ZRLE, Hextile, *
     */

    if ((preferredEncoding != Encodings.encodingTight) &&
        Decoder.supported(Encodings.encodingTight))
      encodings[nEncodings++] = Encodings.encodingTight;

    if ((preferredEncoding != Encodings.encodingZRLE) &&
        Decoder.supported(Encodings.encodingZRLE))
      encodings[nEncodings++] = Encodings.encodingZRLE;

    if ((preferredEncoding != Encodings.encodingHextile) &&
        Decoder.supported(Encodings.encodingHextile))
      encodings[nEncodings++] = Encodings.encodingHextile;

    // Remaining encodings
    for (int i = Encodings.encodingMax; i >= 0; i--) {
      switch (i) {
      case Encodings.encodingTight:
      case Encodings.encodingZRLE:
      case Encodings.encodingHextile:
        break;
      default:
        if ((i != preferredEncoding) && Decoder.supported(i))
            encodings[nEncodings++] = i;
      }
    }

    encodings[nEncodings++] = Encodings.pseudoEncodingLastRect;
    if (cp.customCompressLevel && cp.compressLevel >= 0 && cp.compressLevel <= 9)
      encodings[nEncodings++] = Encodings.pseudoEncodingCompressLevel0 + cp.compressLevel;
    if (!cp.noJpeg && cp.qualityLevel >= 0 && cp.qualityLevel <= 9)
      encodings[nEncodings++] = Encodings.pseudoEncodingQualityLevel0 + cp.qualityLevel;

    writeSetEncodings(nEncodings, encodings);
  }

  public void writeFramebufferUpdateRequest(Rect r, boolean incremental) 
  {
    startMsg(MsgTypes.msgTypeFramebufferUpdateRequest);
    os.writeU8(incremental?1:0);
    os.writeU16(r.tl.x);
    os.writeU16(r.tl.y);
    os.writeU16(r.width());
    os.writeU16(r.height());
    endMsg();
  }

  public void writeKeyEvent(int key, boolean down) 
  {
    startMsg(MsgTypes.msgTypeKeyEvent);
    os.writeU8(down?1:0);
    os.pad(2);
    os.writeU32(key);
    endMsg();
  }

  public void writePointerEvent(Point pos, int buttonMask) 
  {
    Point p = new Point(pos.x,pos.y);
    if (p.x < 0) p.x = 0;
    if (p.y < 0) p.y = 0;
    if (p.x >= cp.width) p.x = cp.width - 1;
    if (p.y >= cp.height) p.y = cp.height - 1;

    startMsg(MsgTypes.msgTypePointerEvent);
    os.writeU8(buttonMask);
    os.writeU16(p.x);
    os.writeU16(p.y);
    endMsg();
  }

  public void writeClientCutText(String str, int len) 
  {
    startMsg(MsgTypes.msgTypeClientCutText);
    os.pad(3);
    os.writeU32(len);
    os.writeBytes(str.getBytes(), 0, len);
    endMsg();
  }

  abstract public void startMsg(int type);
  abstract public void endMsg();

  public void setOutStream(OutStream os_) { os = os_; }

  ConnParams getConnParams() { return cp; }
  OutStream getOutStream() { return os; }

  protected CMsgWriter(ConnParams cp_, OutStream os_) {cp = cp_; os = os_;}

  ConnParams cp;
  OutStream os;
  static LogWriter vlog = new LogWriter("CMsgWriter");
}
