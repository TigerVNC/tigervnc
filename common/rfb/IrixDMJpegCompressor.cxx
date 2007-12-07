#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include <rfb/IrixDMJpegCompressor.h>

using namespace rfb;

const int IrixDMJpegCompressor::DEFAULT_QUALITY = 75;

//
// Constructor and destructor.
//

IrixDMJpegCompressor::IrixDMJpegCompressor()
  : m_quality(DEFAULT_QUALITY),
    m_width(0),
    m_height(0),
    m_bufferSize(0),
    m_compressedData(0),
    m_compressedLength(0)
{
}

IrixDMJpegCompressor::~IrixDMJpegCompressor()
{
  if (m_bufferSize > 0) {
    m_ic.destroyBufferPool(m_srcPool);
    m_ic.destroyBufferPool(m_dstPool);
  }

  if (m_compressedData)
    delete[] m_compressedData;
}

//
// Set JPEG quality level (1..99)
//

void
IrixDMJpegCompressor::setQuality(int level)
{
  if (level < 1) {
    level = 1;
  } else if (level > 99) {
    level = 99;
  }
  if (level != m_quality) {
    m_quality = level;
    m_ic.setImageQuality(level);
  }
}

//
// Perform JPEG compression.
//
// FIXME: This function assumes that pixel format is 32-bit XBGR
//        (depth 24), compatible with IRIX Digital Media libraries.
//

void
IrixDMJpegCompressor::compress(const rdr::U32 *buf,
                               const PixelFormat *fmt,
                               int w, int h, int stride)
{
  // Discard previous compression results.
  if (m_compressedData) {
    delete[] m_compressedData;
    m_compressedData = 0;
  }
  m_compressedLength = 0;

  // Setup the compressor.
  if (!updateImageSize(w, h))
    return;

  // Prepare source image data.
  DMbuffer srcBuf;
  if (!m_ic.allocBuffer(&srcBuf, m_srcPool)) {
    return;
  }
  int widthInBytes = w * (fmt->bpp / 8);
  int strideInBytes = stride * (fmt->bpp / 8);
  if (!m_ic.copyToBuffer(srcBuf, buf, widthInBytes, h, strideInBytes)) {
    m_ic.freeBuffer(srcBuf);
    return;
  }

  // Initiate compression.
  if (!m_ic.sendData(srcBuf)) {
    m_ic.freeBuffer(srcBuf);
    return;
  }
  m_ic.freeBuffer(srcBuf);

  // Wait for results.
  if (!m_ic.waitConversion()) {
    perror("poll");		// FIXME: Unify error handling.
    return;
  }

  // Save the results.
  DMbuffer dstBuf;
  if (!m_ic.receiveData(&dstBuf)) {
    return;
  }
  m_compressedLength = m_ic.getBufferSize(dstBuf);
  m_compressedData = new char[m_compressedLength];
  void *bufPtr = m_ic.mapBufferData(dstBuf);
  memcpy(m_compressedData, bufPtr, m_compressedLength);

  m_ic.freeBuffer(dstBuf);
}

//
// Update image size and make sure that our buffer pools will allocate
// properly-sized buffers.
//
// FIXME: Handle image quality separately.
//

bool
IrixDMJpegCompressor::updateImageSize(int w, int h)
{
  // Configure image formats and parameters, set JPEG image quality level.
  if (w != m_width || h != m_height) {
    if (!m_ic.setImageParams(w, h) || !m_ic.setImageQuality(m_quality)) {
      return false;
    }
    m_width = w;
    m_height = h;
  }

  // Set up source and destination buffer pools.
  int dataLen = w * h * 4;
  if (dataLen > m_bufferSize) {
    if (m_bufferSize > 0) {
      m_ic.destroyBufferPool(m_srcPool);
      m_ic.destroyBufferPool(m_dstPool);
      m_bufferSize = 0;
    }
    if (!m_ic.createSrcBufferPool(&m_srcPool, 1, dataLen)) {
      return false;
    }
    if (!m_ic.registerDstBufferPool(&m_dstPool, 1, dataLen)) {
      m_ic.destroyBufferPool(m_srcPool);
      return false;
    }
    m_bufferSize = dataLen;
  }

  return true;
}

