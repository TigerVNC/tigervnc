#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdint.h>

#include <stdexcept>

#include <glib.h>
#include <glib-object.h>

#include <pipewire/pipewire.h>
#include <pipewire/loop.h>
#include <pipewire/properties.h>
#include <pipewire/stream.h>

#include <spa/buffer/buffer.h>
#include <spa/buffer/meta.h>
#include <spa/param/video/raw.h>
#include <spa/param/latency-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/pod/pod.h>
#include <spa/utils/defs.h>
#include <spa/utils/type.h>
#include <spa/debug/format.h>
#include <spa/debug/pod.h>

#include <core/LogWriter.h>
#include <rfb/VNCServer.h>
#include <rfb/PixelFormat.h>

#include "PipeWireSource.h"
#include "PipeWirePixelBuffer.h"


static core::LogWriter vlog("PipewirePixelBuffer");


PipeWirePixelBuffer::PipeWirePixelBuffer(int32_t pipewireFd,
                                         uint32_t pipewireId,
                                         rfb::VNCServer* server)
  : pipewireFd_(pipewireFd), pipewireId_(pipewireId), server_(server)
{

  source = new PipeWireSource(this);
}

PipeWirePixelBuffer::~PipeWirePixelBuffer()
{
   delete source;
   close(pipewireFd_);
}

void PipeWirePixelBuffer::updatePixelbuffer(int width, int height,
                                            rfb::PixelFormat pf)
{
  setSize(width, height);
  setPF(pf);
  server_->setPixelBuffer(this);
}

void PipeWirePixelBuffer::processBuffer(pw_buffer* buffer)
{
  spa_buffer* buf;
  buf = buffer->buffer;
  int srcStride;
  uint8_t* srcBuffer;

  srcBuffer = (uint8_t*)buf->datas[0].data;
  srcStride = buf->datas[0].chunk->stride / (getPF().bpp / 8);

  imageRect(getPF(), {0, 0, width(), height()}, srcBuffer, srcStride);

  server_->add_changed({{0, 0, width(), height()}});
}

rfb::PixelFormat PipeWirePixelBuffer::convertPixelformat(int format_)
{
  switch (format_) {
  case SPA_VIDEO_FORMAT_BGRx:
  case SPA_VIDEO_FORMAT_BGRA:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            8, 16, 24);
  case SPA_VIDEO_FORMAT_RGBx:
  case SPA_VIDEO_FORMAT_RGBA:
    return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                            24, 16, 8);
  case SPA_VIDEO_FORMAT_xBGR:
  case SPA_VIDEO_FORMAT_ABGR:
    return rfb::PixelFormat(32, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  case SPA_VIDEO_FORMAT_xRGB:
  case SPA_VIDEO_FORMAT_ARGB:
    return rfb::PixelFormat(32, 24, false, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_RGB:
    return rfb::PixelFormat(24, 24, false, true, 255, 255, 255,
                            16, 8, 0);
  case SPA_VIDEO_FORMAT_BGR:
    return rfb::PixelFormat(24, 24, true, true, 255, 255, 255,
                            0, 8, 16);
  default:
    throw std::runtime_error("Unsupported pixel format");
  }
}
