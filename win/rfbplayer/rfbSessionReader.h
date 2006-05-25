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

// -=- rfbSessionReader.h

#include <rfb/Threading.h>

#include <rfbplayer/RfbProto.h>

using namespace rfb;

class rfbSessionReader : public Thread {
public:
  rfbSessionReader(RfbProto *_rfbSession) {
    rfbSession = _rfbSession;
    fStop = false;
    rfbSession->interruptFrameDelay();
  };
  
  ~rfbSessionReader() {
  }

  virtual Thread* join() {
    ((FbsInputStream*)(rfbSession->getInStream()))->resumePlayback();
    fStop = true;
    return Thread::join();
  }

  virtual void rfbSessionReader::run() {
    // Process the rfb messages
    while (!fStop) {
      try {
        rfbSession->processMsg();
      } catch (rdr::Exception e) {
        MessageBox(0, e.str(), "RFB Player", MB_OK | MB_ICONERROR);
        break;
      }
    }
  }

protected:
  bool fStop;
  RfbProto *rfbSession;
};