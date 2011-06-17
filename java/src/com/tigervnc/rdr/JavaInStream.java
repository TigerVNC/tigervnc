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
// A JavaInStream reads from a java.io.InputStream
//

package com.tigervnc.rdr;

public class JavaInStream extends InStream {

  static final int defaultBufSize = 8192;
  static final int minBulkSize = 1024;

  public JavaInStream(java.io.InputStream jis_, int bufSize_) {
    jis = jis_;
    bufSize = bufSize_;
    b = new byte[bufSize];
    ptr = end = ptrOffset = 0;
    timeWaitedIn100us = 5;
    timedKbits = 0;
  }

  public JavaInStream(java.io.InputStream jis_) { this(jis_, defaultBufSize); }

  public void readBytes(byte[] data, int offset, int length) {
    if (length < minBulkSize) {
      super.readBytes(data, offset, length);
      return;
    }

    int n = end - ptr;
    if (n > length) n = length;

    System.arraycopy(b, ptr, data, offset, n);
    offset += n;
    length -= n;
    ptr += n;

    while (length > 0) {
      n = read(data, offset, length);
      offset += n;
      length -= n;
      ptrOffset += n;
    }
  }

  public int pos() { return ptrOffset + ptr; }

  public void startTiming() {
    timing = true;

    // Carry over up to 1s worth of previous rate for smoothing.

    if (timeWaitedIn100us > 10000) {
      timedKbits = timedKbits * 10000 / timeWaitedIn100us;
      timeWaitedIn100us = 10000;
    }
  }

  public void stopTiming() {
    timing = false; 
    if (timeWaitedIn100us < timedKbits/2)
      timeWaitedIn100us = timedKbits/2; // upper limit 20Mbit/s
  }

  public long kbitsPerSecond() {
    return timedKbits * 10000 / timeWaitedIn100us;
  }

  public long timeWaited() { return timeWaitedIn100us; }

  protected int overrun(int itemSize, int nItems, boolean wait) {
    if (itemSize > bufSize)
      throw new Exception("JavaInStream overrun: max itemSize exceeded");

    if (end - ptr != 0)
      System.arraycopy(b, ptr, b, 0, end - ptr);

    ptrOffset += ptr;
    end -= ptr;
    ptr = 0;

    while (end < itemSize) {
      int n = read(b, end, bufSize - end);
      end += n;
    }

    if (itemSize * nItems > end)
      nItems = end / itemSize;

    return nItems;
  }

  private int read(byte[] buf, int offset, int len) {
    try {
      long before = 0;
      if (timing)
        before = System.currentTimeMillis();

      int n = jis.read(buf, offset, len);
      if (n < 0) throw new EndOfStream();

      if (timing) {
        long after = System.currentTimeMillis();
        long newTimeWaited = (after - before) * 10;
        int newKbits = n * 8 / 1000;

        // limit rate to between 10kbit/s and 40Mbit/s

        if (newTimeWaited > newKbits*1000) newTimeWaited = newKbits*1000;
        if (newTimeWaited < newKbits/4)    newTimeWaited = newKbits/4;

        timeWaitedIn100us += newTimeWaited;
        timedKbits += newKbits;
      }

      return n;

    } catch (java.io.IOException e) {
      throw new IOException(e);
    }
  }

  private java.io.InputStream jis;
  private int ptrOffset;
  private int bufSize;

  boolean timing;
  long timeWaitedIn100us;
  long timedKbits;
}
