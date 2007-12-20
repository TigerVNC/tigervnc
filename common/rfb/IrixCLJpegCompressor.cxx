#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include <rfb/IrixCLJpegCompressor.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("IrixCLJpeg");

const int IrixCLJpegCompressor::DEFAULT_QUALITY = 75;

//
// Constructor and destructor.
//

IrixCLJpegCompressor::IrixCLJpegCompressor()
  : m_quality(DEFAULT_QUALITY),
    m_clHandleValid(false),
    m_srcBufferSize(0),
    m_dstBufferSize(0),
    m_sourceData(0),
    m_compressedData(0),
    m_compressedLength(0)
{
  int impactScheme = clQuerySchemeFromName(CL_ALG_VIDEO, "impact");
  if (impactScheme < 0) {
    vlog.error("Warning: No compression scheme named \"impact\"");
  }

  vlog.debug("Trying \"impact\" compression scheme (Octane Compression)");
  int r = clOpenCompressor(impactScheme, &m_clHandle);
  if (r == SUCCESS) {
    vlog.debug("Using \"impact\" compression scheme");
    m_clHandleValid = true;
    return;
  }
  vlog.debug("Trying Cosmo Compress");
  r = clOpenCompressor(CL_JPEG_COSMO, &m_clHandle);
  if (r == SUCCESS) {
    vlog.debug("Using Cosmo Compress");
    m_clHandleValid = true;
    return;
  }

#ifdef DEBUG_FORCE_CL
  vlog.debug("DEBUG: Trying Irix software JPEG compressor");
  r = clOpenCompressor(CL_JPEG_SOFTWARE, &m_clHandle);
  if (r == SUCCESS) {
    vlog.debug("DEBUG: Using Irix software JPEG compressor");
    m_clHandleValid = true;
    return;
  }
#endif

  vlog.error("Ho hardware JPEG available via Irix CL library");
}

IrixCLJpegCompressor::~IrixCLJpegCompressor()
{
  if (m_clHandleValid)
    clCloseCompressor(m_clHandle);

  if (m_sourceData)
    delete[] m_sourceData;
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
//        (depth 24), compatible with IRIX Compression Library.
//

void
IrixCLJpegCompressor::compress(const rdr::U32 *buf,
                               const PixelFormat *fmt,
                               int w, int h, int stride)
{
  // Discard previous compression results.
  m_compressedLength = 0;

  // A sanity check.
  if (!m_clHandleValid)
    return;

  // (Re)allocate the source buffer if necessary.
  if (w * h > m_srcBufferSize) {
    if (m_sourceData)
      delete[] m_sourceData;
    m_srcBufferSize = w * h;
    m_sourceData = new rdr::U32[m_srcBufferSize];
  }

  // Copy source pixels
  for (int y = 0; y < h; y++) {
    memcpy(&m_sourceData[y * w], &buf[y * stride], w * sizeof(rdr::U32));
  }

  // Set image attributes and JPEG quality factor.
  clSetParam(m_clHandle, CL_FORMAT, CL_FORMAT_XBGR);
  clSetParam(m_clHandle, CL_IMAGE_WIDTH, w);
  clSetParam(m_clHandle, CL_IMAGE_HEIGHT, h);
  clSetParam(m_clHandle, CL_FRAME_BUFFER_SIZE, w * h * 4);
  clSetParam(m_clHandle, CL_JPEG_QUALITY_FACTOR, m_quality);

  // Determine buffer size required.
  int newBufferSize = clGetParam(m_clHandle, CL_COMPRESSED_BUFFER_SIZE);

  // (Re)allocate destination buffer if necessary.
  if (newBufferSize > m_dstBufferSize) {
    if (m_compressedData)
      delete[] m_compressedData;
    m_dstBufferSize = newBufferSize;
    m_compressedData = new char[m_dstBufferSize];
  }

  int newCompressedSize = newBufferSize;
  int n = clCompress(m_clHandle, 1, m_sourceData,
                     &newCompressedSize, m_compressedData);
  if (n != 1)
    return;

  m_compressedLength = (size_t)newCompressedSize;
}

