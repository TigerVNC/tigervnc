/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- FbsInputStream class

#include <windows.h>

#include <rfb/Exception.h>

#include <rfbplayer/FbsInputStream.h>

FbsInputStream::FbsInputStream(char* FileName) {
  bufferSize = 0;
  ptr = end = start = NULL;

  timeOffset = 0;
  seekOffset = -1;
  startTime  = GetTickCount();

  playbackSpeed = 1.0;
  seekBackwards = false;
  paused        = false;

  interruptDelay = false;

  fbsFile = fopen(FileName, "rb");
  if (fbsFile == NULL) {
    char *msg = new char[12 + sizeof(FileName)];
    strcpy(msg, "Can't open ");
    strcat(msg, FileName);
    throw rfb::Exception(msg);
  }

  U8 b[12];
  readNByte(b, 12);

  if (b[0] != 'F' || b[1] != 'B' || b[2] != 'S' || b[3] != ' ' ||
      b[4] != '0' || b[5] != '0' || b[6] != '1' || b[7] != '.' ||
	    b[8]  < '0' || b[8]  > '9' || b[9]  < '0' || b[9] > '9'  ||
	    b[10] < '0' || b[10] > '9' || b[11] != '\n') {
    throw rfb::Exception("Incorrect protocol version");
  }
}

FbsInputStream::~FbsInputStream() {
  if (start != NULL) 
    delete [] start;
  ptr = end = start = NULL;
  fclose(fbsFile);
}

int FbsInputStream::pos() {
  return ptr - start;
}

//
// Close FbsInputStream and free data buffer
//

void FbsInputStream::close() {
  fclose(fbsFile);

  startTime = -1;
  timeOffset = 0;
  seekOffset = -1;
  seekBackwards = false;
  paused = false;
  playbackSpeed = 1.0;

  if (start != NULL)
    delete [] start;
  ptr = end = start = NULL;
  bufferSize = 0;
}

//
// Fill data buffer from the session file (InStream::overrun() override)
//

int FbsInputStream::overrun(int itemSize, int nItems, bool wait=true) {
  // Just wait unless we are performing playback OR seeking.
  waitWhilePaused(); 
  
  // Perform backwardSeek (throws the special exception)
  if (seekBackwards) {
    throw rfb::Exception("[REWIND]");
  }

  // Save a tail of data
  U8 *tmp;
  int n = end - ptr;
  if (n) {
    tmp = new U8[n];
    memmove(tmp, ptr, n);
  }

  bufferSize = (int)readUnsigned32();
  if (bufferSize >= 0) {
    if (start != NULL)
      delete [] start;
    int realSize = (bufferSize + 3) & 0xFFFFFFFC; // padding to multiple of 32-bits
    ptr = start = new U8[realSize + n];
    end = ptr + bufferSize + n;
    if (n) {
      memmove(start, tmp, n);
      delete [] tmp;
    }
    readNByte(start + n, realSize);
    timeOffset = (long)(readUnsigned32() / playbackSpeed);

    if (itemSize * nItems > bufferSize)
      nItems = bufferSize / itemSize;
  }

  if (bufferSize < 0 || timeOffset < 0) {
    if (start != NULL)
      delete [] start;
    ptr = end = start = NULL;
    bufferSize = 0;
    return 0;
  }

  if (seekOffset >= 0) {
    if (timeOffset >= seekOffset) {
      startTime = GetTickCount() - seekOffset;
      seekOffset = -1;
    } else {
	    return nItems;
    }
  }

  while (!interruptDelay) {
    long timeDiff = startTime + timeOffset - GetTickCount();
    if (timeDiff <= 0) {
	    break;
    }
    Sleep(min(20, timeDiff));
    waitWhilePaused();
  }
  interruptDelay = false;

  return nItems;
}

int FbsInputStream::readUnsigned32() {
  U8 buf[4];
  if (!readNByte(buf, 4))
    return -1;

  return ((long)(buf[0] & 0xFF) << 24 |
    	    (buf[1] & 0xFF) << 16 |
	        (buf[2] & 0xFF) << 8  |
	        (buf[3] & 0xFF)); 
}

//
// Read n-bytes from the session file
//

bool FbsInputStream::readNByte(U8 b[], int n) {
  int off = 0;
  
  while (off != n) {
    int count = fread(b, 1, n - off, fbsFile);
    if (count < n) {
      if (ferror(fbsFile)) 
        throw rfb::Exception("Read error from session file");
      if (feof(fbsFile))
        throw rfb::Exception("[End Of File]");
    }
    off += count;
  }
  return true;
}

void FbsInputStream::waitWhilePaused() {
  while (paused && !isSeeking()) {
    // A small delay helps to decrease the cpu usage
    Sleep(20);
  }
}

void FbsInputStream::interruptFrameDelay() {
  interruptDelay = true;
}

//
// Methods providing additional functionality.
//

long FbsInputStream::getTimeOffset() {
  //long off = max(seekOffset, timeOffset);
  return (long)(timeOffset * playbackSpeed);
}

void FbsInputStream::setTimeOffset(long pos) {
  seekOffset = (long)(pos / playbackSpeed);
  if (seekOffset < timeOffset) {
    seekBackwards = true;
  }
}

void FbsInputStream::setSpeed(double newSpeed) {
  long newOffset = (long)(timeOffset * playbackSpeed / newSpeed);
  startTime += timeOffset - newOffset;
  timeOffset = newOffset;
  if (isSeeking()) {
    seekOffset = (long)(seekOffset * playbackSpeed / newSpeed);
  }
  playbackSpeed = newSpeed;
}

double FbsInputStream::getSpeed() {
  return playbackSpeed;
}

bool FbsInputStream::isSeeking() {
  return (seekOffset >= 0);
}

long FbsInputStream::getSeekOffset() {
  return (long)(seekOffset * playbackSpeed);
}

bool FbsInputStream::isPaused() {
  return paused;
}

void FbsInputStream::pausePlayback() {
  paused = true;
}

void FbsInputStream::resumePlayback() {
  paused = false;
  startTime = GetTickCount() - timeOffset;
}
