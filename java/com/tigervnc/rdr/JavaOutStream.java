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
// A JavaOutStream writes to a java.io.OutputStream
//

package com.tigervnc.rdr;

public class JavaOutStream extends OutStream {

  static final int defaultBufSize = 16384;
  static final int minBulkSize = 1024;

  public JavaOutStream(java.io.OutputStream jos_, int bufSize_) {
    jos = jos_;
    bufSize = bufSize_;
    b = new byte[bufSize];
    ptr = offset = start = 0;
    end = start + bufSize;
  }

  public JavaOutStream(java.io.OutputStream jos) { this(jos, defaultBufSize); }

  public int length() 
  { 
    return offset + ptr - start; 
  }

  public void flush() 
  {
    int sentUpTo = start;
    while (sentUpTo < ptr) {
      try {
        jos.write(b, sentUpTo, ptr - sentUpTo);
        sentUpTo += ptr - sentUpTo;
        offset += ptr - sentUpTo;
      } catch (java.io.IOException e) {
        throw new IOException(e);
      }
    }
    ptr = start;
  }

  protected int overrun(int itemSize, int nItems) 
  {
    if (itemSize > bufSize)
      throw new Exception("JavaOutStream overrun: max itemSize exceeded");

    flush();

    if (itemSize * nItems > end - ptr)
      nItems = (end - ptr) / itemSize;

    return nItems;
  }

  private java.io.OutputStream jos;
  private int start;
  private int offset;
  private int bufSize;
}
