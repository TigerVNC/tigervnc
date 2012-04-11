/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2012 Brian P. Hinz
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

import com.tigervnc.network.*;
import java.nio.channels.Selector;
import java.nio.channels.SelectionKey;
import java.util.Set;
import java.util.Iterator;

public class FdInStream extends InStream {

  static final int DEFAULT_BUF_SIZE = 8192;
  static final int minBulkSize = 1024;

  public FdInStream(FileDescriptor fd_, int timeoutms_, int bufSize_,
                    boolean closeWhenDone_) 
  {
    fd = fd_; closeWhenDone = closeWhenDone_;
    timeoutms = timeoutms_; blockCallback = null;
    timing = false; timeWaitedIn100us = 5; timedKbits = 0;
    bufSize = ((bufSize_ > 0) ? bufSize_ : DEFAULT_BUF_SIZE);
    b = new byte[bufSize];
    ptr = end = offset = 0;
  }

  public FdInStream(FileDescriptor fd_) { this(fd_, -1, 0, false); }

  public FdInStream(FileDescriptor fd_, FdInStreamBlockCallback blockCallback_, 
                    int bufSize_)
  {
    fd = fd_; timeoutms = 0; blockCallback = blockCallback_;
    timing = false; timeWaitedIn100us = 5; timedKbits = 0;
    bufSize = ((bufSize_ > 0) ? bufSize_ : DEFAULT_BUF_SIZE);
    b = new byte[bufSize];
    ptr = end = offset = 0;
  }

  public FdInStream(FileDescriptor fd_, 
                    FdInStreamBlockCallback blockCallback_) {
    this(fd_, blockCallback_, 0);
  }

  public final void readBytes(byte[] data, int dataPtr, int length) {
    if (length < minBulkSize) {
      super.readBytes(data, dataPtr, length);
      return;
    }

    int n = end - ptr;
    if (n > length) n = length;

    System.arraycopy(b, ptr, data, dataPtr, n);
    dataPtr += n;
    length -= n;
    ptr += n;

    while (length > 0) {
      n = readWithTimeoutOrCallback(data, dataPtr, length);
      dataPtr += n;
      length -= n;
      offset += n;
    }
  }

  public void setTimeout(int timeoutms_) {
    timeoutms = timeoutms_;
  }

  public void setBlockCallback(FdInStreamBlockCallback blockCallback_)
  {
    blockCallback = blockCallback_;
    timeoutms = 0;
  }

  public final int pos() { return offset + ptr; }

  public final void startTiming() {
    timing = true;

    // Carry over up to 1s worth of previous rate for smoothing.

    if (timeWaitedIn100us > 10000) {
      timedKbits = timedKbits * 10000 / timeWaitedIn100us;
      timeWaitedIn100us = 10000;
    }
  }

  public final void stopTiming() {
    timing = false; 
    if (timeWaitedIn100us < timedKbits/2)
      timeWaitedIn100us = timedKbits/2; // upper limit 20Mbit/s
  }

  public final long kbitsPerSecond() {
    return timedKbits * 10000 / timeWaitedIn100us;
  }

  public final long timeWaited() { return timeWaitedIn100us; }

  protected int overrun(int itemSize, int nItems, boolean wait) 
  {
    if (itemSize > bufSize)
      throw new Exception("FdInStream overrun: max itemSize exceeded");

    if (end - ptr != 0)
      System.arraycopy(b, ptr, b, 0, end - ptr);

    offset += ptr;
    end -= ptr;
    ptr = 0;

    int bytes_to_read;
    while (end < itemSize) {
      bytes_to_read = bufSize - end;
      if (!timing) {
        // When not timing, we must be careful not to read too much
        // extra data into the buffer. Otherwise, the line speed
        // estimation might stay at zero for a long time: All reads
        // during timing=1 can be satisfied without calling
        // readWithTimeoutOrCallback. However, reading only 1 or 2 bytes
        // bytes is ineffecient.
        bytes_to_read = Math.min(bytes_to_read, Math.max(itemSize*nItems, 8));
      }
      int n = readWithTimeoutOrCallback(b, end, bytes_to_read, wait);
      if (n == 0) return 0;
      end += n;
    }

    if (itemSize * nItems > end - ptr)
      nItems = (end - ptr) / itemSize;

    return nItems;
  }

  protected int readWithTimeoutOrCallback(byte[] buf, int bufPtr, int len, boolean wait) {
    long before = 0;
    if (timing)
      before = System.nanoTime();

    int n;
    while (true) {
      do {
        Integer tv;
    
        if (!wait) {
          tv = new Integer(0);
        } else if (timeoutms != -1) {
          tv = new Integer(timeoutms);
        } else {
          tv = null;
        }

        try {
          n = fd.select(SelectionKey.OP_READ, tv);
        } catch (Exception e) {
          throw new SystemException("select:"+e.toString());
        }
      } while (n < 0);
        
          
      if (n > 0) break;
      if (!wait) return 0;
      if (blockCallback == null) throw new TimedOut();
    
      blockCallback.blockCallback();
    }

    try {
      n = fd.read(buf, bufPtr, len);
    } catch (Exception e) {
      throw new SystemException("read:"+e.toString());
    }

    if (n == 0) throw new EndOfStream();

    if (timing) {
      long after = System.nanoTime();
      long newTimeWaited = (after - before) / 100000;
      int newKbits = n * 8 / 1000;

      // limit rate to between 10kbit/s and 40Mbit/s

      if (newTimeWaited > newKbits*1000) {
        newTimeWaited = newKbits*1000;
      } else if (newTimeWaited < newKbits/4) {
        newTimeWaited = newKbits/4;
      }

      timeWaitedIn100us += newTimeWaited;
      timedKbits += newKbits;
    }

    return n;
  }

  private int readWithTimeoutOrCallback(byte[] buf, int bufPtr, int len) { 
    return readWithTimeoutOrCallback(buf, bufPtr, len, true);
  }

  public FileDescriptor getFd() {
    return fd;
  }

  public void setFd(FileDescriptor fd_) {
    fd = fd_;
  }

  private FileDescriptor fd;
  boolean closeWhenDone;
  protected int timeoutms;
  private FdInStreamBlockCallback blockCallback;
  private int offset;
  private int bufSize;

  protected boolean timing;
  protected long timeWaitedIn100us;
  protected long timedKbits;
}
