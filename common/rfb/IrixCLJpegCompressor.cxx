#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include <rfb/IrixCLJpegCompressor.h>

using namespace rfb;

const int IrixCLJpegCompressor::DEFAULT_QUALITY = 75;

//
// Constructor and destructor.
//

IrixCLJpegCompressor::IrixCLJpegCompressor()
  : m_quality(DEFAULT_QUALITY),
    m_compressedData(0),
    m_compressedLength(0)
{
}

IrixCLJpegCompressor::~IrixCLJpegCompressor()
{
  if (m_compressedData)
    delete[] m_compressedData;
}

//
// Set JPEG quality level (1..99)
//

void
IrixCLJpegCompressor::setQuality(int level)
{
  if (level < 1) {
    level = 1;
  } else if (level > 99) {
    level = 99;
  }
  if (level != m_quality) {
    m_quality = level;
  }
}

//
// Perform JPEG compression.
//
// FIXME: This function assumes that pixel format is 32-bit XBGR
//        (depth 24), compatible with IRIX Digital Media libraries.
//

void
IrixCLJpegCompressor::compress(const rdr::U32 *buf,
                               const PixelFormat *fmt,
                               int w, int h, int stride)
{
  // Discard previous compression results.
  if (m_compressedData) {
    delete[] m_compressedData;
    m_compressedData = 0;
  }
  m_compressedLength = 0;

  // FIXME: Do something.
}

