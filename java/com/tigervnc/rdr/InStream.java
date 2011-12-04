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
// rdr::InStream marshalls data from a buffer stored in RDR (RFB Data
// Representation).
//

package com.tigervnc.rdr;

abstract public class InStream {

  // check() ensures there is buffer data for at least one item of size
  // itemSize bytes.  Returns the number of items in the buffer (up to a
  // maximum of nItems).

  public int check(int itemSize, int nItems, boolean wait) {
/*
    int available = end - ptr;
    if (itemSize * nItems > available) {
      if (itemSize > available)
        return overrun(itemSize, nItems, wait);

      nItems = available / itemSize;
    }
    return nItems;
*/
    if (ptr + itemSize * nItems > end) {
      if (ptr + itemSize > end)
        return overrun(itemSize, nItems, wait);

      nItems = (end - ptr) / itemSize;
    }
    return nItems;
  }

  public int check(int itemSize, int nItems) { return check(itemSize, nItems, true); }
  public int check(int itemSize) { return check(itemSize, 1); }

  // checkNoWait() tries to make sure that the given number of bytes can
  // be read without blocking.  It returns true if this is the case, false
  // otherwise.  The length must be "small" (less than the buffer size).

  public final boolean checkNoWait(int length) { return check(length, 1, false)!=0; }

  // readU/SN() methods read unsigned and signed N-bit integers.

  public final int readS8()  { check(1,1,true); return b[ptr++]; }
  public final int readS16() { check(2,1,true); int b0 = b[ptr++];
                               int b1 = b[ptr++] & 0xff; return b0 << 8 | b1; }
  public final int readS32() { check(4,1,true); int b0 = b[ptr++];
                               int b1 = b[ptr++] & 0xff;
                               int b2 = b[ptr++] & 0xff;
                               int b3 = b[ptr++] & 0xff;
                               return b0 << 24 | b1 << 16 | b2 << 8 | b3; }

  public final int readU8()  { return readS8()  & 0xff;  }
  public final int readU16() { return readS16() & 0xffff; }
  public final int readU32() { return readS32() & 0xffffffff; }

  // readString() reads a string - a U32 length followed by the data.

  public final String readString() {
    int len = readU32();
    if (len > maxStringLength)
      throw new Exception("InStream max string length exceeded");

    byte[] str = new byte[len];
    readBytes(str, 0, len);
    String utf8string = new String();
    try {
      utf8string = new String(str,"UTF8");
    } catch(java.io.UnsupportedEncodingException e) {
      e.printStackTrace();
    }
    return utf8string;
  }

  // maxStringLength protects against allocating a huge buffer.  Set it
  // higher if you need longer strings.

  public static int maxStringLength = 65535;

  public final void skip(int bytes) {
    while (bytes > 0) {
      int n = check(1, bytes, true);
      ptr += n;
      bytes -= n;
    }
  }

  // readBytes() reads an exact number of bytes into an array at an offset.

  public void readBytes(byte[] data, int dataPtr, int length) {
    int dataEnd = dataPtr + length;
    while (dataPtr < dataEnd) {
      int n = check(1, dataEnd - dataPtr, true);
      System.arraycopy(b, ptr, data, dataPtr, n);
      ptr += n;
      dataPtr += n;
    }
  }

  // readOpaqueN() reads a quantity "without byte-swapping".  Because java has
  // no byte-ordering, we just use big-endian.

  public final int readOpaque8()  { return readU8(); }
  public final int readOpaque16() { check(2); int r0 = b[ptr++];
                                    int r1 = b[ptr++]; return r0 << 8 | r1; }
  public final int readOpaque32() { check(4); int r0 = b[ptr++]; int r1 = b[ptr++]; 
                                    int r2 = b[ptr++]; int r3 = b[ptr++]; 
                                    return r0 << 24 | r1 << 16 | r2 << 8 | r3; }
  public final int readOpaque24A() { check(3, 1, true); int r0 = b[ptr++];
                                     int r1 = b[ptr++]; int r2 = b[ptr++];
                                     return r0 << 24 | r1 << 16 | r2 << 8; }
  public final int readOpaque24B() { check(3, 1, true); int r0 = b[ptr++];
                                     int r1 = b[ptr++]; int r2 = b[ptr++];
                                     return r0 << 16 | r1 << 8 | r2; }

  public final int readPixel(int bytesPerPixel, boolean bigEndian) {
    byte[] pix = new byte[4];
    readBytes(pix, 0, bytesPerPixel);

    if (bigEndian) {
      return 0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
    } else {
      return 0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
    }
  }

  public final void readPixels(int[] buf, int length, int bytesPerPixel, boolean bigEndian) {
    int npixels = length*bytesPerPixel;
    byte[] pixels = new byte[npixels];
    readBytes(pixels, 0, npixels);
    for (int i = 0; i < length; i++) {
      byte[] pix = new byte[4];
      System.arraycopy(pixels, i*bytesPerPixel, pix, 0, bytesPerPixel);
      if (bigEndian) {
        buf[i] = 0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
      } else {
        buf[i] = 0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
      }
    }
  }

  public final int readCompactLength() {
    int b = readU8();
    int result = b & 0x7F;
    if ((b & 0x80) != 0) {
      b = readU8();
      result |= (b & 0x7F) << 7;
      if ((b & 0x80) != 0) {
        b = readU8();
        result |= (b & 0xFF) << 14;
      }
    }
    return result;
  }
  
  // pos() returns the position in the stream.

  abstract public int pos();

  // bytesAvailable() returns true if at least one byte can be read from the
  // stream without blocking.  i.e. if false is returned then readU8() would
  // block.

  public boolean bytesAvailable() { return end != ptr; }

  // getbuf(), getptr(), getend() and setptr() are "dirty" methods which allow
  // you to manipulate the buffer directly.  This is useful for a stream which
  // is a wrapper around an underlying stream.

  public final byte[] getbuf() { return b; }
  public final int getptr() { return ptr; }
  public final int getend() { return end; }
  public final void setptr(int p) { ptr = p; }

  // overrun() is implemented by a derived class to cope with buffer overrun.
  // It ensures there are at least itemSize bytes of buffer data.  Returns
  // the number of items in the buffer (up to a maximum of nItems).  itemSize
  // is supposed to be "small" (a few bytes).

  abstract protected int overrun(int itemSize, int nItems, boolean wait);

  protected InStream() {}
  protected byte[] b;
  protected int ptr;
  protected int end;
  protected int start;
}
