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

/*
 * This code implements congestion control in the same way as TCP in
 * order to avoid excessive latency in the transport. This is needed
 * because "buffer bloat" is unfortunately still a very real problem.
 *
 * The basic principle is TCP Congestion Control (RFC 5618), with the
 * addition of using the TCP Vegas algorithm. The reason we use Vegas
 * is that we run on top of a reliable transport so we need a latency
 * based algorithm rather than a loss based one.
 */

#include <sys/time.h>

#include <rfb/Congestion.h>
#include <rfb/util.h>

// Debug output on what the congestion control is up to
#undef CONGESTION_DEBUG

using namespace rfb;

// This window should get us going fairly fast on a decent bandwidth network.
// If it's too high, it will rapidly be reduced and stay low.
static const unsigned INITIAL_WINDOW = 16384;

// TCP's minimal window is 3*MSS. But since we don't know the MSS, we
// make a guess at 4 KiB (it's probably a bit higher).
static const unsigned MINIMUM_WINDOW = 4096;

// The current default maximum window for Linux (4 MiB). Should be a good
// limit for now...
static const unsigned MAXIMUM_WINDOW = 4194304;

struct Congestion::RTTInfo {
  struct timeval tv;
  int offset;
  unsigned inFlight;
};

Congestion::Congestion() :
    baseRTT(-1), congWindow(INITIAL_WINDOW),
    ackedOffset(0), sentOffset(0),
    minRTT(-1), seenCongestion(false),
    congestionTimer(this)
{
}

Congestion::~Congestion()
{
}


void Congestion::sentPing(int offset)
{
  struct RTTInfo rttInfo;

  if (ackedOffset == 0)
    ackedOffset = offset;

  memset(&rttInfo, 0, sizeof(struct RTTInfo));

  gettimeofday(&rttInfo.tv, NULL);
  rttInfo.offset = offset;
  rttInfo.inFlight = rttInfo.offset - ackedOffset;

  pings.push_back(rttInfo);

  sentOffset = offset;

  // Let some data flow before we adjust the settings
  if (!congestionTimer.isStarted())
    congestionTimer.start(__rfbmin(baseRTT * 2, 100));
}

void Congestion::gotPong()
{
  struct RTTInfo rttInfo;
  unsigned rtt, delay;

  if (pings.empty())
    return;

  rttInfo = pings.front();
  pings.pop_front();

  rtt = msSince(&rttInfo.tv);
  if (rtt < 1)
    rtt = 1;

  ackedOffset = rttInfo.offset;

  // Try to estimate wire latency by tracking lowest seen latency
  if (rtt < baseRTT)
    baseRTT = rtt;

  if (rttInfo.inFlight > congWindow) {
    seenCongestion = true;

    // Estimate added delay because of overtaxed buffers
    delay = (rttInfo.inFlight - congWindow) * baseRTT / congWindow;

    if (delay < rtt)
      rtt -= delay;
    else
      rtt = 1;

    // If we underestimate the congestion window, then we'll get a latency
    // that's less than the wire latency, which will confuse other portions
    // of the code.
    if (rtt < baseRTT)
      rtt = baseRTT;
  }

  // We only keep track of the minimum latency seen (for a given interval)
  // on the basis that we want to avoid continuous buffer issue, but don't
  // mind (or even approve of) bursts.
  if (rtt < minRTT)
    minRTT = rtt;
}

bool Congestion::isCongested(int offset, unsigned idleTime)
{
  // Idle for too long? (and no data on the wire)
  //
  // FIXME: This should really just be one baseRTT, but we're getting
  //        problems with triggering the idle timeout on each update.
  //        Maybe we need to use a moving average for the wire latency
  //        instead of baseRTT.
  if ((sentOffset == ackedOffset) && (idleTime > 2 * baseRTT)) {

#ifdef CONGESTION_DEBUG
    if (congWindow > INITIAL_WINDOW)
      fprintf(stderr, "Reverting to initial window (%d KiB) after %d ms\n",
              INITIAL_WINDOW / 1024, sock->outStream().getIdleTime());
#endif

    // Close congestion window and allow a transfer
    // FIXME: Reset baseRTT like Linux Vegas?
    congWindow = __rfbmin(INITIAL_WINDOW, congWindow);

    return false;
  }

  // FIXME: Should we compensate for non-update data?
  //        (i.e. use sentOffset instead of offset)
  if ((offset - ackedOffset) < congWindow)
    return false;

  // If we just have one outstanding "ping", that means the client has
  // started receiving our update. In order to not regress compared to
  // before we had congestion avoidance, we allow another update here.
  // This could further clog up the tubes, but congestion control isn't
  // really working properly right now anyway as the wire would otherwise
  // be idle for at least RTT/2.
  if (pings.size() == 1)
    return false;

  return true;
}

bool Congestion::handleTimeout(Timer* t)
{
  unsigned diff;

  if (!seenCongestion)
    return false;

  // The goal is to have a slightly too large congestion window since
  // a "perfect" one cannot be distinguished from a too small one. This
  // translates to a goal of a few extra milliseconds of delay.

  diff = minRTT - baseRTT;

  if (diff > __rfbmin(100, baseRTT)) {
    // Way too fast
    congWindow = congWindow * baseRTT / minRTT;
  } else if (diff > __rfbmin(50, baseRTT/2)) {
    // Slightly too fast
    congWindow -= 4096;
  } else if (diff < 5) {
    // Way too slow
    congWindow += 8192;
  } else if (diff < 25) {
    // Too slow
    congWindow += 4096;
  }

  if (congWindow < MINIMUM_WINDOW)
    congWindow = MINIMUM_WINDOW;
  if (congWindow > MAXIMUM_WINDOW)
    congWindow = MAXIMUM_WINDOW;

#ifdef CONGESTION_DEBUG
  fprintf(stderr, "RTT: %d ms (%d ms), Window: %d KiB, Bandwidth: %g Mbps\n",
          minRTT, baseRTT, congWindow / 1024,
          congWindow * 8.0 / baseRTT / 1000.0);
#endif

  minRTT = -1;
  seenCongestion = false;

  return false;
}

