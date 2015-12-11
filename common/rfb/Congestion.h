/* Copyright 2009-2015 Pierre Ossman for Cendio AB
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

#ifndef __RFB_CONGESTION_H__
#define __RFB_CONGESTION_H__

#include <list>

#include <rfb/Timer.h>

namespace rfb {
  class Congestion : public Timer::Callback {
  public:
    Congestion();
    ~Congestion();

    // sentPing() must be called when a marker is placed on the
    // outgoing stream, along with the current stream position.
    // gotPong() must be called when the response for such a marker
    // is received.
    void sentPing(int offset);
    void gotPong();

    // isCongested() determines if the transport is currently congested
    // or if more data can be sent. The curren stream position and how
    // long the transport has been idle must be specified.
    bool isCongested(int offset, unsigned idleTime);

  private:
    // Timer callbacks
    virtual bool handleTimeout(Timer* t);

  private:
    unsigned baseRTT;
    unsigned congWindow;
    unsigned ackedOffset, sentOffset;

    unsigned minRTT;
    bool seenCongestion;
    Timer congestionTimer;

    struct RTTInfo;
    std::list<struct RTTInfo> pings;
  };
}

#endif
