#ifndef __IRIXDMJPEGCOMPRESSOR_H__
#define __IRIXDMJPEGCOMPRESSOR_H__

#include <sys/types.h>

#include <rdr/types.h>
#include <rfb/PixelFormat.h>

#include <rfb/JpegCompressor.h>
#include <rfb/IrixDMIC_RawToJpeg.h>

namespace rfb {

  //
  // A C++ class for performing JPEG compression.
  // This implementation uses IRIX Digital Media libraries.
  //

  class IrixDMJpegCompressor : public JpegCompressor
  {
  public:
    IrixDMJpegCompressor();
    virtual ~IrixDMJpegCompressor();

    // Check if the object has been created successfully.
    bool isValid() const { return m_ic.isValid(); }

    // Set JPEG quality level (0..100).
    virtual void setQuality(int level);

    // Actually compress the image.
    virtual void compress(const rdr::U32 *buf, const PixelFormat *fmt,
			  int w, int h, int stride);

    // Access results of the compression.
    virtual size_t getDataLength() { return m_compressedLength; }
    virtual const char *getDataPtr() { return m_compressedData; }

  protected:
    // Update image size and make sure that our buffer pools will
    // allocate properly-sized buffers.
    bool updateImageSize(int w, int h);

  protected:
    static const int DEFAULT_QUALITY;
    int m_quality;

    IrixDMIC_RawToJpeg m_ic;
    int m_width;
    int m_height;
    DMbufferpool m_srcPool;
    DMbufferpool m_dstPool;
    int m_bufferSize;

    char *m_compressedData;
    size_t m_compressedLength;
  };

}

#endif // __IRIXDMJPEGCOMPRESSOR_H__

