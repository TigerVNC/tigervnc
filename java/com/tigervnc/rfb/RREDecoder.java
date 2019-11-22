/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2016-2019 Brian P. Hinz
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

package com.tigervnc.rfb;

import java.nio.*;

import com.tigervnc.rdr.*;

public class RREDecoder extends Decoder {

  public RREDecoder() { super(DecoderFlags.DecoderPlain); }

  public void readRect(Rect r, InStream is,
                       ServerParams server, OutStream os)
  {
    int numRects;

    numRects = is.readU32();
    os.writeU32(numRects);

    os.copyBytes(is, server.pf().bpp/8 + numRects * (server.pf().bpp/8 + 8));
  }

  public void decodeRect(Rect r, Object buffer,
                         int buflen, ServerParams server,
                         ModifiablePixelBuffer pb)
  {
    MemInStream is = new MemInStream((byte[])buffer, 0, buflen);
    PixelFormat pf = server.pf();
    switch (pf.bpp) {
    case 8:  rreDecode8 (r, is, pf, pb); break;
    case 16: rreDecode16(r, is, pf, pb); break;
    case 32: rreDecode32(r, is, pf, pb); break;
    }
  }

  private static ByteBuffer READ_PIXEL(InStream is, PixelFormat pf) {
    ByteBuffer b = ByteBuffer.allocate(4);
    switch (pf.bpp) {
    case 8:
      b.putInt(is.readOpaque8());
      return ByteBuffer.allocate(1).put(b.get(3));
    case 16:
      b.putInt(is.readOpaque16());
      return ByteBuffer.allocate(2).put(b.array(), 2, 2);
    case 32:
    default:
      b.putInt(is.readOpaque32());
      return b;
    }
  }

  private void RRE_DECODE(Rect r, InStream is,
                          PixelFormat pf, ModifiablePixelBuffer pb)
  {
    int nSubrects = is.readU32();
    byte[] bg = READ_PIXEL(is, pf).array();
    pb.fillRect(pf, r, bg);

    for (int i = 0; i < nSubrects; i++) {
      byte[] pix = READ_PIXEL(is, pf).array();
      int x = is.readU16();
      int y = is.readU16();
      int w = is.readU16();
      int h = is.readU16();
      pb.fillRect(pf, new Rect(r.tl.x+x, r.tl.y+y, r.tl.x+x+w, r.tl.y+y+h), pix);
    }
  }

  private void rreDecode8(Rect r, InStream is,
                          PixelFormat pf,
                          ModifiablePixelBuffer pb)
  {
    RRE_DECODE(r, is, pf, pb);
  }

  private void rreDecode16(Rect r, InStream is,
                           PixelFormat pf,
                           ModifiablePixelBuffer pb)
  {
    RRE_DECODE(r, is, pf, pb);
  }

  private void rreDecode32(Rect r, InStream is,
                           PixelFormat pf,
                           ModifiablePixelBuffer pb)
  {
    RRE_DECODE(r, is, pf, pb);
  }

}
