/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

//
// rdr::InStream marshalls data from a buffer stored in RDR (RFB Data
// Representation).
//

package com.tigervnc.rdr;

import java.nio.*;

import com.tigervnc.network.*;

abstract public class InStream {

  // check() ensures there is buffer data for at least one item of size
  // itemSize bytes.  Returns the number of items in the buffer (up to a
  // maximum of nItems).

  public int check(int itemSize, int nItems, boolean wait) {
    int nAvail;

    if (itemSize > (end - ptr))
        return overrun(itemSize, nItems, wait);

    nAvail = (end - ptr) / itemSize;
    if (nAvail < nItems)
      return nAvail;

    return nItems;
  }

  public int check(int itemSize, int nItems) { return check(itemSize, nItems, true); }
  public int check(int itemSize) { return check(itemSize, 1); }

  // checkNoWait() tries to make sure that the given number of bytes can
  // be read without blocking.  It returns true if this is the case, false
  // otherwise.  The length must be "small" (less than the buffer size).

  public final boolean checkNoWait(int length) { return check(length, 1, false)!=0; }

  // readU/SN() methods read unsigned and signed N-bit integers.

  public final int readS8()  { check(1); return b[ptr++]; }
  public final int readS16() { check(2); int b0 = b[ptr++];
                               int b1 = b[ptr++] & 0xff; return b0 << 8 | b1; }
  public final int readS32() { check(4); int b0 = b[ptr++];
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

    ByteBuffer str = ByteBuffer.allocate(len);
    readBytes(str, len);
    String utf8string = new String();
    try {
      utf8string = new String(str.array(),"UTF8");
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
      int n = check(1, bytes);
      ptr += n;
      bytes -= n;
    }
  }

  // readBytes() reads an exact number of bytes

  public void readBytes(ByteBuffer data, int length) {
    while (length > 0) {
      int n = check(1, length);
      data.put(b, ptr, n);
      ptr += n;
      length -= n;
    }
  }

  // readOpaqueN() reads a quantity "without byte-swapping".  Because java has
  // no byte-ordering, we just use big-endian.

  public final int readOpaque8()  { return readU8(); }
  public final int readOpaque16() { return readU16(); }
  public final int readOpaque32() { return readU32(); }

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
  protected int overrun(int itemSize, int nItems) {
    return overrun(itemSize, nItems, true);
  }

  protected InStream() {}
  protected byte[] b;
  protected int ptr;
  protected int end;
}
