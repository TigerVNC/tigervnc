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
// rdr::OutStream marshalls data into a buffer stored in RDR (RFB Data
// Representation).
//

package com.tigervnc.rdr;

abstract public class OutStream {

  // check() ensures there is buffer space for at least one item of size
  // itemSize bytes.  Returns the number of items which fit (up to a maximum
  // of nItems).

  public final int check(int itemSize, int nItems) {
    if (ptr + itemSize * nItems > end) {
      if (ptr + itemSize > end)
        return overrun(itemSize, nItems);

      nItems = (end - ptr) / itemSize;
    }
    return nItems;
  }

  public final void check(int itemSize) {
    if (ptr + itemSize > end)
      overrun(itemSize, 1);
  }

  // writeU/SN() methods write unsigned and signed N-bit integers.

  public final void writeU8( int u) { check(1); b[ptr++] = (byte)u; }
  public final void writeU16(int u) { check(2); b[ptr++] = (byte)(u >> 8);
                                      b[ptr++] = (byte)u; }
  public final void writeU32(int u) { check(4); b[ptr++] = (byte)(u >> 24);
                                      b[ptr++] = (byte)(u >> 16);
                                      b[ptr++] = (byte)(u >> 8);
                                      b[ptr++] = (byte)u; }

  public final void writeS8( int s) { writeU8( s); }
  public final void writeS16(int s) { writeU16(s); }
  public final void writeS32(int s) { writeU32(s); }

  // writeString() writes a string - a U32 length followed by the data.

  public final void writeString(String str) {
    int len = str.length();
    writeU32(len);
    for (int i = 0; i < len;) {
      int j = i + check(1, len - i);
      while (i < j) {
	b[ptr++] = (byte)str.charAt(i++);
      }
    }
  }

  public final void pad(int bytes) {
    while (bytes-- > 0) writeU8(0);
  }

  public final void skip(int bytes) {
    while (bytes > 0) {
      int n = check(1, bytes);
      ptr += n;
      bytes -= n;
    }
  }

  // writeBytes() writes an exact number of bytes from an array at an offset.

  public void writeBytes(byte[] data, int offset, int length) {
    int offsetEnd = offset + length;
    while (offset < offsetEnd) {
      int n = check(1, offsetEnd - offset);
      System.arraycopy(data, offset, b, ptr, n);
      ptr += n;
      offset += n;
    }
  }

  // writeOpaqueN() writes a quantity without byte-swapping.  Because java has
  // no byte-ordering, we just use big-endian.

  public final void writeOpaque8( int u) { writeU8( u); }
  public final void writeOpaque16(int u) { writeU16(u); }
  public final void writeOpaque32(int u) { writeU32(u); }
  public final void writeOpaque24A(int u) { check(3);
                                            b[ptr++] = (byte)(u >> 24);
                                            b[ptr++] = (byte)(u >> 16);
                                            b[ptr++] = (byte)(u >> 8); }
  public final void writeOpaque24B(int u) { check(3);
                                            b[ptr++] = (byte)(u >> 16);
                                            b[ptr++] = (byte)(u >> 8);
                                            b[ptr++] = (byte)u; }

  // length() returns the length of the stream.

  abstract public int length();

  // flush() requests that the stream be flushed.

  public void flush() {}

  // getptr(), getend() and setptr() are "dirty" methods which allow you to
  // manipulate the buffer directly.  This is useful for a stream which is a
  // wrapper around an underlying stream.

  public final byte[] getbuf() { return b; }
  public final int getptr() { return ptr; }
  public final int getend() { return end; }
  public final void setptr(int p) { ptr = p; }

  // overrun() is implemented by a derived class to cope with buffer overrun.
  // It ensures there are at least itemSize bytes of buffer space.  Returns
  // the number of items which fit (up to a maximum of nItems).  itemSize is
  // supposed to be "small" (a few bytes).

  abstract protected int overrun(int itemSize, int nItems);

  protected OutStream() {}
  protected byte[] b;
  protected int ptr;
  protected int end;
}
