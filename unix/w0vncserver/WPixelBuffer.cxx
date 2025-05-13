#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/PixelBuffer.h>
#include <core/Region.h>

#include "WPixelBuffer.h"

static const rfb::PixelFormat pfBGRX(32, 24, false, true, 255, 255, 255,
                                     16, 8, 0);

WPixelBuffer::WPixelBuffer(int width, int height, uint8_t* _data, int
                           _stride)
  : FullFramePixelBuffer()
{
  // FIXME: Don't hardoce BGRx
  format = pfBGRX;
  setBuffer(width, height, _data, _stride);
}

WPixelBuffer::~WPixelBuffer()
{
}