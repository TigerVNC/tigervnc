/* Copyright 2015 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
 * This program reads files produced by TightVNC's/TurboVNC's
 * compare-encodings. It is basically a dump of the RFB protocol
 * from the server side from the ServerInit message and forward.
 * It is assumed that the client is using a bgr888 (LE) pixel
 * format.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include <rdr/Exception.h>
#include <rdr/FileInStream.h>
#include <rdr/OutStream.h>

#include <rfb/CConnection.h>
#include <rfb/CMsgReader.h>
#include <rfb/CMsgWriter.h>
#include <rfb/PixelBuffer.h>
#include <rfb/PixelFormat.h>

#include "util.h"

// FIXME: Files are always in this format
static const rfb::PixelFormat filePF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

class DummyOutStream : public rdr::OutStream {
public:
  DummyOutStream();

  virtual size_t length();
  virtual void flush();

private:
  virtual void overrun(size_t needed);

  int offset;
  rdr::U8 buf[131072];
};

class CConn : public rfb::CConnection {
public:
  CConn(const char *filename);
  ~CConn();

  virtual void initDone();
  virtual void setPixelFormat(const rfb::PixelFormat& pf);
  virtual void setCursor(int, int, const rfb::Point&, const rdr::U8*);
  virtual void setCursorPos(const rfb::Point&);
  virtual void framebufferUpdateStart();
  virtual void framebufferUpdateEnd();
  virtual void setColourMapEntries(int, int, rdr::U16*);
  virtual void bell();
  virtual void serverCutText(const char*);

public:
  double cpuTime;

protected:
  rdr::FileInStream *in;
  DummyOutStream *out;
};

DummyOutStream::DummyOutStream()
{
  offset = 0;
  ptr = buf;
  end = buf + sizeof(buf);
}

size_t DummyOutStream::length()
{
  flush();
  return offset;
}

void DummyOutStream::flush()
{
  offset += ptr - buf;
  ptr = buf;
}

void DummyOutStream::overrun(size_t needed)
{
  flush();
  if (avail() < needed)
    throw rdr::Exception("Insufficient dummy output buffer");
}

CConn::CConn(const char *filename)
{
  cpuTime = 0.0;

  in = new rdr::FileInStream(filename);
  out = new DummyOutStream;
  setStreams(in, out);

  // Need to skip the initial handshake
  setState(RFBSTATE_INITIALISATION);
  // That also means that the reader and writer weren't setup
  setReader(new rfb::CMsgReader(this, in));
  setWriter(new rfb::CMsgWriter(&server, out));
}

CConn::~CConn()
{
  delete in;
  delete out;
}

void CConn::initDone()
{
  setFramebuffer(new rfb::ManagedPixelBuffer(filePF,
                                             server.width(),
                                             server.height()));
}

void CConn::setPixelFormat(const rfb::PixelFormat& pf)
{
  // Override format
  CConnection::setPixelFormat(filePF);
}

void CConn::setCursor(int, int, const rfb::Point&, const rdr::U8*)
{
}

void CConn::setCursorPos(const rfb::Point&)
{
}

void CConn::framebufferUpdateStart()
{
  CConnection::framebufferUpdateStart();

  startCpuCounter();
}

void CConn::framebufferUpdateEnd()
{
  CConnection::framebufferUpdateEnd();

  endCpuCounter();

  cpuTime += getCpuCounter();
}

void CConn::setColourMapEntries(int, int, rdr::U16*)
{
}

void CConn::bell()
{
}

void CConn::serverCutText(const char*)
{
}

struct stats
{
  double decodeTime;
  double realTime;
};

static struct stats runTest(const char *fn)
{
  CConn *cc;
  struct timeval start, stop;
  struct stats s;

  gettimeofday(&start, NULL);

  try {
    cc = new CConn(fn);
  } catch (rdr::Exception& e) {
    fprintf(stderr, "Failed to open rfb file: %s\n", e.str());
    exit(1);
  }

  try {
    while (true)
      cc->processMsg();
  } catch (rdr::EndOfStream& e) {
  } catch (rdr::Exception& e) {
    fprintf(stderr, "Failed to run rfb file: %s\n", e.str());
    exit(1);
  }

  gettimeofday(&stop, NULL);

  s.decodeTime = cc->cpuTime;
  s.realTime = (double)stop.tv_sec - start.tv_sec;
  s.realTime += ((double)stop.tv_usec - start.tv_usec)/1000000.0;

  delete cc;

  return s;
}

static void sort(double *array, int count)
{
  bool sorted;
  int i;
  do {
    sorted = true;
    for (i = 1;i < count;i++) {
      if (array[i-1] > array[i]) {
        double d;
        d = array[i];
        array[i] = array[i-1];
        array[i-1] = d;
        sorted = false;
      }
    }
  } while (!sorted);
}

static const int runCount = 9;

int main(int argc, char **argv)
{
  int i;
  struct stats runs[runCount];
  double values[runCount], dev[runCount];
  double median, meddev;

  if (argc != 2) {
    printf("Syntax: %s <rfb file>\n", argv[0]);
    return 1;
  }

  // Warmup
  runTest(argv[1]);

  // Multiple runs to get a good average
  for (i = 0;i < runCount;i++)
    runs[i] = runTest(argv[1]);

  // Calculate median and median deviation for CPU usage
  for (i = 0;i < runCount;i++)
    values[i] = runs[i].decodeTime;

  sort(values, runCount);
  median = values[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((values[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("CPU time: %g s (+/- %g %%)\n", median, meddev);

  // And for CPU core usage
  for (i = 0;i < runCount;i++)
    values[i] = runs[i].decodeTime / runs[i].realTime;

  sort(values, runCount);
  median = values[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((values[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("Core usage: %g (+/- %g %%)\n", median, meddev);

  return 0;
}
