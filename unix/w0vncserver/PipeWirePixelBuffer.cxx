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

#include <pixman.h>

#include <core/LogWriter.h>
#include <rfb/VNCServer.h>
#include <rfb/PixelFormat.h>
#include <core/Rect.h>

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

  pixman_bool_t ret;
  spa_buffer* buf;
  buf = buffer->buffer;
  spa_meta_region* damage;
  int srcStride;
  int dstStride;
  uint8_t* srcBuffer;
  uint8_t* dstBuffer;

  srcBuffer = (uint8_t*)buf->datas[0].data;
  srcStride = buf->datas[0].chunk->stride / (getPF().bpp / 8);
  dstBuffer = getBufferRW({0, 0, width(), height()}, &dstStride);

  ret = pixman_blt((uint32_t*)srcBuffer, (uint32_t*)dstBuffer,
                   srcStride, dstStride, getPF().bpp,
                   getPF().bpp, 0, 0, 0, 0, width(), height());

  if (!ret)
    imageRect(getPF(), {0, 0, width(), height()}, srcBuffer, srcStride);

  server_->add_changed({{0, 0, width(), height()}});
  damage = (spa_meta_region *)spa_buffer_find_meta_data(buf,
                                                        SPA_META_VideoDamage,
                                                        sizeof(*damage));
  if (damage) {
    core::Point tl{damage->region.position.x, damage->region.position.y};
    core::Point br{static_cast<int>(damage->region.position.x + damage->region.size.width),
                   static_cast<int>(damage->region.position.y + damage->region.size.height)};
    server_->add_changed({{tl, br}});
  } else {
    server_->add_changed({{0, 0, width(), height()}});
  }
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
