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

// -=- FbsInputStream.h

#include <rdr/InStream.h>

using namespace rdr;

class FbsInputStream : public InStream {
  public:
    FbsInputStream(char *FileName);
    ~FbsInputStream();

    // Methods are used to contol the Rfb Stream
    long getTimeOffset();
    void setTimeOffset(long pos);
    double getSpeed();
    void setSpeed(double newSpeed);
    bool isSeeking();
    long getSeekOffset();
    bool isPaused();
    void pausePlayback();
    void resumePlayback();
    void interruptFrameDelay();
    void close();
    int  pos();

  private:
    U8 *start;
    int bufferSize;
    long startTime;
    long timeOffset;
    long seekOffset;
    double playbackSpeed;
    bool seekBackwards;
    bool paused;
    bool interruptDelay;

    FILE  *fbsFile;

    // overrun() - overrides InStream::overrun().
    // It is implemented to fill the data buffer from the session file.
    // It ensures there are at least itemSize bytes of buffer data.  Returns
    // the number of items in the buffer (up to a maximum of nItems).  itemSize
    // is supposed to be "small" (a few bytes).

    int overrun(int itemSize, int nItems, bool wait);

    int readUnsigned32();
    bool readNByte(U8 *b, int n);
    void waitWhilePaused();
};
