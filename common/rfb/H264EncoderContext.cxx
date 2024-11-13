#include <os/Mutex.h>
#include <rfb/LogWriter.h>
#include <rfb/H264EncoderContext.h>
#include <rfb/H264LibavEncoderContext.h>

using namespace rfb;

static LogWriter vlog("H264EncoderContext");

H264EncoderContext* H264EncoderContext::createContext(int width, int height)
{
  H264EncoderContext* ret = new H264LibavEncoderContext();
  if (!ret->initCodec(width, height)) {
    throw std::runtime_error("H264EncoderContext: Unable to create context");
  }
  return ret;
}

H264EncoderContext::~H264EncoderContext()
{
}

bool H264EncoderContext::isReady()
{
  os::AutoMutex lock(&mutex);
  return initialized;
}

void H264EncoderContext::reset()
{
  freeCodec();
}
