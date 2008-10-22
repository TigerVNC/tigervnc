#ifndef __JPEGCOMPRESSOR_H__
#define __JPEGCOMPRESSOR_H__

#include <stdio.h>
#include <sys/types.h>
extern "C" {
#include <jpeglib.h>
}

#include <rdr/types.h>
#include <rfb/PixelFormat.h>

namespace rfb {

  //
  // An abstract interface for performing JPEG compression.
  //

  class JpegCompressor
  {
  public:
    virtual ~JpegCompressor() {}

    // Set JPEG quality level (0..100)
    virtual void setQuality(int level) = 0;

    // Actually compress an image.
    virtual void compress(const rdr::U32 *buf, const PixelFormat *fmt,
                          int w, int h, int stride) = 0;

    // Access results of the compression.
    virtual size_t getDataLength() = 0;
    virtual const char *getDataPtr() = 0;
  };

  //
  // A C++ class for performing JPEG compression via the
  // Independent JPEG Group's software (free JPEG library).
  //

  class StandardJpegCompressor : public JpegCompressor
  {
  public:
    StandardJpegCompressor();
    virtual ~StandardJpegCompressor();

    // Set JPEG quality level (0..100)
    virtual void setQuality(int level);

    // Actually compress the image.
    virtual void compress(const rdr::U32 *buf, const PixelFormat *fmt,
                          int w, int h, int stride);

    // Access results of the compression.
    virtual size_t getDataLength() { return m_cdata_ready; }
    virtual const char *getDataPtr() { return (const char *)m_cdata; }

  public:
    // Our implementation of JPEG destination manager. These three
    // functions should never be called directly. They are made public
    // because they should be accessible from C-compatible functions
    // called by the JPEG library.
    void initDestination();
    bool emptyOutputBuffer();
    void termDestination();

  protected:
    static const int ALLOC_CHUNK_SIZE;
    static const int DEFAULT_QUALITY;

    struct jpeg_compress_struct m_cinfo;
    struct jpeg_error_mgr m_jerr;

    unsigned char *m_cdata;
    size_t m_cdata_allocated;
    size_t m_cdata_ready;
  };

}

#endif // __JPEGCOMPRESSOR_H__

