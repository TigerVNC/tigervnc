/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2019 Brian P. Hinz
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

import java.awt.image.*;
import java.nio.*;
import java.util.Arrays;

import com.tigervnc.rdr.*;

public class HextileDecoder extends Decoder {

  public static final int hextileRaw = (1 << 0);
  public static final int hextileBgSpecified = (1 << 1);
  public static final int hextileFgSpecified = (1 << 2);
  public static final int hextileAnySubrects = (1 << 3);
  public static final int hextileSubrectsColoured = (1 << 4);

  public HextileDecoder() { super(DecoderFlags.DecoderPlain); }

  public void readRect(Rect r, InStream is,
                       ServerParams server, OutStream os)
  {
    Rect t = new Rect();
    int bytesPerPixel;

    bytesPerPixel = server.pf().bpp/8;

    for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

      t.br.y = Math.min(r.br.y, t.tl.y + 16);

      for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {
        int tileType;

        t.br.x = Math.min(r.br.x, t.tl.x + 16);

        tileType = is.readU8();
        os.writeU8(tileType);

        if ((tileType & hextileRaw) != 0) {
          os.copyBytes(is, t.area() * bytesPerPixel);
          continue;
        }

        if ((tileType & hextileBgSpecified) != 0)
          os.copyBytes(is, bytesPerPixel);

        if ((tileType & hextileFgSpecified) != 0)
          os.copyBytes(is, bytesPerPixel);

        if ((tileType & hextileAnySubrects) != 0) {
          int nSubrects;

          nSubrects = is.readU8();
          os.writeU8(nSubrects);

          if ((tileType & hextileSubrectsColoured) != 0)
            os.copyBytes(is, nSubrects * (bytesPerPixel + 2));
          else
            os.copyBytes(is, nSubrects * 2);
        }
      }
    }
  }

  public void decodeRect(Rect r, Object buffer,
                         int buflen, ServerParams server,
                         ModifiablePixelBuffer pb)
  {
    MemInStream is = new MemInStream((byte[])buffer, 0, buflen);
    PixelFormat pf = server.pf();
    switch (pf.bpp) {
    case 8:  hextileDecode8(r, is, pf, pb); break;
    case 16: hextileDecode16(r, is, pf, pb); break;
    case 32: hextileDecode32(r, is, pf, pb); break;
    }
  }

  private void hextileDecode8(Rect r, InStream is,
                              PixelFormat pf,
                              ModifiablePixelBuffer pb)
  {
    HEXTILE_DECODE(r, is, pf, pb);
  }

  private void hextileDecode16(Rect r, InStream is,
                               PixelFormat pf,
                               ModifiablePixelBuffer pb)
  {
    HEXTILE_DECODE(r, is, pf, pb);
  }

  private void hextileDecode32(Rect r, InStream is,
                               PixelFormat pf,
                               ModifiablePixelBuffer pb)
  {
    HEXTILE_DECODE(r, is, pf, pb);
  }

  private static ByteBuffer READ_PIXEL(InStream is, PixelFormat pf) {
    switch (pf.bpp) {
    case 8:
      return ByteBuffer.allocate(1).put(0, (byte)is.readOpaque8());
    case 16:
      return ByteBuffer.allocate(2).putShort(0, (short)is.readOpaque16());
    default:
      return ByteBuffer.allocate(4).putInt(0, is.readOpaque32());
    }
  }

  private void HEXTILE_DECODE(Rect r, InStream is,
                              PixelFormat pf,
                              ModifiablePixelBuffer pb)
  {
    Rect t = new Rect();
    ByteBuffer bg = ByteBuffer.allocate(pf.bpp/8);
    ByteBuffer fg = ByteBuffer.allocate(pf.bpp/8);
    ByteBuffer buf = ByteBuffer.allocate(16 * 16 * pf.bpp/8);

    for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 16) {

      t.br.y = Math.min(r.br.y, t.tl.y + 16);

      for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 16) {

        t.br.x = Math.min(r.br.x, t.tl.x + 16);

        int tileType = is.readU8();

        if ((tileType & hextileRaw) != 0) {
          is.readBytes(buf.duplicate(), t.area() * (pf.bpp/8));
          pb.imageRect(pf, t, buf.array());
          continue;
        }

        if ((tileType & hextileBgSpecified) != 0)
          bg = READ_PIXEL(is, pf);

        int len = t.area();
        ByteBuffer ptr = buf.duplicate();
        while (len-- > 0) ptr.put(bg.array());

        if ((tileType & hextileFgSpecified) != 0)
          fg = READ_PIXEL(is, pf);

        if ((tileType & hextileAnySubrects) != 0) {
          int nSubrects = is.readU8();

          for (int i = 0; i < nSubrects; i++) {

            if ((tileType & hextileSubrectsColoured) != 0)
              fg = READ_PIXEL(is, pf);

            int xy = is.readU8();
            int wh = is.readU8();

            int x = ((xy >> 4) & 15);
            int y = (xy & 15);
            int w = ((wh >> 4) & 15) + 1;
            int h = (wh & 15) + 1;
            if (x + w > 16 || y + h > 16) {
              throw new Exception("HEXTILE_DECODE: Hextile out of bounds");
            }
            ptr = buf.duplicate();
            ptr.position((y * t.width() + x)*pf.bpp/8);
            int rowAdd = (t.width() - w)*pf.bpp/8;
            while (h-- > 0) {
              len = w;
              while (len-- > 0) ptr.put(fg.array());
              if (h > 0) ptr.position(ptr.position()+rowAdd);
            }
          }
        }
        pb.imageRect(pf, t, buf.array());
      }
    }
  }
}
