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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <rdr/Exception.h>
#include <rdr/FileInStream.h>

#include <rfb/CConnection.h>
#include <rfb/CMsgReader.h>
#include <rfb/Decoder.h>
#include <rfb/PixelBuffer.h>
#include <rfb/PixelFormat.h>

#include "util.h"

// FIXME: Files are always in this format
static const rfb::PixelFormat filePF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

class CConn : public rfb::CConnection {
public:
  CConn(const char *filename);
  ~CConn();

  virtual void setDesktopSize(int w, int h);
  virtual void setPixelFormat(const rfb::PixelFormat& pf);
  virtual void setCursor(int, int, const rfb::Point&, void*, void*);
  virtual void dataRect(const rfb::Rect&, int);
  virtual void setColourMapEntries(int, int, rdr::U16*);
  virtual void bell();
  virtual void serverCutText(const char*, rdr::U32);

public:
  double cpuTime;

protected:
  rdr::FileInStream *in;
  rfb::Decoder *decoders[rfb::encodingMax+1];
  rfb::ManagedPixelBuffer pb;
};

CConn::CConn(const char *filename)
{
  int i;

  cpuTime = 0.0;

  in = new rdr::FileInStream(filename);
  setStreams(in, NULL);

  memset(decoders, 0, sizeof(decoders));
  for (i = 0;i < rfb::encodingMax;i++) {
    if (!rfb::Decoder::supported(i))
      continue;

    decoders[i] = rfb::Decoder::createDecoder(i, this);
  }

  // Need to skip the initial handshake
  setState(RFBSTATE_INITIALISATION);
  // That also means that the reader and writer weren't setup
  setReader(new rfb::CMsgReader(this, in));
}

CConn::~CConn()
{
  int i;

  delete in;

  for (i = 0;i < rfb::encodingMax;i++)
    delete decoders[i];
}

void CConn::setDesktopSize(int w, int h)
{
  CConnection::setDesktopSize(w, h);

  pb.setSize(cp.width, cp.height);
}

void CConn::setPixelFormat(const rfb::PixelFormat& pf)
{
  // Override format
  CConnection::setPixelFormat(filePF);

  pb.setPF(cp.pf());
}

void CConn::setCursor(int, int, const rfb::Point&, void*, void*)
{
}

void CConn::dataRect(const rfb::Rect &r, int encoding)
{
  if (!decoders[encoding])
    throw rdr::Exception("Unknown encoding");

  startCpuCounter();
  decoders[encoding]->readRect(r, &pb);
  endCpuCounter();

  cpuTime += getCpuCounter();
}

void CConn::setColourMapEntries(int, int, rdr::U16*)
{
}

void CConn::bell()
{
}

void CConn::serverCutText(const char*, rdr::U32)
{
}

static double runTest(const char *fn)
{
  CConn *cc;
  double time;

  cc = new CConn(fn);

  try {
    while (true)
      cc->processMsg();
  } catch (rdr::EndOfStream e) {
  } catch (rdr::Exception e) {
    fprintf(stderr, "Failed to run rfb file: %s\n", e.str());
    exit(1);
  }

  time = cc->cpuTime;

  delete cc;

  return time;
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
  double times[runCount], dev[runCount];
  double median, meddev;

  if (argc != 2) {
    printf("Syntax: %s <rfb file>\n", argv[0]);
    return 1;
  }

  // Warmup
  runTest(argv[1]);

  // Multiple runs to get a good average
  for (i = 0;i < runCount;i++)
    times[i] = runTest(argv[1]);

  // Calculate median and median deviation
  sort(times, runCount);
  median = times[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((times[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("CPU time: %g s (+/- %g %)\n", median, meddev);

  return 0;
}
