/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
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

package com.tigervnc.rdr;

import java.nio.ByteBuffer;
import java.nio.channels.*;
import javax.net.ssl.*;

import com.tigervnc.network.*;

public class TLSOutStream extends OutStream {

  static final int defaultBufSize = 16384;

  public TLSOutStream(OutStream _out, SSLEngineManager _manager) {
    manager = _manager;
    out = (FdOutStream)_out;
    SSLSession session = manager.getSession();
    bufSize = session.getApplicationBufferSize();
    b = new byte[bufSize];
    ptr = offset = start = 0;
    end = start + bufSize;
  }

  public int length()
  {
    return offset + ptr - start;
  }

  public void flush()
  {
    int sentUpTo = start;
    while (sentUpTo < ptr) {
      int n = writeTLS(b, sentUpTo, ptr - sentUpTo);
      sentUpTo += n;
      offset += n;
    }

    ptr = start;
  }

  protected int overrun(int itemSize, int nItems)
  {
    if (itemSize > bufSize)
      throw new Exception("TLSOutStream overrun: max itemSize exceeded");

    flush();

    int nAvail;
    nAvail = (end - ptr) / itemSize;
    if (nAvail < nItems)
      return nAvail;

    return nItems;
  }

  protected int writeTLS(byte[] data, int dataPtr, int length)
  {
    int n = 0;

    try {
      n = manager.write(ByteBuffer.wrap(data, dataPtr, length), length);
    } catch (java.io.IOException e) {
      throw new Exception(e.getMessage());
    }

    return n;
  }

  private SSLEngineManager manager;
  private FdOutStream out;
  private int start;
  private int offset;
  private int bufSize;
}
