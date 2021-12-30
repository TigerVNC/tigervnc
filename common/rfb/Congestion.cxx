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

/*
 * This code implements congestion control in the same way as TCP in
 * order to avoid excessive latency in the transport. This is needed
 * because "buffer bloat" is unfortunately still a very real problem.
 *
 * The basic principle is TCP Congestion Control (RFC 5618), with the
 * addition of using the TCP Vegas algorithm. The reason we use Vegas
 * is that we run on top of a reliable transport so we need a latency
 * based algorithm rather than a loss based one. There is also a lot of
 * interpolation of values. This is because we have rather horrible
 * granularity in our measurements.
 *
 * We use a simplistic form of slow start in order to ramp up quickly
 * from an idle state. We do not have any persistent threshold though
 * as we have too much noise for it to be reliable.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <sys/time.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <linux/sockios.h>
#endif

#include <rfb/Congestion.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>

// Debug output on what the congestion control is up to
#undef CONGESTION_DEBUG

// Dump socket congestion window debug trace to disk
#undef CONGESTION_TRACE

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

// Compare position even when wrapped around
static inline bool isAfter(unsigned a, unsigned b) {
  return a != b && a - b <= UINT_MAX / 2;
}

static LogWriter vlog("Congestion");

Congestion::Congestion() :
    lastPosition(0), extraBuffer(0),
    baseRTT(-1), congWindow(INITIAL_WINDOW), inSlowStart(true),
    safeBaseRTT(-1), measurements(0), minRTT(-1), minCongestedRTT(-1)
{
  gettimeofday(&lastUpdate, NULL);
  gettimeofday(&lastSent, NULL);
  memset(&lastPong, 0, sizeof(lastPong));
  gettimeofday(&lastPongArrival, NULL);
  gettimeofday(&lastAdjustment, NULL);
}

Congestion::~Congestion()
{
}

void Congestion::updatePosition(unsigned pos)
{
  struct timeval now;
  unsigned delta, consumed;

  gettimeofday(&now, NULL);

  delta = pos - lastPosition;
  if ((delta > 0) || (extraBuffer > 0))
    lastSent = now;

  // Idle for too long?
  // We use a very crude RTO calculation in order to keep things simple
  // FIXME: should implement RFC 2861
  if (msBetween(&lastSent, &now) > __rfbmax(baseRTT*2, 100)) {

#ifdef CONGESTION_DEBUG
    vlog.debug("Connection idle for %d ms, resetting congestion control",
               msBetween(&lastSent, &now));
#endif

    // Close congestion window and redo wire latency measurement
    congWindow = __rfbmin(INITIAL_WINDOW, congWindow);
    baseRTT = -1;
    measurements = 0;
    gettimeofday(&lastAdjustment, NULL);
    minRTT = minCongestedRTT = -1;
    inSlowStart = true;
  }

  // Commonly we will be in a state of overbuffering. We need to
  // estimate the extra delay that causes so we can separate it from
  // the delay caused by an incorrect congestion window.
  // (we cannot do this until we have a RTT measurement though)
  if (baseRTT != (unsigned)-1) {
    extraBuffer += delta;
    consumed = msBetween(&lastUpdate, &now) * congWindow / baseRTT;
    if (extraBuffer < consumed)
      extraBuffer = 0;
    else
      extraBuffer -= consumed;
  }

  lastPosition = pos;
  lastUpdate = now;
}

void Congestion::sentPing()
{
  struct RTTInfo rttInfo;

  memset(&rttInfo, 0, sizeof(struct RTTInfo));

  gettimeofday(&rttInfo.tv, NULL);
  rttInfo.pos = lastPosition;
  rttInfo.extra = getExtraBuffer();
  rttInfo.congested = isCongested();

  pings.push_back(rttInfo);
}

void Congestion::gotPong()
{
  struct timeval now;
  struct RTTInfo rttInfo;
  unsigned rtt, delay;

  if (pings.empty())
    return;

  gettimeofday(&now, NULL);

  rttInfo = pings.front();
  pings.pop_front();

  lastPong = rttInfo;
  lastPongArrival = now;

  rtt = msBetween(&rttInfo.tv, &now);
  if (rtt < 1)
    rtt = 1;

  // Try to estimate wire latency by tracking lowest seen latency
  if (rtt < baseRTT)
    safeBaseRTT = baseRTT = rtt;

  // Pings sent before the last adjustment aren't interesting as they
  // aren't a measurement of the current congestion window
  if (isBefore(&rttInfo.tv, &lastAdjustment))
    return;

  // Estimate added delay because of overtaxed buffers (see above)
  delay = rttInfo.extra * baseRTT / congWindow;
  if (delay < rtt)
    rtt -= delay;
  else
    rtt = 1;

  // A latency less than the wire latency means that we've
  // understimated the congestion window. We can't really determine
  // how much, so pretend that we got no buffer latency at all.
  if (rtt < baseRTT)
    rtt = baseRTT;

  // Record the minimum seen delay (hopefully ignores jitter) and let
  // the congestion control do its thing.
  //
  // Note: We are delay based rather than loss based, which means we
  //       need to look at pongs even if they weren't limited by the
  //       current window ("congested"). Otherwise we will fail to
  //       detect increasing congestion until the application exceeds
  //       the congestion window.
  if (rtt < minRTT)
    minRTT = rtt;
  if (rttInfo.congested) {
    if (rtt < minCongestedRTT)
      minCongestedRTT = rtt;
  }

  measurements++;
  updateCongestion();
}

bool Congestion::isCongested()
{
  if (getInFlight() < congWindow)
    return false;

  return true;
}

int Congestion::getUncongestedETA()
{
  unsigned targetAcked;

  const struct RTTInfo* prevPing;
  unsigned eta, elapsed;
  unsigned etaNext, delay;

  std::list<struct RTTInfo>::const_iterator iter;

  targetAcked = lastPosition - congWindow;

  // Simple case?
  if (isAfter(lastPong.pos, targetAcked))
    return 0;

  // No measurements yet?
  if (baseRTT == (unsigned)-1)
    return -1;

  prevPing = &lastPong;
  eta = 0;
  elapsed = msSince(&lastPongArrival);

  // Walk the ping queue and figure out which one we are waiting for to
  // get to an uncongested state

  for (iter = pings.begin(); ;++iter) {
    struct RTTInfo curPing;

    // If we aren't waiting for a pong that will clear the congested
    // state then we have to estimate the final bit by pretending that
    // we had a ping just after the last position update.
    if (iter == pings.end()) {
      curPing.tv = lastUpdate;
      curPing.pos = lastPosition;
      curPing.extra = extraBuffer;
    } else {
      curPing = *iter;
    }

    etaNext = msBetween(&prevPing->tv, &curPing.tv);
    // Compensate for buffering delays
    delay = curPing.extra * baseRTT / congWindow;
    etaNext += delay;
    delay = prevPing->extra * baseRTT / congWindow;
    if (delay >= etaNext)
      etaNext = 0;
    else
      etaNext -= delay;

    // Found it?
    if (isAfter(curPing.pos, targetAcked)) {
      eta += etaNext * (curPing.pos - targetAcked) / (curPing.pos - prevPing->pos);
      if (elapsed > eta)
        return 0;
      else
        return eta - elapsed;
    }

    assert(iter != pings.end());

    eta += etaNext;
    prevPing = &*iter;
  }
}

size_t Congestion::getBandwidth()
{
  size_t bandwidth;

  // No measurements yet? Guess RTT of 60 ms
  if (safeBaseRTT == (unsigned)-1)
    bandwidth = congWindow * 1000 / 60;
  else
    bandwidth = congWindow * 1000 / safeBaseRTT;

  // We're still probing so guess actual bandwidth is halfway between
  // the current guess and the next one (slow start doubles each time)
  if (inSlowStart)
    bandwidth = bandwidth + bandwidth / 2;

  return bandwidth;
}

void Congestion::debugTrace(const char* filename, int fd)
{
#ifdef CONGESTION_TRACE
#ifdef __linux__
  FILE *f;
  f = fopen(filename, "ab");
  if (f != NULL) {
    struct tcp_info info;
    int buffered;
    socklen_t len;
    len = sizeof(info);
    if ((getsockopt(fd, IPPROTO_TCP,
                    TCP_INFO, &info, &len) == 0) &&
        (ioctl(fd, SIOCOUTQ, &buffered) == 0)) {
      struct timeval now;
      gettimeofday(&now, NULL);
      fprintf(f, "%u.%06u,%u,%u,%u,%u\n",
              (unsigned)now.tv_sec, (unsigned)now.tv_usec,
              congWindow, info.tcpi_snd_cwnd * info.tcpi_snd_mss,
              getInFlight(), buffered);
    }
    fclose(f);
  }
#endif
#endif
}

unsigned Congestion::getExtraBuffer()
{
  unsigned elapsed;
  unsigned consumed;

  if (baseRTT == (unsigned)-1)
    return 0;

  elapsed = msSince(&lastUpdate);
  consumed = elapsed * congWindow / baseRTT;

  if (consumed >= extraBuffer)
    return 0;
  else
    return extraBuffer - consumed;
}

unsigned Congestion::getInFlight()
{
  struct RTTInfo nextPong;
  unsigned etaNext, delay, elapsed, acked;

  // Simple case?
  if (lastPosition == lastPong.pos)
    return 0;

  // No measurements yet?
  if (baseRTT == (unsigned)-1) {
    if (!pings.empty())
      return lastPosition - pings.front().pos;
    return 0;
  }

  // If we aren't waiting for any pong then we have to estimate things
  // by pretending that we had a ping just after the last position
  // update.
  if (pings.empty()) {
    nextPong.tv = lastUpdate;
    nextPong.pos = lastPosition;
    nextPong.extra = extraBuffer;
  } else {
    nextPong = pings.front();
  }

  // First we need to estimate how many bytes have made it through
  // completely. Look at the next ping that should arrive and figure
  // out how far behind it should be and interpolate the positions.

  etaNext = msBetween(&lastPong.tv, &nextPong.tv);
  // Compensate for buffering delays
  delay = nextPong.extra * baseRTT / congWindow;
  etaNext += delay;
  delay = lastPong.extra * baseRTT / congWindow;
  if (delay >= etaNext)
    etaNext = 0;
  else
    etaNext -= delay;

  elapsed = msSince(&lastPongArrival);

  // The pong should be here any second. Be optimistic and assume
  // we can already use its value.
  if (etaNext <= elapsed)
    acked = nextPong.pos;
  else {
    acked = lastPong.pos;
    acked += (nextPong.pos - lastPong.pos) * elapsed / etaNext;
  }

  return lastPosition - acked;
}

void Congestion::updateCongestion()
{
  unsigned diff;

  // We want at least three measurements to avoid noise
  if (measurements < 3)
    return;

  assert(minRTT >= baseRTT);
  assert(minCongestedRTT >= baseRTT);

  // The goal is to have a slightly too large congestion window since
  // a "perfect" one cannot be distinguished from a too small one. This
  // translates to a goal of a few extra milliseconds of delay.

  diff = minRTT - baseRTT;

  if (diff > __rfbmax(100, baseRTT/2)) {
    // We have no way of detecting loss, so assume massive latency
    // spike means packet loss. Adjust the window and go directly
    // to congestion avoidance.
#ifdef CONGESTION_DEBUG
    vlog.debug("Latency spike! Backing off...");
#endif
    congWindow = congWindow * baseRTT / minRTT;
    inSlowStart = false;
  }

  if (inSlowStart) {
    // Slow start. Aggressive growth until we see congestion.

    if (diff > 25) {
      // If we see an increased latency then we assume we've hit the
      // limit and it's time to leave slow start and switch to
      // congestion avoidance
      congWindow = congWindow * baseRTT / minRTT;
      inSlowStart = false;
    } else {
      // It's not safe to increase unless we actually used the entire
      // congestion window, hence we look at minCongestedRTT and not
      // minRTT

      diff = minCongestedRTT - baseRTT;
      if (diff < 25)
        congWindow *= 2;
    }
  } else {
    // Congestion avoidance (VEGAS)

    if (diff > 50) {
      // Slightly too fast
      congWindow -= 4096;
    } else {
      // Only the "congested" pongs are checked to see if the
      // window is too small.

      diff = minCongestedRTT - baseRTT;

      if (diff < 5) {
        // Way too slow
        congWindow += 8192;
      } else if (diff < 25) {
        // Too slow
        congWindow += 4096;
      }
    }
  }

  if (congWindow < MINIMUM_WINDOW)
    congWindow = MINIMUM_WINDOW;
  if (congWindow > MAXIMUM_WINDOW)
    congWindow = MAXIMUM_WINDOW;

#ifdef CONGESTION_DEBUG
  vlog.debug("RTT: %d/%d ms (%d ms), Window: %d KiB, Bandwidth: %g Mbps%s",
             minRTT, minCongestedRTT, baseRTT, congWindow / 1024,
             congWindow * 8.0 / baseRTT / 1000.0,
             inSlowStart ? " (slow start)" : "");
#endif

  measurements = 0;
  gettimeofday(&lastAdjustment, NULL);
  minRTT = minCongestedRTT = -1;
}

