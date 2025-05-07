#ifndef __WPIXELBUFFER_H__
#define __WPIXELBUFFER_H__

#include <rfb/PixelBuffer.h>

class WPixelBuffer : public rfb::FullFramePixelBuffer
{
public:
  WPixelBuffer(int width, int height, uint8_t* data, int stride);
  virtual ~WPixelBuffer();
};

#endif // __WPIXELBUFFER_H__
