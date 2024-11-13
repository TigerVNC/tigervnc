#ifndef __RFB_H264ENCODER_H__
#define __RFB_H264ENCODER_H__

#include <rfb/Encoder.h>

namespace rfb {
  class H264Encoder : public Encoder {
  public:
    H264Encoder(SConnection* conn);
    virtual ~H264Encoder();
    bool isSupported() override;
    void writeRect(const PixelBuffer* pb, const Palette& palette) override;
    void writeSolidRect(int width, int height, const PixelFormat& pf,
                        const uint8_t* colour) override;
  };
}
#endif
