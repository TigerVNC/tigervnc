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

public class TLSInStream extends InStream {

  static final int defaultBufSize = 16384;

  public TLSInStream(InStream _in, SSLEngineManager _manager) {
    in = (FdInStream)_in;
    manager = _manager;
    offset = 0;
    SSLSession session = manager.getSession();
    bufSize = session.getApplicationBufferSize();
    b = new byte[bufSize];
    ptr = end = start = 0;
  }

  public final int pos() {
    return offset + ptr - start;
  }

  public final void startTiming() {
    in.startTiming();
  }

  public final void stopTiming() {
    in.stopTiming();
  }

  public final long kbitsPerSecond() {
    return in.kbitsPerSecond();
  }

  public final long timeWaited() {
    return in.timeWaited();
  }

  protected final int overrun(int itemSize, int nItems, boolean wait) {
    if (itemSize > bufSize)
      throw new Exception("TLSInStream overrun: max itemSize exceeded");

    if (end - ptr != 0)
      System.arraycopy(b, ptr, b, 0, end - ptr);

    offset += ptr - start;
    end -= ptr - start;
    ptr = start;

    while ((end - start) < itemSize) {
      int n = readTLS(b, end, start + bufSize - end, wait);
      if (!wait && n == 0)
        return 0;
      end += n;
    }

    int nAvail;
    nAvail = (end - ptr) / itemSize;
    if (nAvail < nItems)
      return nAvail;

    return nItems;
  }

  protected int readTLS(byte[] buf, int bufPtr, int len, boolean wait)
  {
    int n = -1;

    try {
      n = manager.read(ByteBuffer.wrap(buf, bufPtr, len), len);
    } catch (java.io.IOException e) {
      e.printStackTrace();
    }

    if (n < 0) throw new TLSException("readTLS", n);

    return n;
  }

  private SSLEngineManager manager;
  private int offset;
  private int start;
  private int bufSize;
  private FdInStream in;
}
