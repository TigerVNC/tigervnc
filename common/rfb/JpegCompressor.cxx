#include <stdlib.h>

#include <rfb/JpegCompressor.h>

using namespace rfb;

const int StandardJpegCompressor::ALLOC_CHUNK_SIZE = 65536;
const int StandardJpegCompressor::DEFAULT_QUALITY = 75;

//
// Extend jpeg_destination_mgr struct with a pointer to our object.
//

typedef struct {
  struct jpeg_destination_mgr pub;
  StandardJpegCompressor *_this;
} my_destination_mgr;

//
// C-compatible interface to our destination manager. It just obtains
// a pointer to the right object and calls a corresponding C++ member
// function on that object.
//

static void
init_destination(j_compress_ptr cinfo)
{
  my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
  dest_ptr->_this->initDestination();
}

static boolean
empty_output_buffer (j_compress_ptr cinfo)
{
  my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
  return (boolean)dest_ptr->_this->emptyOutputBuffer();
}

static void
term_destination (j_compress_ptr cinfo)
{
  my_destination_mgr *dest_ptr = (my_destination_mgr *)cinfo->dest;
  dest_ptr->_this->termDestination();
}

//
// Constructor and destructor.
//

StandardJpegCompressor::StandardJpegCompressor()
  : m_cdata(0),
    m_cdata_allocated(0),
    m_cdata_ready(0)
{
  // Initialize JPEG compression structure.
  m_cinfo.err = jpeg_std_error(&m_jerr);
  jpeg_create_compress(&m_cinfo);

  // Set up a destination manager.
  my_destination_mgr *dest = new my_destination_mgr;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->_this = this;
  m_cinfo.dest = (jpeg_destination_mgr *)dest;

  // Set up a destination manager.
  m_cinfo.input_components = 3;
  m_cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults(&m_cinfo);
  jpeg_set_quality(&m_cinfo, DEFAULT_QUALITY, true);

  // We prefer speed over quality.
  m_cinfo.dct_method = JDCT_FASTEST;
}

StandardJpegCompressor::~StandardJpegCompressor()
{
  // Free compressed data buffer.
  if (m_cdata)
    free(m_cdata);

  // Clean up the destination manager.
  delete m_cinfo.dest;
  m_cinfo.dest = NULL;

  // Release the JPEG compression structure.
  jpeg_destroy_compress(&m_cinfo);
}

//
// Our implementation of destination manager.
//

void
StandardJpegCompressor::initDestination()
{
  if (!m_cdata) {
    size_t new_size = ALLOC_CHUNK_SIZE;
    m_cdata = (unsigned char *)malloc(new_size);
    m_cdata_allocated = new_size;
  }

  m_cdata_ready = 0;
  m_cinfo.dest->next_output_byte = m_cdata;
  m_cinfo.dest->free_in_buffer =  m_cdata_allocated;
}

bool
StandardJpegCompressor::emptyOutputBuffer()
{
  size_t old_size = m_cdata_allocated;
  size_t new_size = old_size + ALLOC_CHUNK_SIZE;

  m_cdata = (unsigned char *)realloc(m_cdata, new_size);
  m_cdata_allocated = new_size;

  m_cinfo.dest->next_output_byte = &m_cdata[old_size];
  m_cinfo.dest->free_in_buffer = new_size - old_size;

  return true;
}

void
StandardJpegCompressor::termDestination()
{
  m_cdata_ready = m_cdata_allocated - m_cinfo.dest->free_in_buffer;
}

//
// Set JPEG quality level (0..100)
//

void
StandardJpegCompressor::setQuality(int level)
{
  if (level < 0) {
    level = 0;
  } else if (level > 100) {
    level = 100;
  }
  jpeg_set_quality(&m_cinfo, level, true);
}

//
// Perform JPEG compression.
//
// FIXME: This function assumes that (fmt->bpp == 32 &&
//        fmt->depth == 24 && fmt->redMax == 255 &&
//        fmt->greenMax == 255 && fmt->blueMax == 255).
//

void
StandardJpegCompressor::compress(const rdr::U32 *buf,
                                 const PixelFormat *fmt,
                                 int w, int h, int stride)
{
  m_cinfo.image_width = w;
  m_cinfo.image_height = h;

  jpeg_start_compress(&m_cinfo, TRUE);

  const rdr::U32 *src = buf;

  // We'll pass up to 8 rows to jpeg_write_scanlines().
  JSAMPLE *rgb = new JSAMPLE[w * 3 * 8];
  JSAMPROW row_pointer[8];
  for (int i = 0; i < 8; i++)
    row_pointer[i] = &rgb[w * 3 * i];

  // Feed the pixels to the JPEG library.
  while (m_cinfo.next_scanline < m_cinfo.image_height) {
    int max_rows = m_cinfo.image_height - m_cinfo.next_scanline;
    if (max_rows > 8) {
      max_rows = 8;
    }
    for (int dy = 0; dy < max_rows; dy++) {
      JSAMPLE *dst = row_pointer[dy];
      for (int x = 0; x < w; x++) {
        dst[x*3]   = (JSAMPLE)(src[x] >> fmt->redShift);
        dst[x*3+1] = (JSAMPLE)(src[x] >> fmt->greenShift);
        dst[x*3+2] = (JSAMPLE)(src[x] >> fmt->blueShift);
      }
      src += stride;
    }
    jpeg_write_scanlines(&m_cinfo, row_pointer, max_rows);
  }

  delete[] rgb;

  jpeg_finish_compress(&m_cinfo);
}

