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
 * fbs-dump, which in turn takes files from rfbproxy. It is
 * basically a dump of the RFB protocol from the server side after
 * the ServerInit message. Mostly this consists of FramebufferUpdate
 * message using the HexTile encoding. Screen size and pixel format
 * are not encoded in the file and must be specified by the user.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <rdr/Exception.h>
#include <rdr/OutStream.h>
#include <rdr/FileInStream.h>

#include <rfb/PixelFormat.h>

#include <rfb/CConnection.h>
#include <rfb/CMsgReader.h>
#include <rfb/Decoder.h>
#include <rfb/UpdateTracker.h>

#include <rfb/EncodeManager.h>
#include <rfb/SConnection.h>
#include <rfb/SMsgWriter.h>

#include "util.h"

static rfb::IntParameter width("width", "Frame buffer width", 0);
static rfb::IntParameter height("height", "Frame buffer height", 0);

static rfb::StringParameter format("format", "Pixel format (e.g. bgr888)", "");

// The frame buffer (and output) is always this format
static const rfb::PixelFormat fbPF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

// Encodings to use
static const rdr::S32 encodings[] = {
  rfb::encodingTight, rfb::encodingCopyRect, rfb::encodingRRE,
  rfb::encodingHextile, rfb::encodingZRLE, rfb::pseudoEncodingLastRect,
  rfb::pseudoEncodingQualityLevel0 + 8 };

class DummyOutStream : public rdr::OutStream {
public:
  DummyOutStream();

  virtual int length();
  virtual void flush();

private:
  virtual int overrun(int itemSize, int nItems);

  int offset;
  rdr::U8 buf[131072];
};

class CConn : public rfb::CConnection {
public:
  CConn(const char *filename);
  ~CConn();

  double getRatio();

  virtual void setDesktopSize(int w, int h);
  virtual void setCursor(int, int, const rfb::Point&, void*, void*);
  virtual void framebufferUpdateStart();
  virtual void framebufferUpdateEnd();
  virtual void dataRect(const rfb::Rect&, int);
  virtual void setColourMapEntries(int, int, rdr::U16*);
  virtual void bell();
  virtual void serverCutText(const char*, rdr::U32);

public:
  double decodeTime;
  double encodeTime;

protected:
  rdr::FileInStream *in;
  rfb::Decoder *decoders[rfb::encodingMax+1];
  rfb::ManagedPixelBuffer pb;
  rfb::SimpleUpdateTracker updates;
  class SConn *sc;
};

class Manager : public rfb::EncodeManager {
public:
  Manager(class rfb::SConnection *conn);

  double getRatio();
};

class SConn : public rfb::SConnection {
public:
  SConn();
  ~SConn();

  void writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb);

  double getRatio();

  virtual void setAccessRights(AccessRights ar);

  virtual void setDesktopSize(int fb_width, int fb_height,
                              const rfb::ScreenSet& layout);

protected:
  DummyOutStream *out;
  Manager *manager;
};

DummyOutStream::DummyOutStream()
{
  offset = 0;
  ptr = buf;
  end = buf + sizeof(buf);
}

int DummyOutStream::length()
{
  flush();
  return offset;
}

void DummyOutStream::flush()
{
  offset += ptr - buf;
  ptr = buf;
}

int DummyOutStream::overrun(int itemSize, int nItems)
{
  flush();
}

CConn::CConn(const char *filename)
{
  int i;

  decodeTime = 0.0;
  encodeTime = 0.0;

  in = new rdr::FileInStream(filename);
  setStreams(in, NULL);

  memset(decoders, 0, sizeof(decoders));
  for (i = 0;i < rfb::encodingMax;i++) {
    if (!rfb::Decoder::supported(i))
      continue;

    decoders[i] = rfb::Decoder::createDecoder(i, this);
  }

  pb.setPF(fbPF);

  // Need to skip the initial handshake and ServerInit
  setState(RFBSTATE_NORMAL);
  // That also means that the reader and writer weren't setup
  setReader(new rfb::CMsgReader(this, in));
  // Nor the frame buffer size and format
  setDesktopSize(width, height);
  rfb::PixelFormat pf;
  pf.parse(format);
  setPixelFormat(pf);

  sc = new SConn();
  sc->cp.setPF(pb.getPF());
  sc->setEncodings(sizeof(encodings)/sizeof(*encodings), encodings);
}

CConn::~CConn()
{
  int i;

  delete sc;

  delete in;

  for (i = 0;i < rfb::encodingMax;i++)
    delete decoders[i];
}

double CConn::getRatio()
{
  return sc->getRatio();
}

void CConn::setDesktopSize(int w, int h)
{
  CConnection::setDesktopSize(w, h);

  pb.setSize(cp.width, cp.height);
}

void CConn::setCursor(int, int, const rfb::Point&, void*, void*)
{
}

void CConn::framebufferUpdateStart()
{
  updates.clear();
}

void CConn::framebufferUpdateEnd()
{
  rfb::UpdateInfo ui;
  rfb::Region clip(pb.getRect());

  updates.getUpdateInfo(&ui, clip);

  startCpuCounter();
  sc->writeUpdate(ui, &pb);
  endCpuCounter();

  encodeTime += getCpuCounter();
}

void CConn::dataRect(const rfb::Rect &r, int encoding)
{
  if (!decoders[encoding])
    throw rdr::Exception("Unknown encoding");

  startCpuCounter();
  decoders[encoding]->readRect(r, &pb);
  endCpuCounter();

  decodeTime += getCpuCounter();

  if (encoding != rfb::encodingCopyRect) // FIXME
    updates.add_changed(rfb::Region(r));
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

Manager::Manager(class rfb::SConnection *conn) :
  EncodeManager(conn)
{
}

double Manager::getRatio()
{
  StatsVector::iterator iter;
  unsigned long long bytes, equivalent;

  bytes = equivalent = 0;
  for (iter = stats.begin();iter != stats.end();++iter) {
    StatsVector::value_type::iterator iter2;
    for (iter2 = iter->begin();iter2 != iter->end();++iter2) {
      bytes += iter2->bytes;
      equivalent += iter2->equivalent;
    }
  }

  return (double)equivalent / bytes;
}

SConn::SConn()
{
  out = new DummyOutStream;
  setStreams(NULL, out);

  setWriter(new rfb::SMsgWriter(&cp, out));

  manager = new Manager(this);
}

SConn::~SConn()
{
  delete manager;
  delete out;
}

void SConn::writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb)
{
  manager->writeUpdate(ui, pb, NULL);
}

double SConn::getRatio()
{
  return manager->getRatio();
}

void SConn::setAccessRights(AccessRights ar)
{
}

void SConn::setDesktopSize(int fb_width, int fb_height,
                           const rfb::ScreenSet& layout)
{
}

static double runTest(const char *fn, double *ratio)
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

  time = cc->encodeTime;
  *ratio = cc->getRatio();

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

static void usage(const char *argv0)
{
  fprintf(stderr, "Syntax: %s [options] <rfb file>\n", argv0);
  fprintf(stderr, "Options:\n");
  rfb::Configuration::listParams(79, 14);
  exit(1);
}

int main(int argc, char **argv)
{
  int i;

  const char *fn;

  double times[runCount], dev[runCount];
  double median, meddev, ratio;

  fn = NULL;
  for (i = 1; i < argc; i++) {
    if (rfb::Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (rfb::Configuration::setParam(&argv[i][1], argv[i+1])) {
          i++;
          continue;
        }
      }
      usage(argv[0]);
    }

    if (fn != NULL)
      usage(argv[0]);

    fn = argv[i];
  }

  if (fn == NULL) {
    fprintf(stderr, "No file specified!\n\n");
    usage(argv[0]);
  }

  if (!format.hasBeenSet()) {
    fprintf(stderr, "Pixel format not specified!\n\n");
    usage(argv[0]);
  }

  if (!width.hasBeenSet() || !height.hasBeenSet()) {
    fprintf(stderr, "Frame buffer size not specified!\n\n");
    usage(argv[0]);
  }

  // Warmup
  runTest(fn, &ratio);

  // Multiple runs to get a good average
  for (i = 0;i < runCount;i++)
    times[i] = runTest(fn, &ratio);

  // Calculate median and median deviation
  sort(times, runCount);
  median = times[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((times[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("CPU time: %g s (+/- %g %)\n", median, meddev);
  printf("Ratio: %g\n", ratio);

  return 0;
}
