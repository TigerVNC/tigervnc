#include <rdr/OutStream.h>
#include <rfb/encodings.h>
#include <rfb/Exception.h>
#include <rfb/SConnection.h>
#include <rfb/PixelBuffer.h>
#include <rfb/LogWriter.h>
#include <rfb/H264EncoderContext.h>
#include <rfb/H264Encoder.h>

using namespace rfb;

static LogWriter vlog("H264Encoder");

H264Encoder::H264Encoder(SConnection* conn_)
  : Encoder(conn_, encodingH264, EncoderPlain)
{
}

H264Encoder::~H264Encoder()
{
}

bool H264Encoder::isSupported()
{
  vlog.debug("isSupported");
  return conn->client.supportsEncoding(encodingH264);
}

void H264Encoder::writeRect(const PixelBuffer* pb, const Palette& /*palette*/)
{
  H264EncoderContext* ctx =
    H264EncoderContext::createContext(pb->width(), pb->height());
  if (!ctx) {
    throw std::runtime_error("H264Encoder: Context not be created");
  }
  if (!ctx->isReady()) {
    throw std::runtime_error("H264Encoder: Context is not ready");
  }

  rdr::OutStream* os;
  os = conn->getOutStream();
  vlog.debug("writeRect width(%d), height(%d), bpp(%d)", pb->width(),
             pb->height(), pb->getPF().bpp);
  ctx->encode(os, pb);
  ctx->reset();
}

void H264Encoder::writeSolidRect(int width, int height, const PixelFormat& pf,
                                 const uint8_t* colour)
{
  (void)colour;
  vlog.info("H264Encoder writeSolidRect, width %d, height %d, pf.bpp %d",
            width, height, pf.bpp);
}
