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
// A MemOutStream grows as needed when data is written to it.
//

package com.tigervnc.rdr;

public class MemOutStream extends OutStream {

  public MemOutStream(int len) {
    b = new byte[len];
    ptr = 0;
    end = len;
  }
  public MemOutStream() { this(1024); }

  public int length() { return ptr; }
  public void clear() { ptr = 0; };
  public void reposition(int pos) { ptr = pos; }

  // data() returns a pointer to the buffer.

  public final byte[] data() { return b; }

  // overrun() either doubles the buffer or adds enough space for nItems of
  // size itemSize bytes.

  protected int overrun(int itemSize, int nItems) {
    int len = ptr + itemSize * nItems;
    if (len < end * 2)
      len = end * 2;

    if (len < end)
      throw new Exception("Overflow in MemOutStream::overrun()");

    byte[] newBuf = new byte[len];
    System.arraycopy(b, 0, newBuf, 0, ptr);
    b = newBuf;
    end = len;

    return nItems;
  }
}
