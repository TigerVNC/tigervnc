#ifndef __RFB_H264LIBAVEnCODER_H__
#define __RFB_H264LIBAVEnCODER_H__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <rfb/H264EncoderContext.h>
#include <rdr/OutStream.h>

namespace rfb {
  class H264LibavEncoderContext : public H264EncoderContext {
  public:
    H264LibavEncoderContext() : H264EncoderContext() {}
    ~H264LibavEncoderContext() { freeCodec(); }

    void h264_encode(AVFrame* frame, AVPacket* packet, rdr::OutStream* os);
    virtual void encode(rdr::OutStream* os, const PixelBuffer* pb) override;
    void getYuvFrame(const PixelBuffer* pb);

  protected:
    virtual bool initCodec(int width, int height) override;
    virtual void freeCodec() override;

  private:
    AVCodecContext* avctx;
    AVFrame* yuv_frame;
    SwsContext* sws;
  };
}

#endif
