/* Copyright 2009-2018 Pierre Ossman for Cendio AB
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

namespace rfb {
  class Congestion {
  public:
    Congestion();
    ~Congestion();

    // updatePosition() registers the current stream position and can
    // and should be called often.
    void updatePosition(unsigned pos);

    // sentPing() must be called when a marker is placed on the
    // outgoing stream. gotPong() must be called when the response for
    // such a marker is received.
    void sentPing();
    void gotPong();

    // isCongested() determines if the transport is currently congested
    // or if more data can be sent.
    bool isCongested();

    // getUncongestedETA() returns the number of milliseconds until the
    // transport is no longer congested. Returns 0 if there is no
    // congestion, and -1 if it is unknown when the transport will no
    // longer be congested.
    int getUncongestedETA();

    // getBandwidth() returns the current bandwidth estimation in bytes
    // per second.
    size_t getBandwidth();

    // debugTrace() writes the current congestion window, as well as the
    // congestion window of the underlying TCP layer, to the specified
    // file
    void debugTrace(const char* filename, int fd);

  protected:
    unsigned getExtraBuffer();
    unsigned getInFlight();

    void updateCongestion();

  private:
    unsigned lastPosition;
    unsigned extraBuffer;
    struct timeval lastUpdate;
    struct timeval lastSent;

    unsigned baseRTT;
    unsigned congWindow;
    bool inSlowStart;

    unsigned safeBaseRTT;

    struct RTTInfo {
      struct timeval tv;
      unsigned pos;
      unsigned extra;
      bool congested;
    };

    std::list<struct RTTInfo> pings;

    struct RTTInfo lastPong;
    struct timeval lastPongArrival;

    int measurements;
    struct timeval lastAdjustment;
    unsigned minRTT, minCongestedRTT;
  };
}

#endif
