#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>

#include <rfb/IrixDMIC_RawToJpeg.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("IrixDMIC");

//
// Constructor. Find a converter and create a context. Also, create
// DMparams structures for using with dmICSetSrcParams,
// dmICSetDstParams and dmICSetConvParams.
//

IrixDMIC_RawToJpeg::IrixDMIC_RawToJpeg(bool needRealtime)
  : m_valid_ic(false),
    m_srcParams(0),
    m_dstParams(0),
    m_convParams(0)
{
  if ( dmParamsCreate(&m_srcParams) != DM_SUCCESS ||
       dmParamsCreate(&m_dstParams) != DM_SUCCESS ||
       dmParamsCreate(&m_convParams) != DM_SUCCESS ) {
    reportError("dmParamsCreate");
    return;
  }

  DMparams *p;
  const int JPEG_ID = 0x6A706567; // same as 'jpeg'
  int id, direction, speed;

  int n = dmICGetNum();
  while (n--) {
    if (dmParamsCreate(&p) != DM_SUCCESS) {
      reportError("dmParamsCreate");
      return;
    }
    if (dmICGetDescription(n, p) != DM_SUCCESS)
      continue;
    id = dmParamsGetInt(p, DM_IC_ID);
    direction = dmParamsGetEnum(p, DM_IC_CODE_DIRECTION);
    speed = dmParamsGetEnum(p, DM_IC_SPEED);
    if ( id == JPEG_ID && direction == DM_IC_CODE_DIRECTION_ENCODE &&
	 (!needRealtime || speed == DM_IC_SPEED_REALTIME) ) {

      const char *engine = dmParamsGetString(p, DM_IC_ENGINE);
      vlog.info("Found JPEG encoder: \"%s\" (%d)", engine, n);

      dmParamsDestroy(p);
      break;
    }
    dmParamsDestroy(p);
  }
  if (n < 0) {
    vlog.error("Error: No matching JPEG encoder found");
    debugListConverters();
    return;
  }
  if (dmICCreate(n, &m_ic) != DM_SUCCESS) {
    reportError("dmICCreate");
    return;
  }

  m_valid_ic = true;
}

//
// Destructor.
//

IrixDMIC_RawToJpeg::~IrixDMIC_RawToJpeg()
{
  if (m_valid_ic)
    dmICDestroy(m_ic);

  if (m_convParams)
    dmParamsDestroy(m_convParams);
  if (m_dstParams)
    dmParamsDestroy(m_dstParams);
  if (m_srcParams)
    dmParamsDestroy(m_srcParams);
}

void
IrixDMIC_RawToJpeg::debugListConverters()
{
  int n = dmICGetNum();
  vlog.debug("IRIX DMIC converters available (%d):", n);
  for (int i = 0; i < n; i++) {
    DMparams *p;
    if (dmParamsCreate(&p) != DM_SUCCESS) {
      return;
    }
    if (dmICGetDescription(i, p) != DM_SUCCESS) {
      vlog.debug("  continue");
      continue;
    }
    union {
      int id;
      char label[5];
    } id;
    id.id = dmParamsGetInt(p, DM_IC_ID);
    id.label[4] = '\0';
    int direction = dmParamsGetEnum(p, DM_IC_CODE_DIRECTION);
    const char *directionLabel = "converter";
    if (direction == DM_IC_CODE_DIRECTION_ENCODE) {
      directionLabel = "encoder";
    } else if (direction == DM_IC_CODE_DIRECTION_DECODE) {
      directionLabel = "decoder";
    }
    int speed = dmParamsGetEnum(p, DM_IC_SPEED);
    const char *speedLabel = "";
    if (speed == DM_IC_SPEED_REALTIME) {
      speedLabel = "realtime ";
    }
    const char *engine = dmParamsGetString(p, DM_IC_ENGINE);
    vlog.debug("  converter %d: '%s' %s%s (%s)",
	       i, id.label, speedLabel, directionLabel, engine);
    dmParamsDestroy(p);
  }
}

//
// Configure the source and destination formats.
//
// FIXME: Remember image size that was previously set, do not set the
//        same size again.
//

bool
IrixDMIC_RawToJpeg::setImageParams(int w, int h)
{
  if (!m_valid_ic) {
    reportErrorNotInited();
    return false;
  }

  // Set source image parameters.
  DMpacking packing = DM_IMAGE_PACKING_XBGR;
  int orient = DM_IMAGE_TOP_TO_BOTTOM;
  if (dmSetImageDefaults(m_srcParams, w, h, packing) != DM_SUCCESS) {
    reportError("dmSetImageDefaults");
    return false;
  }
  DMstatus err = dmParamsSetEnum(m_srcParams, DM_IMAGE_ORIENTATION, orient);
  if (err != DM_SUCCESS) {
    reportError("dmParamsSetEnum");
    return false;
  }
  if (dmICSetSrcParams(m_ic, m_srcParams) != DM_SUCCESS) {
    reportError("dmICSetSrcParams");
    return false;
  }

  // Set destination image parameters.
  packing = DM_IMAGE_PACKING_CbYCrY;
  const char *compression = DM_IMAGE_JPEG;
  if (dmSetImageDefaults(m_dstParams, w, h, packing) != DM_SUCCESS) {
    reportError("dmSetImageDefaults");
    return false;
  }
  err = dmParamsSetEnum(m_dstParams, DM_IMAGE_ORIENTATION, orient);
  if (err != DM_SUCCESS) {
    reportError("dmParamsSetEnum");
    return false;
  }
  err = dmParamsSetString(m_dstParams, DM_IMAGE_COMPRESSION, compression);
  if (err != DM_SUCCESS) {
    reportError("dmParamsSetString");
    return false;
  }
  if (dmICSetDstParams(m_ic, m_dstParams) != DM_SUCCESS) {
    reportError("dmICSetDstParams");
    return false;
  }

  return true;
}

//
// Set JPEG image quality level (an integer in the range 0..99).
//
// FIXME: Remember image quality that was previously set, do not set
//        the same quality again.
//

bool
IrixDMIC_RawToJpeg::setImageQuality(int quality)
{
  if (!m_valid_ic) {
    reportErrorNotInited();
    return false;
  }

  double qf = (double)quality / 100.0;

  DMstatus err = dmParamsSetFloat(m_convParams, DM_IMAGE_QUALITY_SPATIAL, qf);
  if (err != DM_SUCCESS) {
    reportError("dmParamsSetFloat");
    return false;
  }

  // For some reason, dmICSetConvParams() does not have effect without
  // calling dmICSetDstParams() as well. So we call it here.
  if (m_dstParams && dmParamsGetNumElems(m_dstParams) &&
      dmICSetDstParams(m_ic, m_dstParams) != DM_SUCCESS) {
    reportError("dmICSetDstParams");
    return false;
  }

  if (dmICSetConvParams(m_ic, m_convParams) != DM_SUCCESS) {
    reportError("dmICSetConvParams");
    return false;
  }

  return true;
}

//
// Set up source buffer pool.
//
// NOTE: Both setImageParams() and setImageQuality() functions should
//       be called prior to creating and registering buffer pools.
//

bool
IrixDMIC_RawToJpeg::createSrcBufferPool(DMbufferpool *pool,
                                        int bufCount, int bufSize)
{
  if (!m_valid_ic) {
    reportErrorNotInited();
    return false;
  }

  DMparams *p;
  if (dmParamsCreate(&p) != DM_SUCCESS) {
    reportError("dmParamsCreate");
    return false;
  }

  if (dmBufferSetPoolDefaults(p, bufCount, bufSize, DM_FALSE, DM_TRUE) !=
      DM_SUCCESS) {
    reportError("dmBufferSetPoolDefaults");
    dmParamsDestroy(p);
    return false;
  }
  if (dmICGetSrcPoolParams(m_ic, p) != DM_SUCCESS) {
    reportError("dmICGetSrcPoolParams");
    dmParamsDestroy(p);
    return false;
  }
  if (dmBufferCreatePool(p, pool) != DM_SUCCESS) {
    reportError("dmBufferCreatePool");
    dmParamsDestroy(p);
    return false;
  }

  dmParamsDestroy(p);
  return true;
}

//
// Set up and register destination buffer pool.
//
// NOTE: Both setImageParams() and setImageQuality() functions should
//       be called prior to creating and registering buffer pools.
//

bool
IrixDMIC_RawToJpeg::registerDstBufferPool(DMbufferpool *pool,
                                          int bufCount, int bufSize)
{
  if (!m_valid_ic) {
    reportErrorNotInited();
    return false;
  }

  DMparams *p;
  if (dmParamsCreate(&p) != DM_SUCCESS) {
    reportError("dmParamsCreate");
    return false;
  }

  if (dmBufferSetPoolDefaults(p, bufCount, bufSize, DM_FALSE, DM_TRUE) !=
      DM_SUCCESS) {
    reportError("dmBufferSetPoolDefaults");
    dmParamsDestroy(p);
    return false;
  }
  if (dmICGetDstPoolParams(m_ic, p) != DM_SUCCESS) {
    reportError("dmICGetDstPoolParams");
    dmParamsDestroy(p);
    return false;
  }
  if (dmBufferCreatePool(p, pool) != DM_SUCCESS) {
    reportError("dmBufferCreatePool");
    dmParamsDestroy(p);
    return false;
  }

  dmParamsDestroy(p);

  if (dmICSetDstPool(m_ic, *pool) != DM_SUCCESS) {
    reportError("dmICSetDstPool");
    destroyBufferPool(*pool);
    return false;
  }

  return true;
}

//
// Destroy buffer pool created with either createSrcBufferPool() or
// registerDstBufferPool().
//

void
IrixDMIC_RawToJpeg::destroyBufferPool(DMbufferpool pool)
{
  if (dmBufferDestroyPool(pool) != DM_SUCCESS)
    reportError("dmBufferDestroyPool");
}

//
// Allocate a buffer from the specified pool.
//

bool
IrixDMIC_RawToJpeg::allocBuffer(DMbuffer *pbuf, DMbufferpool pool)
{
  if (dmBufferAllocate(pool, pbuf) != DM_SUCCESS) {
    reportError("dmBufferAllocate");
    return false;
  }

  return true;
}

//
// Fill in a DMbuffer with data.
//
// NOTE: The caller must make sure that the buffer size is no less
//       than dataSize.
//

bool
IrixDMIC_RawToJpeg::copyToBuffer(DMbuffer buf, const void *data, int dataSize)
{
  void *bufPtr = dmBufferMapData(buf);
  memcpy(bufPtr, data, dataSize);

  if (dmBufferSetSize(buf, dataSize) != DM_SUCCESS) {
    reportError("dmBufferSetSize");
    return false;
  }

  return true;
}

//
// Fill in a DMbuffer with data.
//
// NOTE: The caller must make sure that the buffer size is no less
//       than (nRows * rowSize).
//

bool
IrixDMIC_RawToJpeg::copyToBuffer(DMbuffer buf, const void *data,
                                 int rowSize, int nRows, int stride)
{
  char *dataBytes = (char *)data;
  char *bufPtr = (char *)dmBufferMapData(buf);
  for (int i = 0; i < nRows; i++) {
    memcpy(bufPtr, &dataBytes[i * stride], rowSize);
    bufPtr += rowSize;
  }

  if (dmBufferSetSize(buf, nRows * rowSize) != DM_SUCCESS) {
    reportError("dmBufferSetSize");
    return false;
  }

  return true;
}

//
// Map DMbuffer to physical memory.
//

void *
IrixDMIC_RawToJpeg::mapBufferData(DMbuffer buf)
{
  return dmBufferMapData(buf);
}

//
// Get the number of valid bytes in DMbuffer.
//

int
IrixDMIC_RawToJpeg::getBufferSize(DMbuffer buf)
{
  return dmBufferGetSize(buf);
}

//
// Free DMbuffer.
//

void
IrixDMIC_RawToJpeg::freeBuffer(DMbuffer buf)
{
  if (dmBufferFree(buf) != DM_SUCCESS)
    reportError("dmBufferFree");
}

//
// Send input data (raw pixels) to the converter.
//

bool
IrixDMIC_RawToJpeg::sendData(DMbuffer buf)
{
  if (dmICSend(m_ic, buf, 0, NULL) != DM_SUCCESS) {
    reportError("dmICSend");
    return false;
  }

  return true;
}

//
// Wait until compression is finished (infinite timeout!).
// This function should be called after sendData() and before receiveData().
//
// FIXME: Report errors.
//

bool
IrixDMIC_RawToJpeg::waitConversion()
{
  struct pollfd ps;
  ps.fd = dmICGetDstQueueFD(m_ic);
  ps.events = POLLIN;

  int result = poll(&ps, 1, -1);
  if (result != 1)
    return false;

  if ((ps.revents & POLLIN) != 0) {
    return true;
  } else {
    return false;
  }
}

//
// Receive output (JPEG data) from the converter.
// Call waitConversion() function first.
//

bool
IrixDMIC_RawToJpeg::receiveData(DMbuffer *pbuf)
{
  if (dmICReceive(m_ic, pbuf) != DM_SUCCESS) {
    reportError("dmICReceive");
    return false;
  }

  return true;
}

//
// Report an error when a function returns DM_FAILURE.
//

void
IrixDMIC_RawToJpeg::reportError(const char *funcName)
{
  char errorDetail[DM_MAX_ERROR_DETAIL];
  const char *errorCategory = dmGetError(NULL, errorDetail);
  vlog.error("%s() failed: %s: %s",
             funcName, errorCategory, errorDetail);
}

//
// Report an error when (m_valid_ic == false).
//

void
IrixDMIC_RawToJpeg::reportErrorNotInited()
{
  vlog.error("Internal error: Image converter not initialized");
}

