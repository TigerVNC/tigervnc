//
//  Copyright (C) 2002 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2008 Wimba, Inc.  All Rights Reserved.
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

package com.tightvnc.rfbplayer;

import java.io.*;
import java.util.*;

class FbsInputStream extends InputStream {

  protected InputStream in;
  protected long startTime;
  protected long timeOffset;
  protected long seekOffset;
  protected boolean farSeeking;
  protected boolean paused;
  protected boolean isQuitting = false;
  protected double playbackSpeed;

  protected byte[] buffer;
  protected int bufferSize;
  protected int bufferPos;

  /** The number of bytes to skip in the beginning of the next data block. */
  protected long nextBlockOffset;

  protected Observer obs;

  /**
   * Construct FbsInputStream object based on the given InputStream, positioned
   * at the very beginning of the corresponding FBS file. This constructor
   * reads and checks FBS file signature which would look like "FBS 001.000\n",
   * but any decimal number is allowed after the dot.
   *
   * @param in the InputStream object that will be used as a base for this new
   * FbsInputStream instance. It should be positioned at the very beginning of
   * the corresponding FBS file, so that first 12 bytes read from the stream
   * should form FBS file signature.
   * @throws java.io.IOException thrown on read error or on incorrect FBS file
   * signature.
   */
  FbsInputStream(InputStream in) throws IOException {
    this(in, 0, null, 0);

    byte[] b = new byte[12];
    readFully(b);

    if (b[0] != 'F' || b[1] != 'B' || b[2] != 'S' || b[3] != ' ' ||
        b[4] != '0' || b[5] != '0' || b[6] != '1' || b[7] != '.' ||
        b[8] < '0' || b[8] > '9' || b[9] < '0' || b[9] > '9' ||
        b[10] < '0' || b[10] > '9' || b[11] != '\n') {
      throw new IOException("Incorrect FBS file signature");
    }
  }

  /**
   * Construct FbsInputStream object based on the given byte array and
   * continued in the specified InputStream. Arbitrary position in the FBS file
   * is allowed.
   *
   * @param in
   *    the input stream for reading future data, after <code>buffer</code>
   *    will be exhausted. The stream should be positioned at any data block
   *    boundary (byte counter should follow in next four bytes).
   * @param timeOffset
   *    time position corresponding the the data block provided in
   *    <code>buffer</code>.
   * @param buffer
   *    the data block that will be treated as the beginning of this FBS data
   *    stream. This byte array is not copied into the new object so it should
   *    not be altered by the caller afterwards.
   * @param nextBlockOffset
   *    the number of bytes that should be skipped in first data block read
   *    from <code>in</code>.
   */
  FbsInputStream(InputStream in, long timeOffset, byte[] buffer,
                 long nextBlockOffset) {

    this.in = in;
    startTime = System.currentTimeMillis() - timeOffset;
    this.timeOffset = timeOffset;
    seekOffset = -1;
    farSeeking = false;
    paused = false;
    playbackSpeed = 1.0;

    this.buffer = buffer;
    bufferSize = (buffer != null) ? buffer.length : 0;
    bufferPos = 0;

    this.nextBlockOffset = nextBlockOffset;
  }

  // Force stream to finish any wait.
  public void quit() {
    isQuitting = true;
    synchronized(this) {
      notify();
    }
  }

  //
  // Basic methods overriding InputStream's methods.
  //
  public int read() throws IOException {
    while (bufferSize == 0) {
      if (!fillBuffer())
        return -1;
    }
    bufferSize--;
    return buffer[bufferPos++] & 0xFF;
  }

  public int available() throws IOException {
    // FIXME: This will work incorrectly if our caller will wait until
    // some amount of data is available when the buffer contains less
    // data than then that. Current implementation never reads more
    // data until the buffer is fully exhausted.
    return bufferSize;
  }

  public synchronized void close() throws IOException {
    if (in != null)
      in.close();
    in = null;
    startTime = -1;
    timeOffset = 0;
    seekOffset = -1;
    farSeeking = false;
    paused = false;
    playbackSpeed = 1.0;

    buffer = null;
    bufferSize = 0;
    bufferPos = 0;

    nextBlockOffset = 0;
    obs = null;
  }

  //
  // Methods providing additional functionality.
  //
  public synchronized long getTimeOffset() {
    long off = Math.max(seekOffset, timeOffset);
    return (long)(off * playbackSpeed);
  }

  public synchronized void setTimeOffset(long pos, boolean allowJump) {
    seekOffset = (long)(pos / playbackSpeed);
    if (allowJump) {
      long minJumpForwardOffset = timeOffset + (long)(10000 / playbackSpeed);
      if (seekOffset < timeOffset || seekOffset > minJumpForwardOffset) {
        farSeeking = true;
      }
    }
    notify();
  }

  public synchronized void setSpeed(double newSpeed) {
    long newOffset = (long)(timeOffset * playbackSpeed / newSpeed);
    startTime += timeOffset - newOffset;
    timeOffset = newOffset;
    if (isSeeking()) {
      seekOffset = (long)(seekOffset * playbackSpeed / newSpeed);
    }
    playbackSpeed = newSpeed;
  }

  public boolean isSeeking() {
    return (seekOffset >= 0);
  }

  public long getSeekOffset() {
    return (long)(seekOffset * playbackSpeed);
  }

  public boolean isPaused() {
    return paused;
  }

  public synchronized void pausePlayback() {
    paused = true;
    notify();
  }

  public synchronized void resumePlayback() {
    paused = false;
    startTime = System.currentTimeMillis() - timeOffset;
    notify();
  }

  public void addObserver(Observer target) {
    obs = target;
  }

  //
  // Methods for internal use.
  //
  private synchronized boolean fillBuffer() throws IOException {
    // The reading thread should be interrupted on far seeking.
    if (farSeeking)
      throw new EOFException("[JUMP]");

    // Just wait unless we are performing playback OR seeking.
    waitWhilePaused();

    if (!readDataBlock()) {
      return false;
    }

    if (seekOffset >= 0) {
      if (timeOffset >= seekOffset) {
        startTime = System.currentTimeMillis() - seekOffset;
        seekOffset = -1;
      } else {
        return true;
      }
    }

    while (!isQuitting) {
      long timeDiff = startTime + timeOffset - System.currentTimeMillis();
      if (timeDiff <= 0) {
        break;
      }
      try {
        wait(timeDiff);
      } catch (InterruptedException e) {
      }
      waitWhilePaused();
    }

    return true;
  }

  /**
   * Read FBS data block into the buffer.
   * If {@link #nextBlockOffset} is not zero, that number of bytes will be
   * skipped in the beginning of the data block.
   *
   * @return true on success, false if end of file was reached.
   * @throws java.io.IOException can be thrown while reading from the
   *   underlying input stream, or as a result of bad FBS file data.
   */
  private boolean readDataBlock() throws IOException {
    // Read byte counter, check for EOF condition.
    long readResult = readUnsigned32();
    if (readResult < 0) {
      return false;
    }

    bufferSize = (int)readResult;
    int alignedSize = (bufferSize + 3) & 0xFFFFFFFC;

    if (nextBlockOffset > 0) {
      in.skip(nextBlockOffset);
      bufferSize -= nextBlockOffset;
      alignedSize -= nextBlockOffset;
      nextBlockOffset = 0;
    }

    if (bufferSize >= 0) {
      buffer = new byte[alignedSize];
      readFully(buffer);
      bufferPos = 0;
      timeOffset = (long)(readUnsigned32() / playbackSpeed);
    }

    if (bufferSize < 0 || timeOffset < 0 || bufferPos >= bufferSize) {
      buffer = null;
      bufferSize = 0;
      bufferPos = 0;
      throw new IOException("Invalid FBS file data");
    }

    return true;
  }

  //
  // In paused mode, wait for external notification on this object.
  //
  private void waitWhilePaused() {
    while (paused && !isSeeking() && !isQuitting) {
      synchronized(this) {
        try {
          // Note: we call Observer.update(Observable,Object) method
          // directly instead of maintaining an Observable object.
          obs.update(null, null);
          wait();
        } catch (InterruptedException e) {
        }
      }
    }
  }

  private long readUnsigned32() throws IOException {
    byte[] buf = new byte[4];
    if (!readFully(buf))
      return -1;

    return ((long)(buf[0] & 0xFF) << 24 |
        (buf[1] & 0xFF) << 16 |
        (buf[2] & 0xFF) << 8 |
        (buf[3] & 0xFF));
  }

  private boolean readFully(byte[] b) throws IOException {
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

