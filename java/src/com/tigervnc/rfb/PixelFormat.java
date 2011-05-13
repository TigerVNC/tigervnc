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

  public void bufferFromRGB(int dst, byte[] src) {
    if (bigEndian) {
      dst =
        (src[0] & 0xFF) << 16 | (src[1] & 0xFF) << 8 | (src[2] & 0xFF) | 0xFF << 24;
    } else {
      dst =
        (src[2] & 0xFF) << 16 | (src[1] & 0xFF) << 8 | (src[0] & 0xFF) | 0xFF << 24;
    }
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
