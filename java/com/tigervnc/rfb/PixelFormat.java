/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011 Brian P. Hinz
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

//
// PixelFormat
//

package com.tigervnc.rfb;

import java.awt.color.*;
import java.awt.image.*;
import java.nio.*;
import java.util.*;

import com.tigervnc.rdr.*;

public class PixelFormat {

  public PixelFormat(int b, int d, boolean e, boolean t,
                     int rm, int gm, int bm, int rs, int gs, int bs)
  {
    bpp = b; depth = d; trueColour = t; bigEndian = e;
    redMax = rm; greenMax = gm; blueMax = bm;
    redShift = rs; greenShift = gs; blueShift = bs;
    converters = new HashMap<Integer, ColorConvertOp>();
    if (!isSane())
      throw new Exception("invalid pixel format");

    updateState();
  }

  public PixelFormat()
  {
    this(8, 8, false, true, 7, 7, 3, 0, 3, 6);
    updateState();
  }

  public boolean equal(PixelFormat other)
  {
    if (bpp != other.bpp || depth != other.depth)
      return false;

    if (redMax != other.redMax)
      return false;
    if (greenMax != other.greenMax)
      return false;
    if (blueMax != other.blueMax)
      return false;

    // Endianness requires more care to determine compatibility
    if (bigEndian == other.bigEndian || bpp == 8) {
      if (redShift != other.redShift)
        return false;
      if (greenShift != other.greenShift)
        return false;
      if (blueShift != other.blueShift)
        return false;
    } else {
      // Has to be the same byte for each channel
      if (redShift/8 != (3 - other.redShift/8))
        return false;
      if (greenShift/8 != (3 - other.greenShift/8))
        return false;
      if (blueShift/8 != (3 - other.blueShift/8))
        return false;

      // And the same bit offset within the byte
      if (redShift%8 != other.redShift%8)
        return false;
      if (greenShift%8 != other.greenShift%8)
        return false;
      if (blueShift%8 != other.blueShift%8)
        return false;

      // And not cross a byte boundary
      if (redShift/8 != (redShift + redBits - 1)/8)
        return false;
      if (greenShift/8 != (greenShift + greenBits - 1)/8)
        return false;
      if (blueShift/8 != (blueShift + blueBits - 1)/8)
        return false;
    }

    return true;
  }

  public void read(InStream is) {
    bpp = is.readU8();
    depth = is.readU8();
    bigEndian = is.readU8()!=0;
    trueColour = is.readU8()!=0;
    redMax = is.readU16();
    greenMax = is.readU16();
    blueMax = is.readU16();
    redShift = is.readU8();
    greenShift = is.readU8();
    blueShift = is.readU8();
    is.skip(3);

    // We have no real support for colour maps. If the client
    // wants one, then we force a 8-bit true colour format and
    // pretend it's a colour map.
    if (!trueColour) {
      redMax = 7;
      greenMax = 7;
      blueMax = 3;
      redShift = 0;
      greenShift = 3;
      blueShift = 6;
    }

    if (!isSane())
      throw new Exception("invalid pixel format: "+print());

    updateState();
  }

  public void write(OutStream os) {
    os.writeU8(bpp);
    os.writeU8(depth);
    os.writeU8(bigEndian?1:0);
    os.writeU8(trueColour?1:0);
    os.writeU16(redMax);
    os.writeU16(greenMax);
    os.writeU16(blueMax);
    os.writeU8(redShift);
    os.writeU8(greenShift);
    os.writeU8(blueShift);
    os.pad(3);
  }

  public final boolean isBigEndian() {
    return bigEndian;
  }

  public final boolean isLittleEndian() {
    return ! bigEndian;
  }

  public final boolean is888() {
    if(!trueColour)
      return false;
    if(bpp != 32)
      return false;
    if(depth != 24)
      return false;
    if(redMax != 255)
      return false;
    if(greenMax != 255)
      return false;
    if(blueMax != 255)
      return false;
    if ((redShift & 0x7) != 0)
      return false;
    if ((greenShift & 0x7) != 0)
      return false;
    if ((blueShift & 0x7) != 0)
      return false;

    return true;
  }

  public int pixelFromRGB(int red, int green, int blue, ColorModel cm)
  {
    if (trueColour) {
      int r = (red   * redMax     + 32767) / 65535;
      int g = (green * greenMax   + 32767) / 65535;
      int b = (blue  * blueMax    + 32767) / 65535;

      return (r << redShift) | (g << greenShift) | (b << blueShift);
    } else if (cm != null) {
      // Try to find the closest pixel by Cartesian distance
      int colours = 1 << depth;
      int diff = 256 * 256 * 4;
      int col = 0;
      for (int i=0; i<colours; i++) {
        int r, g, b;
        r = cm.getRed(i);
        g = cm.getGreen(i);
        b = cm.getBlue(i);
        int rd = (r-red) >> 8;
        int gd = (g-green) >> 8;
        int bd = (b-blue) >> 8;
        int d = rd*rd + gd*gd + bd*bd;
        if (d < diff) {
          col = i;
          diff = d;
        }
      }
      return col;
    }
    // XXX just return 0 for colour map?
    return 0;
  }

  // This method should be invoked with duplicates of dst/src Buffers
  public void bufferFromRGB(ByteBuffer dst, ByteBuffer src, int pixels)
  {
    bufferFromRGB(dst, src, pixels, pixels, 1);
  }

  // This method should be invoked with duplicates of dst/src Buffers
  public void bufferFromRGB(ByteBuffer dst, ByteBuffer src,
                            int w, int stride, int h)
  {
    if (is888()) {
      // Optimised common case
      int r, g, b, x;

      if (bigEndian) {
        r = dst.position() + (24 - redShift)/8;
        g = dst.position() + (24 - greenShift)/8;
        b = dst.position() + (24 - blueShift)/8;
        x = dst.position() + (24 - (48 - redShift - greenShift - blueShift))/8;
      } else {
        r = dst.position() + redShift/8;
        g = dst.position() + greenShift/8;
        b = dst.position() + blueShift/8;
        x = dst.position() + (48 - redShift - greenShift - blueShift)/8;
      }

      int dstPad = (stride - w) * 4;
      while (h-- > 0) {
        int w_ = w;
        while (w_-- > 0) {
          dst.put(r, src.get());
          dst.put(g, src.get());
          dst.put(b, src.get());
          dst.put(x, (byte)0);
          r += 4;
          g += 4;
          b += 4;
          x += 4;
        }
        r += dstPad;
        g += dstPad;
        b += dstPad;
        x += dstPad;
      }
    } else {
      // Generic code
      int dstPad = (stride - w) * bpp/8;
      while (h-- > 0) {
        int w_ = w;
        while (w_-- > 0) {
          int p;
          int r, g, b;

          r = src.get();
          g = src.get();
          b = src.get();

          p = pixelFromRGB(r, g, b, model);

          bufferFromPixel(dst, p);
          dst.position(dst.position() + bpp/8);
        }
        dst.position(dst.position() + dstPad);
      }
    }
  }

  // This method should be invoked with duplicates of dst/src Buffers
  public void rgbFromBuffer(ByteBuffer dst, ByteBuffer src, int pixels)
  {
    rgbFromBuffer(dst, src, pixels, pixels, 1);
  }

  // This method should be invoked with duplicates of dst/src Buffers
  public void rgbFromBuffer(ByteBuffer dst, ByteBuffer src,
                            int w, int stride, int h)
  {
    if (is888()) {
      // Optimised common case
      int r, g, b;

      if (bigEndian) {
        r = src.position() + (24 - redShift)/8;
        g = src.position() + (24 - greenShift)/8;
        b = src.position() + (24 - blueShift)/8;
      } else {
        r = src.position() + redShift/8;
        g = src.position() + greenShift/8;
        b = src.position() + blueShift/8;
      }

      int srcPad = (stride - w) * 4;
      while (h-- > 0) {
        int w_ = w;
        while (w_-- > 0) {
          dst.put(src.get(r));
          dst.put(src.get(g));
          dst.put(src.get(b));
          r += 4;
          g += 4;
          b += 4;
        }
        r += srcPad;
        g += srcPad;
        b += srcPad;
      }
    } else {
      // Generic code
      int srcPad = (stride - w) * bpp/8;
      while (h-- > 0) {
        int w_ = w;
        while (w_-- > 0) {
          int p;
          byte r, g, b;

          p = pixelFromBuffer(src.duplicate());

          r = (byte)getColorModel().getRed(p);
          g = (byte)getColorModel().getGreen(p);
          b = (byte)getColorModel().getBlue(p);

          dst.put(r);
          dst.put(g);
          dst.put(b);
          src.position(src.position() + bpp/8);
        }
        src.position(src.position() + srcPad);
      }
    }
  }

  public void rgbFromPixels(byte[] dst, int dstPtr, int[] src, int srcPtr, int pixels, ColorModel cm)
  {
    int p;
    byte r, g, b;

    for (int i=0; i < pixels; i++) {
      p = src[i];

      dst[dstPtr++] = (byte)cm.getRed(p);
      dst[dstPtr++] = (byte)cm.getGreen(p);
      dst[dstPtr++] = (byte)cm.getBlue(p);
    }
  }

  // This method should be invoked with a duplicates of buffer
  public int pixelFromBuffer(ByteBuffer buffer)
  {
    int p;

    p = 0xff000000;

    if (!bigEndian) {
      switch (bpp) {
      case 32:
        p |= buffer.get() << 24;
        p |= buffer.get() << 16;
      case 16:
        p |= buffer.get() << 8;
      case 8:
        p |= buffer.get();
      }
    } else {
      p |= buffer.get(0);
      if (bpp >= 16) {
        p |= buffer.get(1) << 8;
        if (bpp == 32) {
          p |= buffer.get(2) << 16;
          p |= buffer.get(3) << 24;
        }
      }
    }

    return p;
  }

  public String print() {
    StringBuffer s = new StringBuffer();
    s.append("depth "+depth+" ("+bpp+"bpp)");
    if (bpp != 8) {
      if (bigEndian)
        s.append(" big-endian");
      else
        s.append(" little-endian");
    }

    if (!trueColour) {
      s.append(" colour-map");
      return s.toString();
    }

    if (blueShift == 0 && greenShift > blueShift && redShift > greenShift &&
        blueMax  == (1 << greenShift) - 1 &&
        greenMax == (1 << (redShift-greenShift)) - 1 &&
        redMax   == (1 << (depth-redShift)) - 1)
    {
      s.append(" rgb"+(depth-redShift)+(redShift-greenShift)+greenShift);
      return s.toString();
    }

    if (redShift == 0 && greenShift > redShift && blueShift > greenShift &&
        redMax   == (1 << greenShift) - 1 &&
        greenMax == (1 << (blueShift-greenShift)) - 1 &&
        blueMax  == (1 << (depth-blueShift)) - 1)
    {
      s.append(" bgr"+(depth-blueShift)+(blueShift-greenShift)+greenShift);
      return s.toString();
    }

    s.append(" rgb max "+redMax+","+greenMax+","+blueMax+" shift "+redShift+
             ","+greenShift+","+blueShift);
    return s.toString();
  }

  private static int bits(int value)
  {
    int bits;

    bits = 16;

    if ((value & 0xff00) == 0) {
      bits -= 8;
      value <<= 8;
    }
    if ((value & 0xf000) == 0) {
      bits -= 4;
      value <<= 4;
    }
    if ((value & 0xc000) == 0) {
      bits -= 2;
      value <<= 2;
    }
    if ((value & 0x8000) == 0) {
      bits -= 1;
      value <<= 1;
    }

    return bits;
  }

  private void updateState()
  {
    int endianTest = 1;

    redBits = bits(redMax);
    greenBits = bits(greenMax);
    blueBits = bits(blueMax);

    maxBits = redBits;
    if (greenBits > maxBits)
      maxBits = greenBits;
    if (blueBits > maxBits)
      maxBits = blueBits;

    minBits = redBits;
    if (greenBits < minBits)
      minBits = greenBits;
    if (blueBits < minBits)
      minBits = blueBits;

    if ((((char)endianTest) == 0) != bigEndian)
      endianMismatch = true;
    else
      endianMismatch = false;

    model = getColorModel(this);
  }

  private boolean isSane()
  {
    int totalBits;

    if ((bpp != 8) && (bpp != 16) && (bpp != 32))
      return false;
    if (depth > bpp)
      return false;

    if (!trueColour && (depth != 8))
      return false;

    if ((redMax & (redMax + 1)) != 0)
      return false;
    if ((greenMax & (greenMax + 1)) != 0)
      return false;
    if ((blueMax & (blueMax + 1)) != 0)
      return false;

    /*
     * We don't allow individual channels > 8 bits in order to keep our
     * conversions simple.
     */
    if (redMax >= (1 << 8))
      return false;
    if (greenMax >= (1 << 8))
      return false;
    if (blueMax >= (1 << 8))
      return false;

    totalBits = bits(redMax) + bits(greenMax) + bits(blueMax);
    if (totalBits > depth)
      return false;

    if ((bits(redMax) + redShift) > bpp)
      return false;
    if ((bits(greenMax) + greenShift) > bpp)
      return false;
    if ((bits(blueMax) + blueShift) > bpp)
      return false;

    if (((redMax << redShift) & (greenMax << greenShift)) != 0)
      return false;
    if (((redMax << redShift) & (blueMax << blueShift)) != 0)
      return false;
    if (((greenMax << greenShift) & (blueMax << blueShift)) != 0)
      return false;

    return true;
  }

  public void bufferFromPixel(ByteBuffer buffer, int p)
  {
    if (bigEndian) {
      switch (bpp) {
        case 32:
          buffer.put((byte)((p >> 24) & 0xff));
          buffer.put((byte)((p >> 16) & 0xff));
          break;
        case 16:
          buffer.put((byte)((p >> 8) & 0xff));
          break;
        case 8:
          buffer.put((byte)((p >> 0) & 0xff));
          break;
      }
    } else {
      buffer.put(0, (byte)((p >> 0) & 0xff));
      if (bpp >= 16) {
        buffer.put(1, (byte)((p >> 8) & 0xff));
        if (bpp == 32) {
          buffer.put(2, (byte)((p >> 16) & 0xff));
          buffer.put(3, (byte)((p >> 24) & 0xff));
        }
      }
    }
  }

  public ColorModel getColorModel()
  {
    return model;
  }

  public static ColorModel getColorModel(PixelFormat pf) {
    if (!(pf.bpp == 32) && !(pf.bpp == 16) && !(pf.bpp == 8))
      throw new Exception("Internal error: bpp must be 8, 16, or 32 in PixelBuffer ("+pf.bpp+")");
    ColorModel cm;
    switch (pf.depth) {
    case  3:
      // Fall-through to depth 8
    case  6:
      // Fall-through to depth 8
    case  8:
      int rmask = pf.redMax << pf.redShift;
      int gmask = pf.greenMax << pf.greenShift;
      int bmask = pf.blueMax << pf.blueShift;
      cm = new DirectColorModel(8, rmask, gmask, bmask);
      break;
    case 16:
      cm = new DirectColorModel(32, 0xF800, 0x07C0, 0x003E);
      break;
    case 24:
      cm = new DirectColorModel(32, (0xff << 16), (0xff << 8), 0xff);
      break;
    case 32:
      cm = new DirectColorModel(32, (0xff << pf.redShift),
        (0xff << pf.greenShift), (0xff << pf.blueShift));
      break;
    default:
      throw new Exception("Unsupported color depth ("+pf.depth+")");
    }
    assert(cm != null);
    return cm;
  }

  public ColorConvertOp getColorConvertOp(ColorSpace src)
  {
    // The overhead associated with initializing ColorConvertOps is
    // enough to justify maintaining a static lookup table.
    if (converters.containsKey(src.getType()))
      return converters.get(src.getType());
    ColorSpace dst = model.getColorSpace();
    converters.put(src.getType(), new ColorConvertOp(src, dst, null));
    return converters.get(src.getType());
  }

  public ByteOrder getByteOrder()
  {
    if (isBigEndian())
      return ByteOrder.BIG_ENDIAN;
    else
      return ByteOrder.LITTLE_ENDIAN;
  }

  public Raster rasterFromBuffer(Rect r, ByteBuffer buf)
  {
    Buffer dst;
    DataBuffer db = null;

    SampleModel sm =
      model.createCompatibleSampleModel(r.width(), r.height());
    switch (sm.getTransferType()) {
    case DataBuffer.TYPE_INT:
      dst = IntBuffer.allocate(r.area()).put(buf.asIntBuffer());
      db = new DataBufferInt(((IntBuffer)dst).array(), r.area());
      break;
    case DataBuffer.TYPE_BYTE:
      db = new DataBufferByte(buf.array(), r.area());
      break;
    case DataBuffer.TYPE_SHORT:
      dst = ShortBuffer.allocate(r.area()).put(buf.asShortBuffer());
      db = new DataBufferShort(((ShortBuffer)dst).array(), r.area());
      break;
    }
    assert(db != null);
    return Raster.createRaster(sm, db, new java.awt.Point(0, 0));
  }

  private static HashMap<Integer, ColorConvertOp> converters;

  public int bpp;
  public int depth;
  public boolean bigEndian;
  public boolean trueColour;
  public int redMax;
  public int greenMax;
  public int blueMax;
  public int redShift;
  public int greenShift;
  public int blueShift;

  protected int redBits, greenBits, blueBits;
  protected int maxBits, minBits;
  protected boolean endianMismatch;

  private ColorModel model;
}
