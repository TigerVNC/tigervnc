/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2012 TigerVnc Team
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

package com.tigervnc.rdr;

import com.tigervnc.network.*;
import java.nio.channels.SelectionKey;

public class FdOutStream extends OutStream {

  static final int defaultBufSize = 16384;
  static final int minBulkSize = 1024;

  public FdOutStream(FileDescriptor fd_, boolean blocking_, int timeoutms_, int bufSize_)
  {
    fd = fd_;
    blocking = blocking_;
    timeoutms = timeoutms_;
    bufSize = bufSize_;
    b = new byte[bufSize];
    offset = 0;
    ptr = sentUpTo = start = 0;
    end = start + bufSize;
  }

  public FdOutStream(FileDescriptor fd_) { this(fd_, false, 0, defaultBufSize); }

  public void setTimeout(int timeoutms_) {
    timeoutms = timeoutms_;
  }

  public void setBlocking(boolean blocking_) {
    blocking = blocking_;
  }

  public int length() 
  { 
    return offset + ptr - sentUpTo; 
  }

  public void flush() 
  {
    int timeoutms_;

    if (blocking)
      timeoutms_ = timeoutms;
    else
      timeoutms_ = 0;

    while (sentUpTo < ptr) {
      int n = writeWithTimeout(b, sentUpTo, ptr - sentUpTo, timeoutms_);

      // Timeout?
      if (n == 0) {
        // If non-blocking then we're done here
        if (!blocking)
          break;

        // Otherwise try blocking (with possible timeout)
        if ((timeoutms_ == 0) && (timeoutms != 0)) {
          timeoutms_ = timeoutms;
          break;
        }

        // Proper timeout
        throw new TimedOut();
      }

      sentUpTo += n;
      offset += n;
    }

    // Managed to flush everything?
    if (sentUpTo == ptr)
      ptr = sentUpTo = start;
  }

  private int writeWithTimeout(byte[] data, int dataPtr, int length, int timeoutms)
  {
    int timeout;
    int n;

    do {
    
      if (timeoutms != -1) {
        timeout = timeoutms;
      } else {
        timeout = 0;
      }

      try {
        n = fd.select(SelectionKey.OP_WRITE, timeout);
      } catch (java.lang.Exception e) {
        System.out.println(e.toString());
        throw new Exception(e.toString());
      }
          
    } while (n < 0);

    try {
      n = fd.write(data, dataPtr, length);
    } catch (java.lang.Exception e) {
      throw new Exception(e.toString());
    }
    
    return n;
  }

  protected int overrun(int itemSize, int nItems) 
  {
    if (itemSize > bufSize)
      throw new Exception("FdOutStream overrun: max itemSize exceeded");

    // First try to get rid of the data we have
    flush();

    // Still not enough space?
    if (itemSize > end - ptr) {
      // Can we shuffle things around?
      // (don't do this if it gains us less than 25%)
      if ((sentUpTo - start > bufSize / 4) &&
          (itemSize < bufSize - (ptr - sentUpTo))) {
        System.arraycopy(b, ptr, b, start, ptr - sentUpTo);
        ptr = start + (ptr - sentUpTo);
        sentUpTo = start;
      } else {
        // Have to get rid of more data, so turn off non-blocking
        // for a bit...
        boolean realBlocking;

        realBlocking = blocking;
        blocking = true;
        flush();
        blocking = realBlocking;
      }
    }

    // Can we fit all the items asked for?
    if (itemSize * nItems > end - ptr)
      nItems = (end - ptr) / itemSize;

    return nItems;
  }

  public FileDescriptor getFd() {
    return fd;
  }

  public void setFd(FileDescriptor fd_) {
    fd = fd_;
  }

  protected FileDescriptor fd;
  protected boolean blocking;
  protected int timeoutms;
  protected int start;
  protected int sentUpTo;
  protected int offset;
  protected int bufSize;
}
