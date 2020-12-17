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
import java.util.*;

import com.tigervnc.rdr.*;

public class ZRLEDecoder extends Decoder {

  private static ByteBuffer readOpaque24A(InStream is)
  {
    is.check(3);
    ByteBuffer r = ByteBuffer.allocate(4);
    r.put(0, (byte)is.readU8());
    r.put(1, (byte)is.readU8());
    r.put(2, (byte)is.readU8());
    return r;
  }

  private static ByteBuffer readOpaque24B(InStream is)
  {
    is.check(3);
    ByteBuffer r = ByteBuffer.allocate(4);
    r.put(1, (byte)is.readU8());
    r.put(2, (byte)is.readU8());
    r.put(3, (byte)is.readU8());
    return r;
  }

  public ZRLEDecoder() {
    super(DecoderFlags.DecoderOrdered);
    zis = new ZlibInStream();
  }

  public void readRect(Rect r, InStream is,
                      ServerParams server, OutStream os)
  {
    int len;

    len = is.readU32();
    os.writeU32(len);
    os.copyBytes(is, len);
  }

  public void decodeRect(Rect r, Object buffer,
                         int buflen, ServerParams server,
                         ModifiablePixelBuffer pb)
  {
    MemInStream is = new MemInStream((byte[])buffer, 0, buflen);
    PixelFormat pf = server.pf();
    switch (pf.bpp) {
    case 8:  zrleDecode8(r, is, zis, pf, pb); break;
    case 16: zrleDecode16(r, is, zis, pf, pb); break;
    case 32:
      {
        if (pf.depth <= 24) {
          int maxPixel = pf.pixelFromRGB(-1, -1, -1, pf.getColorModel());
          boolean fitsInLS3Bytes = maxPixel < (1<<24);
          boolean fitsInMS3Bytes = (maxPixel & 0xff) == 0;

          if ((fitsInLS3Bytes && pf.isLittleEndian()) ||
              (fitsInMS3Bytes && pf.isBigEndian()))
          {
            zrleDecode24A(r, is, zis, pf, pb);
            break;
          }

          if ((fitsInLS3Bytes && pf.isBigEndian()) ||
              (fitsInMS3Bytes && pf.isLittleEndian()))
          {
            zrleDecode24B(r, is, zis, pf, pb);
            break;
          }
        }

        zrleDecode32(r, is, zis, pf, pb);
        break;
      }
    }
  }

  private static enum PIXEL_T { U8, U16, U24A, U24B, U32 };

  private static ByteBuffer READ_PIXEL(InStream is, PIXEL_T type) {
    switch (type) {
    case U8:
      return ByteBuffer.allocate(1).put(0, (byte)is.readOpaque8());
    case U16:
      return ByteBuffer.allocate(2).putShort(0, (short)is.readOpaque16());
    case U24A:
      return readOpaque24A(is);
    case U24B:
      return readOpaque24B(is);
    default:
      return ByteBuffer.allocate(4).putInt(0, is.readOpaque32());
    }
  }

  private void zrleDecode8(Rect r, InStream is,
                           ZlibInStream zis,
                           PixelFormat pf, ModifiablePixelBuffer pb)
  {
    ZRLE_DECODE(r, is, zis, pf, pb, PIXEL_T.U8);
  }

  private void zrleDecode16(Rect r, InStream is,
                            ZlibInStream zis,
                            PixelFormat pf, ModifiablePixelBuffer pb)
  {
    ZRLE_DECODE(r, is, zis, pf, pb, PIXEL_T.U16);
  }

  private void zrleDecode24A(Rect r, InStream is,
                             ZlibInStream zis,
                             PixelFormat pf, ModifiablePixelBuffer pb)
  {
    ZRLE_DECODE(r, is, zis, pf, pb, PIXEL_T.U24A);
  }

  private void zrleDecode24B(Rect r, InStream is,
                             ZlibInStream zis,
                             PixelFormat pf, ModifiablePixelBuffer pb)
  {
    ZRLE_DECODE(r, is, zis, pf, pb, PIXEL_T.U24B);
  }

  private void zrleDecode32(Rect r, InStream is,
                            ZlibInStream zis,
                            PixelFormat pf, ModifiablePixelBuffer pb)
  {
    ZRLE_DECODE(r, is, zis, pf, pb, PIXEL_T.U32);
  }

  private void ZRLE_DECODE(Rect r, InStream is,
                           ZlibInStream zis,
                           PixelFormat pf, ModifiablePixelBuffer pb,
                           PIXEL_T pix_t)
  {
    int length = is.readU32();
    zis.setUnderlying(is, length);
    Rect t = new Rect();
    ByteBuffer buf = ByteBuffer.allocate(64 * 64 * pf.bpp/8);

    for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 64) {

      t.br.y = Math.min(r.br.y, t.tl.y + 64);

      for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 64) {

        t.br.x = Math.min(r.br.x, t.tl.x + 64);

        int mode = zis.readU8();
        boolean rle = (mode & 128) != 0;
        int palSize = mode & 127;
        ByteBuffer palette = ByteBuffer.allocate(128 * pf.bpp/8);

        for (int i = 0; i < palSize; i++) {
          palette.put(READ_PIXEL(zis, pix_t));
        }
        palette.flip();

        if (palSize == 1) {
          byte[] pix = new byte[pf.bpp/8];
          palette.get(pix);
          pb.fillRect(pf, t, pix);
          continue;
        }

        if (!rle) {
          if (palSize == 0) {

            // raw
            switch (pix_t) {
            case U24A:
            case U24B:
              ByteBuffer ptr = buf.duplicate();
              for (int i=0; i < t.area(); i++) {
                ptr.put(READ_PIXEL(zis, pix_t));
              }
              break;
            default:
              zis.readBytes(buf.duplicate(), t.area() * (pf.bpp/8));
            }

          } else {

            // packed pixels
            int bppp = ((palSize > 16) ? 8 :
                        ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

            ByteBuffer ptr = buf.duplicate();

            for (int i = 0; i < t.height(); i++) {
              int eol = ptr.position() + t.width()*pf.bpp/8;
              int b = 0;
              int nbits = 0;

              while (ptr.position() < eol) {
                if (nbits == 0) {
                  b = zis.readU8();
                  nbits = 8;
                }
                nbits -= bppp;
                int index = (b >> nbits) & ((1 << bppp) - 1) & 127;
                ptr.put(palette.array(), index*pf.bpp/8, pf.bpp/8);
              }
            }
          }

        } else {

          if (palSize == 0) {

            // plain RLE

            ByteBuffer ptr = buf.duplicate();
            int end = ptr.position() + t.area()*pf.bpp/8;
            while (ptr.position() < end) {
              ByteBuffer pix = READ_PIXEL(zis, pix_t);
              int len = 1;
              int b;
              do {
                b = zis.readU8();
                len += b;
              } while (b == 255);

              if (end - ptr.position() < len*pf.bpp/8) {
                System.err.println("ZRLE decode error\n");
                throw new Exception("ZRLE decode error");
              }

              while (len-- > 0) ptr.put(pix.array());

            }
          } else {

            // palette RLE

            ByteBuffer ptr = buf.duplicate();
            int end = ptr.position() + t.area()*pf.bpp/8;
            while (ptr.position() < end) {
              int index = zis.readU8();
              int len = 1;
              if ((index & 128) != 0) {
                int b;
                do {
                  b = zis.readU8();
                  len += b;
                } while (b == 255);

                if (end - ptr.position() < len*pf.bpp/8) {
                  System.err.println("ZRLE decode error\n");
                  throw new Exception("ZRLE decode error");
                }
              }

              index &= 127;

              ByteBuffer pix = ByteBuffer.allocate(pf.bpp/8);
              pix.put(palette.array(), index*pf.bpp/8, pf.bpp/8);

              while (len-- > 0) ptr.put(pix.array());
            }
          }
        }

        pb.imageRect(pf, t, buf.array());
      }
    }

    zis.flushUnderlying();
    zis.setUnderlying(null, 0);
  }

  private ZlibInStream zis;
}
