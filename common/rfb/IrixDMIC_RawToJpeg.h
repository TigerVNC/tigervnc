#ifndef __IRIXDMIC_RAWTOJPEG_H__
#define __IRIXDMIC_RAWTOJPEG_H__

#include <dmedia/dmedia.h>
#include <dmedia/dm_imageconvert.h>

namespace rfb {

  //
  // A C++ wrapper for IRIX-specific Digital Media Image Conversion
  // library used for JPEG compression.
  //

  class IrixDMIC_RawToJpeg
  {
  public:
    IrixDMIC_RawToJpeg(bool needRealtime = true);
    virtual ~IrixDMIC_RawToJpeg();

    void debugListConverters();

    bool isValid() const { return m_valid_ic; }

    bool setImageParams(int w, int h);
    bool setImageQuality(int quality);

    bool createSrcBufferPool(DMbufferpool *pool, int bufCount, int bufSize);
    bool registerDstBufferPool(DMbufferpool *pool, int bufCount, int bufSize);
    static void destroyBufferPool(DMbufferpool pool);

    static bool allocBuffer(DMbuffer *pbuf, DMbufferpool pool);
    static bool copyToBuffer(DMbuffer buf, const void *data, int dataSize);
    static bool copyToBuffer(DMbuffer buf, const void *data,
                             int rowSize, int nRows, int stride);
    static int getBufferSize(DMbuffer buf);
    static void * mapBufferData(DMbuffer buf);
    static void freeBuffer(DMbuffer buf);

    bool sendData(DMbuffer buf);
    bool waitConversion();
    bool receiveData(DMbuffer *pbuf);

    static void reportError(const char *funcName);
    static void reportErrorNotInited();

  protected:
    DMimageconverter m_ic;
    bool m_valid_ic;

    DMparams *m_srcParams;
    DMparams *m_dstParams;
    DMparams *m_convParams;
  };

}

#endif // __IRIXDMIC_RAWTOJPEG_H__

