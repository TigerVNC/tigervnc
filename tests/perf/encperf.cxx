/* Copyright 2015 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2015 D. R. Commander.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define __USE_MINGW_ANSI_STDIO 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include <core/Configuration.h>

#include <rdr/OutStream.h>
#include <rdr/FileInStream.h>

#include <rfb/AccessRights.h>
#include <rfb/PixelFormat.h>
#include <rfb/CConnection.h>
#include <rfb/CMsgReader.h>
#include <rfb/CMsgWriter.h>
#include <rfb/UpdateTracker.h>
#include <rfb/EncodeManager.h>
#include <rfb/SConnection.h>
#include <rfb/SMsgWriter.h>

#include "util.h"

static core::IntParameter width("width", "Frame buffer width", 0);
static core::IntParameter height("height", "Frame buffer height", 0);
static core::IntParameter count("count", "Number of benchmark iterations", 9);

static core::StringParameter format("format", "Pixel format (e.g. bgr888)", "");

static core::BoolParameter translate("translate",
                                     "Translate 8-bit and 16-bit datasets into 24-bit",
                                     true);

// The frame buffer (and output) is always this format
static const rfb::PixelFormat fbPF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

// Encodings to use
static const int32_t encodings[] = {
  rfb::encodingTight, rfb::encodingCopyRect, rfb::encodingRRE,
  rfb::encodingHextile, rfb::encodingZRLE, rfb::pseudoEncodingLastRect,
  rfb::pseudoEncodingQualityLevel0 + 8,
  rfb::pseudoEncodingCompressLevel0 + 2};

class DummyOutStream : public rdr::OutStream {
public:
  DummyOutStream();

  size_t length() override;
  void flush() override;

private:
  void overrun(size_t needed) override;

  int offset;
  uint8_t buf[131072];
};

class CConn : public rfb::CConnection {
public:
  CConn(const char *filename);
  ~CConn();

  void getStats(double& ratio, unsigned long long& bytes,
                unsigned long long& rawEquivalent);

  void initDone() override {};
  void resizeFramebuffer() override;
  void setCursor(int, int, const core::Point&, const uint8_t*) override;
  void setCursorPos(const core::Point&) override;
  void framebufferUpdateStart() override;
  void framebufferUpdateEnd() override;
  bool dataRect(const core::Rect&, int) override;
  void setColourMapEntries(int, int, uint16_t*) override;
  void bell() override;
  void serverCutText(const char*) override;
  virtual void getUserPasswd(bool secure, std::string *user, std::string *password) override;
  virtual bool showMsgBox(rfb::MsgBoxFlags flags, const char *title, const char *text) override;

public:
  double decodeTime;
  double encodeTime;

protected:
  rdr::FileInStream *in;
  DummyOutStream *out;
  rfb::SimpleUpdateTracker updates;
  class SConn *sc;
};

class Manager : public rfb::EncodeManager {
public:
  Manager(class rfb::SConnection *conn);

  void getStats(double&, unsigned long long&, unsigned long long&);
};

class SConn : public rfb::SConnection {
public:
  SConn();
  ~SConn();

  void writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb);

  void getStats(double&, unsigned long long&, unsigned long long&);

  void setAccessRights(rfb::AccessRights ar) override;

  void setDesktopSize(int fb_width, int fb_height,
                      const rfb::ScreenSet& layout) override;

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
    throw std::out_of_range("Insufficient dummy output buffer");
}

CConn::CConn(const char *filename)
{
  decodeTime = 0.0;
  encodeTime = 0.0;

  in = new rdr::FileInStream(filename);
  out = new DummyOutStream;
  setStreams(in, out);

  // Need to skip the initial handshake and ServerInit
  setState(RFBSTATE_NORMAL);
  // That also means that the reader and writer weren't setup
  setReader(new rfb::CMsgReader(this, in));
  setWriter(new rfb::CMsgWriter(&server, out));
  // Nor the frame buffer size and format
  rfb::PixelFormat pf;
  pf.parse(format);
  server.setPF(pf);
  setDesktopSize(width, height);

  sc = new SConn();
  sc->client.setPF((bool)translate ? fbPF : pf);
  sc->setEncodings(sizeof(encodings) / sizeof(*encodings), encodings);
}

CConn::~CConn()
{
  delete sc;
  delete in;
  delete out;
}

void CConn::getStats(double& ratio, unsigned long long& bytes,
                     unsigned long long& rawEquivalent)
{
  sc->getStats(ratio, bytes, rawEquivalent);
}

void CConn::resizeFramebuffer()
{
  rfb::ModifiablePixelBuffer *pb;

  pb = new rfb::ManagedPixelBuffer((bool)translate ? fbPF : server.pf(),
                                   server.width(), server.height());
  setFramebuffer(pb);
}

void CConn::setCursor(int, int, const core::Point&, const uint8_t*)
{
}

void CConn::setCursorPos(const core::Point&)
{
}

void CConn::framebufferUpdateStart()
{
  CConnection::framebufferUpdateStart();

  updates.clear();
  startCpuCounter();
}

void CConn::framebufferUpdateEnd()
{
  rfb::UpdateInfo ui;
  rfb::PixelBuffer* pb = getFramebuffer();
  core::Region clip(pb->getRect());

  CConnection::framebufferUpdateEnd();

  endCpuCounter();

  decodeTime += getCpuCounter();

  updates.getUpdateInfo(&ui, clip);

  startCpuCounter();
  sc->writeUpdate(ui, pb);
  endCpuCounter();

  encodeTime += getCpuCounter();
}

bool CConn::dataRect(const core::Rect& r, int encoding)
{
  if (!CConnection::dataRect(r, encoding))
    return false;

  if (encoding != rfb::encodingCopyRect) // FIXME
    updates.add_changed(r);

  return true;
}

void CConn::setColourMapEntries(int, int, uint16_t*)
{
}

void CConn::bell()
{
}

void CConn::serverCutText(const char*)
{
}

void CConn::getUserPasswd(bool, std::string *, std::string *)
{
}

bool CConn::showMsgBox(rfb::MsgBoxFlags, const char *, const char *)
{
    return true;
}

Manager::Manager(class rfb::SConnection *conn_) :
  EncodeManager(conn_)
{
}

void Manager::getStats(double& ratio, unsigned long long& encodedBytes,
                       unsigned long long& rawEquivalent)
{
  StatsVector::iterator iter;
  unsigned long long bytes, equivalent;

  bytes = equivalent = 0;
  for (iter = stats.begin(); iter != stats.end(); ++iter) {
    StatsVector::value_type::iterator iter2;
    for (iter2 = iter->begin(); iter2 != iter->end(); ++iter2) {
      bytes += iter2->bytes;
      equivalent += iter2->equivalent;
    }
  }

  ratio = (double)equivalent / bytes;
  encodedBytes = bytes;
  rawEquivalent = equivalent;
}

SConn::SConn()
: SConnection(rfb::AccessDefault)
{
  out = new DummyOutStream;
  setStreams(nullptr, out);

  setWriter(new rfb::SMsgWriter(&client, out));

  manager = new Manager(this);
}

SConn::~SConn()
{
  delete manager;
  delete out;
}

void SConn::writeUpdate(const rfb::UpdateInfo& ui, const rfb::PixelBuffer* pb)
{
  manager->writeUpdate(ui, pb, nullptr);
}

void SConn::getStats(double& ratio, unsigned long long& bytes,
                     unsigned long long& rawEquivalent)
{
  manager->getStats(ratio, bytes, rawEquivalent);
}

void SConn::setAccessRights(rfb::AccessRights)
{
}

void SConn::setDesktopSize(int, int, const rfb::ScreenSet&)
{
}

struct stats
{
  double decodeTime;
  double encodeTime;
  double realTime;

  double ratio;
  unsigned long long bytes;
  unsigned long long rawEquivalent;
};

static struct stats runTest(const char *fn)
{
  CConn *cc;
  struct stats s;
  struct timeval start, stop;

  gettimeofday(&start, nullptr);

  try {
    cc = new CConn(fn);
  } catch (std::exception& e) {
    fprintf(stderr, "Failed to open rfb file: %s\n", e.what());
    exit(1);
  }

  try {
    while (true)
      cc->processMsg();
  } catch (rdr::end_of_stream& e) {
  } catch (std::exception& e) {
    fprintf(stderr, "Failed to run rfb file: %s\n", e.what());
    exit(1);
  }

  gettimeofday(&stop, nullptr);

  s.decodeTime = cc->decodeTime;
  s.encodeTime = cc->encodeTime;
  s.realTime = (double)stop.tv_sec - start.tv_sec;
  s.realTime += ((double)stop.tv_usec - start.tv_usec)/1000000.0;
  cc->getStats(s.ratio, s.bytes, s.rawEquivalent);

  delete cc;

  return s;
}

static void sort(double *array, int len)
{
  bool sorted;
  int i;
  do {
    sorted = true;
    for (i = 1; i < len; i++) {
      if (array[i-1] > array[i]) {
        double d;
        d = array[i];
        array[i] = array[i - 1];
        array[i - 1] = d;
        sorted = false;
      }
    }
  } while (!sorted);
}

static void usage(const char *argv0)
{
  fprintf(stderr, "Syntax: %s [options] <rfb file>\n", argv0);
  fprintf(stderr, "Options:\n");
  core::Configuration::listParams(79, 14);
  exit(1);
}

int main(int argc, char **argv)
{
  int i;

  const char *fn;

  fn = nullptr;
  for (i = 1; i < argc; i++) {
    int ret;

    ret = core::Configuration::handleParamArg(argc, argv, i);
    if (ret > 0) {
      i += ret;
      continue;
    }

    if (argv[i][0] == '-')
      usage(argv[0]);

    if (fn != nullptr)
      usage(argv[0]);

    fn = argv[i];
  }

  int runCount = count;
  struct stats *runs = new struct stats[runCount];
  double *values = new double[runCount];
  double *dev = new double[runCount];
  double median, meddev;

  if (fn == nullptr) {
    fprintf(stderr, "No file specified!\n\n");
    usage(argv[0]);
  }

  if (strcmp(format, "") == 0) {
    fprintf(stderr, "Pixel format not specified!\n\n");
    usage(argv[0]);
  }

  if (width == 0 || height == 0) {
    fprintf(stderr, "Frame buffer size not specified!\n\n");
    usage(argv[0]);
  }

  // Warmup
  runTest(fn);

  // Multiple runs to get a good average
  for (i = 0; i < runCount; i++)
    runs[i] = runTest(fn);

  // Calculate median and median deviation for CPU usage decoding
  for (i = 0;i < runCount;i++)
    values[i] = runs[i].decodeTime;

  sort(values, runCount);
  median = values[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((values[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("CPU time (decoding): %g s (+/- %g %%)\n", median, meddev);

  // And for CPU usage encoding
  for (i = 0;i < runCount;i++)
    values[i] = runs[i].encodeTime;

  sort(values, runCount);
  median = values[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((values[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("CPU time (encoding): %g s (+/- %g %%)\n", median, meddev);

  // And for CPU core usage encoding
  for (i = 0;i < runCount;i++)
    values[i] = (runs[i].decodeTime + runs[i].encodeTime) / runs[i].realTime;

  sort(values, runCount);
  median = values[runCount/2];

  for (i = 0;i < runCount;i++)
    dev[i] = fabs((values[i] - median) / median) * 100;

  sort(dev, runCount);
  meddev = dev[runCount/2];

  printf("Core usage (total): %g (+/- %g %%)\n", median, meddev);

  printf("Encoded bytes: %llu\n", runs[0].bytes);
  printf("Raw equivalent bytes: %llu\n", runs[0].rawEquivalent);
  printf("Ratio: %g\n", runs[0].ratio);

  return 0;
}
