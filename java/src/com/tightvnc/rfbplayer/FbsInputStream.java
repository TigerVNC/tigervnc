//
//  Copyright (C) 2002 HorizonLive.com, Inc.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// FbsInputStream.java
//

import java.io.*;

class FbsInputStream extends InputStream {

  protected InputStream in;
  protected long startTime;
  protected long timeOffset;

  protected byte[] buffer;
  protected int bufferSize;
  protected int bufferPos;

  //
  // Constructors.
  //

  FbsInputStream() throws IOException {
    throw new IOException("FbsInputStream: no such constructor");
  }

  FbsInputStream(InputStream in) throws IOException
  {
    this.in = in;
    startTime = System.currentTimeMillis();
    timeOffset = 0;

    byte[] b = new byte[12];
    readFully(b);

    if (b[0] != 'F' || b[1] != 'B' || b[2] != 'S' || b[3] != ' ' ||
	b[4] != '0' || b[5] != '0' || b[6] != '1' || b[7] != '.' ||
	b[8] < '0' || b[8] > '9' || b[9] < '0' || b[9] > '9' ||
	b[10] < '0' || b[10] > '9' || b[11] != '\n') {
      throw new IOException("Incorrect protocol version");
    }

    buffer = null;
    bufferSize = 0;
    bufferPos = 0;
  }

  //
  // Basic methods overriding InputStream's methods.
  //

  public int read() throws IOException
  {
    while (bufferSize == 0) {
      if (!fillBuffer())
	return -1;
    }
    bufferSize--;
    return buffer[bufferPos++] & 0xFF;
  }

  public int available() throws IOException
  {
    // FIXME: This will work incorrectly if our caller will wait until
    // some amount of data is available when the buffer contains less
    // data than then that. Current implementation never reads more
    // data until the buffer is fully exhausted.
    return bufferSize;
  }

  public void close() throws IOException
  {
    in.close();
    in = null;
    startTime = 0;
    timeOffset = 0;

    buffer = null;
    bufferSize = 0;
    bufferPos = 0;
  }

  //
  // Methods providing additional functionality.
  //

  public int getPos()
  {
    return (int)(timeOffset / 1000);
  }

  public void resumeReading()
  {
    startTime = System.currentTimeMillis() - timeOffset;
  }

  //
  // Methods for internal use.
  //

  private boolean fillBuffer() throws IOException
  {
    bufferSize = (int)readUnsigned32();
    if (bufferSize >= 0) {
      int realSize = (bufferSize + 3) & 0xFFFFFFFC;
      buffer = new byte[realSize];
      readFully(buffer);
      bufferPos = 0;

      timeOffset = readUnsigned32();
    }

    if (bufferSize < 0 || timeOffset < 0) {
      buffer = null;
      bufferSize = 0;
      bufferPos = 0;
      return false;
    }

    while (true) {
      long timeDiff = startTime + timeOffset - System.currentTimeMillis();
      if (timeDiff <= 0) {
	break;
      }
      try {
	Thread.currentThread().sleep(timeDiff);
      } catch (InterruptedException e) {
      }
    }

    return true;
  }

  private long readUnsigned32() throws IOException
  {
    byte[] buf = new byte[4];
    if (!readFully(buf))
      return -1;

    return ((long)(buf[0] & 0xFF) << 24 |
	    (buf[1] & 0xFF) << 16 |
	    (buf[2] & 0xFF) << 8  |
	    (buf[3] & 0xFF));
  }

  private boolean readFully(byte[] b) throws IOException
  {
    int off = 0;
    int len = b.length;

    while (off != len) {
      int count = in.read(b, off, len - off);
      if (count < 0) {
	return false;
      }
      off += count;
    }

    return true;
  }
}

