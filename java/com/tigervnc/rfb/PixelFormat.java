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

//
// PixelFormat
//

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;
import java.awt.image.ColorModel;

public class PixelFormat {

  public PixelFormat(int b, int d, boolean e, boolean t) {
    bpp = b;
    depth = d;
    bigEndian = e;
    trueColour = t;
  }
  public PixelFormat(int b, int d, boolean e, boolean t,
                     int rm, int gm, int bm, int rs, int gs, int bs) {
    this(b, d, e, t);
    redMax = rm;
    greenMax = gm;
    blueMax = bm;
    redShift = rs;
    greenShift = gs;
    blueShift = bs;
  }
  public PixelFormat() { this(8,8,false,true,7,7,3,0,3,6); }

  public boolean equal(PixelFormat x) {
    return (bpp == x.bpp &&
            depth == x.depth &&
            (bigEndian == x.bigEndian || bpp == 8) &&
            trueColour == x.trueColour &&
            (!trueColour || (redMax == x.redMax &&
                             greenMax == x.greenMax &&
                             blueMax == x.blueMax &&
                             redShift == x.redShift &&
                             greenShift == x.greenShift &&
                             blueShift == x.blueShift)));
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

  public void bufferFromRGB(int[] dst, int dstPtr, byte[] src, 
                            int srcPtr, int pixels) {
    if (is888()) {
      // Optimised common case
      int r, g, b;

      for (int i=srcPtr; i < pixels; i++) {
        if (bigEndian) {
          r = (src[3*i+0] & 0xff) << (24 - redShift);
          g = (src[3*i+1] & 0xff) << (24 - greenShift);
          b = (src[3*i+2] & 0xff) << (24 - blueShift);
          dst[dstPtr+i] = r | g | b | 0xff;
        } else {
          r = (src[3*i+0] & 0xff) << redShift;
          g = (src[3*i+1] & 0xff) << greenShift;
          b = (src[3*i+2] & 0xff) << blueShift;
          dst[dstPtr+i] = (0xff << 24) | r | g | b;
        }
      }
    } else {
      // Generic code
      int p, r, g, b;
      int[] rgb = new int[4];
      
      int i = srcPtr; int j = dstPtr;
      while (i < pixels) {
        r = src[i++] & 0xff;
        g = src[i++] & 0xff;
        b = src[i++] & 0xff;

        //p = pixelFromRGB(r, g, b, cm);
        p = ColorModel.getRGBdefault().getDataElement(new int[] {0xff, r, g, b}, 0);

        bufferFromPixel(dst, j, p);
        j += bpp/8;
      }
    }
  }

  public void rgbFromBuffer(byte[] dst, int dstPtr, byte[] src, int srcPtr, int pixels, ColorModel cm)
  {
    int p;
    byte r, g, b;
  
    for (int i=0; i < pixels; i++) {
      p = pixelFromBuffer(src, srcPtr); 
      srcPtr += bpp/8;
  
      dst[dstPtr++] = (byte)cm.getRed(p);
      dst[dstPtr++] = (byte)cm.getGreen(p);
      dst[dstPtr++] = (byte)cm.getBlue(p);
    }
  }

  public int pixelFromBuffer(byte[] buffer, int bufferPtr)
  {
    int p;
  
    p = 0;
  
    if (bigEndian) {
      switch (bpp) {
      case 32:
        p = (buffer[0] & 0xff) << 24 | (buffer[1] & 0xff) << 16 | (buffer[2] & 0xff) << 8 | 0xff;
        break;
      case 16:
        p = (buffer[0] & 0xff) << 8 | (buffer[1] & 0xff);
        break;
      case 8:
        p = (buffer[0] & 0xff);
        break;
      }
    } else {
      p = (buffer[0] & 0xff);
      if (bpp >= 16) {
        p |= (buffer[1] & 0xff) << 8;
        if (bpp == 32) {
          p |= (buffer[2] & 0xff) << 16;
          p |= (buffer[3] & 0xff) << 24;
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

  public void bufferFromPixel(int[] buffer, int bufPtr, int p)
  {
    if (bigEndian) {
      switch (bpp) {
        case 32:
          buffer[bufPtr++] = (p >> 24) & 0xff;
          buffer[bufPtr++] = (p >> 16) & 0xff;
          break;
        case 16:
          buffer[bufPtr++] = (p >> 8) & 0xff;
          break;
        case 8:
          buffer[bufPtr++] = (p >> 0) & 0xff;
          break;
      }
    } else {
      buffer[0] = (p >> 0) & 0xff;
      if (bpp >= 16) {
        buffer[1] = (p >> 8) & 0xff;
        if (bpp == 32) {
          buffer[2] = (p >> 16) & 0xff;
          buffer[3] = (p >> 24) & 0xff;
        }
      }
    }
  }
          

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
}
