#ifndef __RFB_H264ENCODERCONTEXT_H__
#define __RFB_H264ENCODERCONTEXT_H__

#include <stdint.h>

#include <os/Mutex.h>
#include <rfb/Rect.h>
#include <rfb/Encoder.h>
#include <rdr/OutStream.h>

namespace rfb {
  class H264EncoderContext {
  public:
    static H264EncoderContext* createContext(int width, int height);
    virtual ~H264EncoderContext() = 0;
    virtual void encode(rdr::OutStream* os, const PixelBuffer* pb) = 0;
    void reset();
    bool isReady();

  protected:
    os::Mutex mutex;
    bool initialized;

    H264EncoderContext()
    {
      initialized = false;
    }

    virtual bool initCodec(int width, int height) = 0;
    virtual void freeCodec() = 0;
  };
}

#endif