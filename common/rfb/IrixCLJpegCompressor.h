#ifndef __IRIXCLJPEGCOMPRESSOR_H__
#define __IRIXCLJPEGCOMPRESSOR_H__

#include <sys/types.h>
#include <dmedia/cl.h>

#include <rdr/types.h>
#include <rfb/PixelFormat.h>

#include <rfb/JpegCompressor.h>

namespace rfb {

  //
  // A C++ class for performing JPEG compression.
  // This implementation uses IRIX Compression Library (CL).
  //

  class IrixCLJpegCompressor : public JpegCompressor
  {
  public:
    IrixCLJpegCompressor();
    virtual ~IrixCLJpegCompressor();

    // Check if the object has been created successfully.
    bool isValid() const { return m_clHandleValid; }

    // Set JPEG quality level (0..100).
    virtual void setQuality(int level);

    // Actually compress the image.
    virtual void compress(const rdr::U32 *buf, const PixelFormat *fmt,
			  int w, int h, int stride);

    // Access results of the compression.
    virtual size_t getDataLength() { return m_compressedLength; }
    virtual const char *getDataPtr() { return m_compressedData; }

  protected:
    static const int DEFAULT_QUALITY;
    int m_quality;

    CLhandle m_clHandle;
    bool m_clHandleValid;

    int m_srcBufferSize;
    int m_dstBufferSize;

    rdr::U32 *m_sourceData;
    char *m_compressedData;

    size_t m_compressedLength;
  };

}

#endif // __IRIXCLJPEGCOMPRESSOR_H__

