/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011-2019 Brian P. Hinz
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

import com.tigervnc.rdr.InStream;
import com.tigervnc.rdr.MemInStream;
import com.tigervnc.rdr.OutStream;
import com.tigervnc.rdr.ZlibInStream;
import java.util.ArrayList;
import java.io.InputStream;
import java.awt.image.*;
import java.awt.*;
import java.math.BigInteger;
import java.io.*;
import java.nio.*;
import javax.imageio.*;
import javax.imageio.stream.*;

public class TightDecoder extends Decoder {

  final static int TIGHT_MAX_WIDTH = 2048;
  final static int TIGHT_MIN_TO_COMPRESS = 12;

  // Compression control
  final static int tightExplicitFilter = 0x04;
  final static int tightFill = 0x08;
  final static int tightJpeg = 0x09;
  final static int tightMaxSubencoding = 0x09;

  // Filters to improve compression efficiency
  final static int tightFilterCopy = 0x00;
  final static int tightFilterPalette = 0x01;
  final static int tightFilterGradient = 0x02;

  public TightDecoder() {
    super(DecoderFlags.DecoderPartiallyOrdered);
    zis = new ZlibInStream[4];
    for (int i = 0; i < 4; i++)
      zis[i] = new ZlibInStream();
  }

  public void readRect(Rect r, InStream is,
                       ServerParams server, OutStream os)
  {
    int comp_ctl;

    comp_ctl = is.readU8();
    os.writeU8(comp_ctl);

    comp_ctl >>= 4;

    // "Fill" compression type.
    if (comp_ctl == tightFill) {
      if (server.pf().is888())
        os.copyBytes(is, 3);
      else
        os.copyBytes(is, server.pf().bpp/8);
      return;
    }

    // "JPEG" compression type.
    if (comp_ctl == tightJpeg) {
      int len;

      len = readCompact(is);
      os.writeOpaque32(len);
      os.copyBytes(is, len);
      return;
    }

    // Quit on unsupported compression type.
    if (comp_ctl > tightMaxSubencoding)
      throw new Exception("TightDecoder: bad subencoding value received");

    // "Basic" compression type.

    int palSize = 0;

    if (r.width() > TIGHT_MAX_WIDTH)
      throw new Exception("TightDecoder: too large rectangle ("+r.width()+" pixels)");

    // Possible palette
    if ((comp_ctl & tightExplicitFilter) != 0) {
      int filterId;

      filterId = is.readU8() & 0xff;
      os.writeU8(filterId);

      switch (filterId) {
      case tightFilterPalette:
        palSize = is.readU8() + 1;
        os.writeU32(palSize - 1);

        if (server.pf().is888())
          os.copyBytes(is, palSize * 3);
        else
          os.copyBytes(is, palSize * server.pf().bpp/8);
        break;
      case tightFilterGradient:
        if (server.pf().bpp == 8)
          throw new Exception("TightDecoder: invalid BPP for gradient filter");
        break;
      case tightFilterCopy:
        break;
      default:
        throw new Exception("TightDecoder: unknown filter code received");
      }
    }

    int rowSize, dataSize;

    if (palSize != 0) {
      if (palSize <= 2)
        rowSize = (r.width() + 7) / 8;
      else
        rowSize = r.width();
    } else if (server.pf().is888()) {
      rowSize = r.width() * 3;
    } else {
      rowSize = r.width() * server.pf().bpp/8;
    }

    dataSize = r.height() * rowSize;

    if (dataSize < TIGHT_MIN_TO_COMPRESS) {
      os.copyBytes(is, dataSize);
    } else {
      int len;

      len = readCompact(is);
      os.writeOpaque32(len);
      os.copyBytes(is, len);
    }
  }

  public boolean doRectsConflict(Rect rectA,
                                 Object bufferA,
                                 int buflenA,
                                 Rect rectB,
                                 Object bufferB,
                                 int buflenB,
                                 ServerParams server)
  {
    byte comp_ctl_a, comp_ctl_b;

    assert(buflenA >= 1);
    assert(buflenB >= 1);

    comp_ctl_a = ((byte[])bufferA)[0];
    comp_ctl_b = ((byte[])bufferB)[0];

    // Resets or use of zlib pose the same problem, so merge them
    if ((comp_ctl_a & 0x80) == 0x00)
      comp_ctl_a |= 1 << ((comp_ctl_a >> 4) & 0x03);
    if ((comp_ctl_b & 0x80) == 0x00)
      comp_ctl_b |= 1 << ((comp_ctl_b >> 4) & 0x03);

    if (((comp_ctl_a & 0x0f) & (comp_ctl_b & 0x0f)) != 0)
      return true;

    return false;
  }

  public void decodeRect(Rect r, Object buffer,
                         int buflen, ServerParams server,
                         ModifiablePixelBuffer pb)
  {
    ByteBuffer bufptr;
    PixelFormat pf = server.pf();

    int comp_ctl;

    bufptr = ByteBuffer.wrap((byte[])buffer);

    assert(buflen >= 1);

    comp_ctl = bufptr.get() & 0xff;
    buflen -= 1;

    // Reset zlib streams if we are told by the server to do so.
    for (int i = 0; i < 4; i++) {
      if ((comp_ctl & 1) != 0) {
        zis[i].reset();
      }
      comp_ctl >>= 1;
    }

    // "Fill" compression type.
    if (comp_ctl == tightFill) {
      if (pf.is888()) {
        ByteBuffer pix = ByteBuffer.allocate(4);

        assert(buflen >= 3);

        pf.bufferFromRGB(pix.duplicate(), bufptr, 1);
        pb.fillRect(pf, r, pix.array());
      } else {
        assert(buflen >= pf.bpp/8);
        byte[] pix = new byte[pf.bpp/8];
        bufptr.get(pix);
        pb.fillRect(pf, r, pix);
      }
      return;
    }

    // "JPEG" compression type.
    if (comp_ctl == tightJpeg) {
      int len;

      WritableRaster buf;

      JpegDecompressor jd = new JpegDecompressor();

      assert(buflen >= 4);

      len = bufptr.getInt();
      buflen -= 4;

      // We always use direct decoding with JPEG images
      jd.decompress(bufptr, len, pb, r, pb.getPF());
      pb.commitBufferRW(r);
      return;
    }

    // Quit on unsupported compression type.
    if (comp_ctl > tightMaxSubencoding)
      throw new Exception("TightDecoder: bad subencoding value received");

    // "Basic" compression type.
    int palSize = 0;
    ByteBuffer palette = ByteBuffer.allocate(256 * 4);
    boolean useGradient = false;

    if ((comp_ctl & tightExplicitFilter) != 0) {
      int filterId;

      assert(buflen >= 1);

      filterId = bufptr.get();

      switch (filterId) {
      case tightFilterPalette:
        assert(buflen >= 1);

        palSize = bufptr.getInt() + 1;
        buflen -= 4;

        if (pf.is888()) {
          ByteBuffer tightPalette = ByteBuffer.allocate(palSize * 3);

          assert(buflen >= tightPalette.capacity());

          bufptr.get(tightPalette.array(), 0, tightPalette.capacity());
          buflen -= tightPalette.capacity();

          pf.bufferFromRGB(palette.duplicate(), tightPalette, palSize);
        } else {
          int len;

          len = palSize * pf.bpp/8;

          assert(buflen >= len);

          bufptr.get(palette.array(), 0, len);
          buflen -= len;
        }
        break;
      case tightFilterGradient:
        useGradient = true;
        break;
      case tightFilterCopy:
        break;
      default:
        assert(false);
      }
    }

    // Determine if the data should be decompressed or just copied.
    int rowSize, dataSize;
    ByteBuffer netbuf;

    if (palSize != 0) {
      if (palSize <= 2)
        rowSize = (r.width() + 7) / 8;
      else
        rowSize = r.width();
    } else if (pf.is888()) {
      rowSize = r.width() * 3;
    } else {
      rowSize = r.width() * pf.bpp/8;
    }

    dataSize = r.height() * rowSize;

    if (dataSize < TIGHT_MIN_TO_COMPRESS) {
      assert(buflen >= dataSize);
    } else {
      int len;
      int streamId;
      MemInStream ms;

      assert(buflen >= 4);

      len = bufptr.getInt();
      buflen -= 4;

      assert(buflen >= len);

      streamId = comp_ctl & 0x03;
      ms = new MemInStream(bufptr.array(), bufptr.position(), len);
      zis[streamId].setUnderlying(ms, len);

      // Allocate netbuf and read in data
      netbuf = ByteBuffer.allocate(dataSize);

      zis[streamId].readBytes(netbuf, dataSize);

      zis[streamId].flushUnderlying();
      zis[streamId].setUnderlying(null, 0);
      ms = null;

      bufptr = (ByteBuffer)netbuf.flip();
      buflen = dataSize;
    }

    ByteBuffer outbuf = ByteBuffer.allocate(r.area() * pf.bpp/8);
    int stride = r.width();

    if (palSize == 0) {
      // Truecolor data.
      if (useGradient) {
        if (pf.is888()) {
          FilterGradient24(bufptr, pf, outbuf, stride, r);
        } else {
          switch (pf.bpp) {
          case 8:
            assert(false);
            break;
          case 16:
            FilterGradient(bufptr, pf, outbuf, stride, r);
            break;
          case 32:
            FilterGradient(bufptr, pf, outbuf, stride, r);
            break;
          }
        }
      } else {
        // Copy
        ByteBuffer ptr = (ByteBuffer)outbuf.duplicate().mark();
        ByteBuffer srcPtr = bufptr.duplicate();
        int w = r.width();
        int h = r.height();
        if (pf.is888()) {
          while (h > 0) {
            pf.bufferFromRGB(ptr.duplicate(), srcPtr.duplicate(), w);
            ptr.position(ptr.position() + stride * pf.bpp/8);
            srcPtr.position(srcPtr.position() + w * 3);
            h--;
          }
        } else {
          while (h > 0) {
            ptr.put(srcPtr.array(), srcPtr.position(), w * pf.bpp/8);
            ptr.reset().position(ptr.position() + stride * pf.bpp/8).mark();
            srcPtr.position(srcPtr.position() + w * pf.bpp/8);
            h--;
          }
        }
      }
    } else {
      // Indexed color
      switch (pf.bpp) {
      case 8:
        FilterPalette8(palette, palSize,
                       bufptr, outbuf, stride, r);
        break;
      case 16:
        FilterPalette16(palette.asShortBuffer(), palSize,
                        bufptr, outbuf.asShortBuffer(), stride, r);
        break;
      case 32:
        FilterPalette32(palette.asIntBuffer(), palSize,
                        bufptr, outbuf.asIntBuffer(), stride, r);
        break;
      }
    }

    pb.imageRect(pf, r, outbuf.array());

  }

  final private void FilterGradient24(ByteBuffer inbuf,
                                      PixelFormat pf, ByteBuffer outbuf,
                                      int stride, Rect r)
  {
    int x, y, c;
    byte[] prevRow = new byte[TIGHT_MAX_WIDTH*3];
    byte[] thisRow = new byte[TIGHT_MAX_WIDTH*3];
    ByteBuffer pix = ByteBuffer.allocate(3);
    int[] est = new int[3];

    // Set up shortcut variables
    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      for (x = 0; x < rectWidth; x++) {
        /* First pixel in a row */
        if (x == 0) {
          for (c = 0; c < 3; c++) {
            pix.put(c, (byte)(inbuf.get(y*rectWidth*3+c) + prevRow[c]));
            thisRow[c] = pix.get(c);
          }
          pf.bufferFromRGB((ByteBuffer)outbuf.duplicate().position(y*stride), pix, 1);
          continue;
        }

        for (c = 0; c < 3; c++) {
          est[c] = prevRow[x*3+c] + pix.get(c) - prevRow[(x-1)*3+c];
          if (est[c] > 0xff) {
            est[c] = 0xff;
          } else if (est[c] < 0) {
            est[c] = 0;
          }
          pix.put(c, (byte)(inbuf.get((y*rectWidth+x)*3+c) + est[c]));
          thisRow[x*3+c] = pix.get(c);
        }
        pf.bufferFromRGB((ByteBuffer)outbuf.duplicate().position(y*stride+x), pix, 1);
      }

      System.arraycopy(thisRow, 0, prevRow, 0, prevRow.length);
    }
  }

  final private void FilterGradient(ByteBuffer inbuf,
                                    PixelFormat pf, ByteBuffer outbuf,
                                    int stride, Rect r)
  {
    int x, y, c;
    byte[] prevRow = new byte[TIGHT_MAX_WIDTH];
    byte[] thisRow = new byte[TIGHT_MAX_WIDTH];
    ByteBuffer pix = ByteBuffer.allocate(3);
    int[] est = new int[3];

    // Set up shortcut variables
    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      for (x = 0; x < rectWidth; x++) {
        /* First pixel in a row */
        if (x == 0) {
          pf.rgbFromBuffer(pix.duplicate(), (ByteBuffer)inbuf.position(y*rectWidth), 1);
          for (c = 0; c < 3; c++)
            pix.put(c, (byte)(pix.get(c) + prevRow[c]));

          System.arraycopy(pix.array(), 0, thisRow, 0, pix.capacity());
          pf.bufferFromRGB((ByteBuffer)outbuf.duplicate().position(y*stride), pix, 1);
          continue;
        }

        for (c = 0; c < 3; c++) {
          est[c] = prevRow[x*3+c] + pix.get(c) - prevRow[(x-1)*3+c];
          if (est[c] > 0xff) {
            est[c] = 0xff;
          } else if (est[c] < 0) {
            est[c] = 0;
          }
        }

        pf.rgbFromBuffer(pix.duplicate(), (ByteBuffer)inbuf.position(y*rectWidth+x), 1);
        for (c = 0; c < 3; c++)
          pix.put(c, (byte)(pix.get(c) + est[c]));

        System.arraycopy(pix.array(), 0, thisRow, x*3, pix.capacity());

        pf.bufferFromRGB((ByteBuffer)outbuf.duplicate().position(y*stride+x), pix, 1);
      }

      System.arraycopy(thisRow, 0, prevRow, 0, prevRow.length);
    }
  }

  private void FilterPalette8(ByteBuffer palette, int palSize,
                              ByteBuffer inbuf, ByteBuffer outbuf,
                              int stride, Rect r)
  {
    // Indexed color
    int x, h = r.height(), w = r.width(), b, pad = stride - w;
    ByteBuffer ptr = outbuf.duplicate();
    byte bits;
    ByteBuffer srcPtr = inbuf.duplicate();
    if (palSize <= 2) {
      // 2-color palette
      while (h > 0) {
        for (x = 0; x < w / 8; x++) {
          bits = srcPtr.get();
          for (b = 7; b >= 0; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        if (w % 8 != 0) {
          bits = srcPtr.get();
          for (b = 7; b >= 8 - w % 8; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    } else {
      // 256-color palette
      while (h > 0) {
        int endOfRow = ptr.position() + w;
        while (ptr.position() < endOfRow) {
          ptr.put(palette.get(srcPtr.get()));
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    }
  }

  private void FilterPalette16(ShortBuffer palette, int palSize,
                               ByteBuffer inbuf, ShortBuffer outbuf,
                               int stride, Rect r)
  {
    // Indexed color
    int x, h = r.height(), w = r.width(), b, pad = stride - w;
    ShortBuffer ptr = outbuf.duplicate();
    byte bits;
    ByteBuffer srcPtr = inbuf.duplicate();
    if (palSize <= 2) {
      // 2-color palette
      while (h > 0) {
        for (x = 0; x < w / 8; x++) {
          bits = srcPtr.get();
          for (b = 7; b >= 0; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        if (w % 8 != 0) {
          bits = srcPtr.get();
          for (b = 7; b >= 8 - w % 8; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    } else {
      // 256-color palette
      while (h > 0) {
        int endOfRow = ptr.position() + w;
        while (ptr.position() < endOfRow) {
          ptr.put(palette.get(srcPtr.get()));
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    }
  }

  private void FilterPalette32(IntBuffer palette, int palSize,
                               ByteBuffer inbuf, IntBuffer outbuf,
                               int stride, Rect r)
  {
    // Indexed color
    int x, h = r.height(), w = r.width(), b, pad = stride - w;
    IntBuffer ptr = outbuf.duplicate();
    byte bits;
    ByteBuffer srcPtr = inbuf.duplicate();
    if (palSize <= 2) {
      // 2-color palette
      while (h > 0) {
        for (x = 0; x < w / 8; x++) {
          bits = srcPtr.get();
          for (b = 7; b >= 0; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        if (w % 8 != 0) {
          bits = srcPtr.get();
          for (b = 7; b >= 8 - w % 8; b--) {
            ptr.put(palette.get(bits >> b & 1));
          }
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    } else {
      // 256-color palette
      while (h > 0) {
        int endOfRow = ptr.position() + w;
        while (ptr.position() < endOfRow) {
          ptr.put(palette.get(srcPtr.get() & 0xff));
        }
        ptr.position(ptr.position() + pad);
        h--;
      }
    }
  }

  public final int readCompact(InStream is) {
    byte b;
    int result;

    b = (byte)is.readU8();
    result = (int)b & 0x7F;
    if ((b & 0x80) != 0) {
      b = (byte)is.readU8();
      result |= ((int)b & 0x7F) << 7;
      if ((b & 0x80) != 0) {
        b = (byte)is.readU8();
        result |= ((int)b & 0xFF) << 14;
      }
    }
    return result;
  }

  private ZlibInStream[] zis;

}
