extern "C" {
#include <libavutil/imgutils.h>
#include <libavcodec/version.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
}

#include <rfb/LogWriter.h>
#include <rfb/PixelBuffer.h>
#include <rfb/H264LibavEncoderContext.h>

// write to file debug
// #define __DEBUG_FILE_OUTPUT

using namespace rfb;

static LogWriter vlog("H264LibavEncoderContext");
#ifdef __DEBUG_FILE_OUTPUT
static int frame_cnt = 0;
static void save_gray_frame(unsigned char* buf, int wrap, int xsize, int ysize,
                            FILE* f);
static void save_rgb_frame(const unsigned char* buf, int wrap, int xsize,
                           int ysize, FILE* f);

FILE* yuv_file = NULL;
FILE* rgb_file = NULL;
FILE* h_file = NULL;
#endif

bool H264LibavEncoderContext::initCodec(int width, int height)
{
  os::AutoMutex lock(&mutex);

  const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
  if (!codec) {
    vlog.debug("Cannot find encoder");
    return false;
  }

  avctx = avcodec_alloc_context3(codec);
  if (!avctx) {
    vlog.debug("Cannot alloc context");
    return false;
  }

  yuv_frame = av_frame_alloc();
  if (!yuv_frame) {
    avcodec_free_context(&avctx);
    vlog.debug("Cannot alloc frame");
    return false;
  }

  // set context
  avctx->bit_rate = 400000;
  avctx->width = ceil((double)width / 2) * 2;
  avctx->height = ceil((double)height / 2) * 2;
  avctx->pix_fmt = AV_PIX_FMT_YUV420P;
  avctx->time_base.num = 1;
  avctx->time_base.den = 25;
  avctx->framerate.num = 25;
  avctx->framerate.den = 1;
  avctx->max_b_frames = 0;
  avctx->profile = FF_PROFILE_H264_MAIN;
  avctx->level = 30;

  int ret = avcodec_open2(avctx, codec, nullptr);
  if (ret < 0) {
    char szMsg[256] = {0};
    sprintf(szMsg, "err:%s\n", av_err2str(ret));
    avcodec_free_context(&avctx);
    av_frame_free(&yuv_frame);
    vlog.error("Could not open codec, ret %d, error(%s)", ret, szMsg);
    return false;
  }
  sws = nullptr;
  initialized = true;
  return true;
}

void H264LibavEncoderContext::freeCodec()
{
  os::AutoMutex lock(&mutex);
  if (!initialized) {
    return;
  }
  avcodec_free_context(&avctx);
  av_frame_free(&yuv_frame);
  sws_freeContext(sws);
  initialized = false;
#ifdef __DEBUG_FILE_OUTPUT
  if (yuv_file) {
    fflush(yuv_file);
    fclose(yuv_file);
    yuv_file = NULL;
  }
  if (h_file) {
    fflush(h_file);
    fclose(h_file);
    h_file = NULL;
  }
  if (rgb_file) {
    fflush(rgb_file);
    fclose(rgb_file);
    rgb_file = NULL;
  }
#endif
}

void H264LibavEncoderContext::encode(rdr::OutStream* os, const PixelBuffer* pb)
{
  os::AutoMutex lock(&mutex);
  if (!initialized) {
    return;
  }

#ifdef __DEBUG_FILE_OUTPUT
  frame_cnt++;

  char yuvPath[256] = {0};
  sprintf(yuvPath, "out_yuv_%d.yuv", frame_cnt);
  if (yuv_file == NULL) {
    yuv_file = fopen(yuvPath, "wb");
    vlog.debug("fopen yuv new file yuvPath(%s)", yuvPath);
  }
  char rgbPath[256] = {0};
  sprintf(rgbPath, "out_rgb_%d.rgb", frame_cnt);
  if (rgb_file == NULL) {
    rgb_file = fopen(rgbPath, "wb");
    vlog.debug("fopen rgb new file rgbPath(%s)", rgbPath);
  }
#endif

  getYuvFrame(pb);

#ifdef __DEBUG_FILE_OUTPUT
  AVFrame* pFrame = yuv_frame;
  save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width,
                  pFrame->height, yuv_file);
#endif

  AVPacket* packet = av_packet_alloc();
  h264_encode(yuv_frame, packet, os);
  h264_encode(nullptr, packet, os);

  // encode end
  vlog.debug("encode end ");
  packet->size = 0;
  packet->data = nullptr;
  av_packet_free(&packet);
}
void H264LibavEncoderContext::h264_encode(AVFrame* frame, AVPacket* packet,
                                          rdr::OutStream* os)
{
#ifdef __DEBUG_FILE_OUTPUT
  char hPath[256] = {0};
  sprintf(hPath, "out_h264_%d.h264", frame_cnt);
  if (h_file == NULL) {
    h_file = fopen(hPath, "wb");
    vlog.debug("fopen h264 new file yuvPath(%s)", hPath);
  }
#endif

  int ret = avcodec_send_frame(avctx, frame);
  if (ret < 0) {
    char szMsg[256] = {0};
    sprintf(szMsg, "err:%s\n", av_err2str(ret));
    vlog.error("Error during encoder, ret %d, error %s", ret, szMsg);
    return;
  }
  while (ret >= 0) {
    ret = avcodec_receive_packet(avctx, packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    }
    else if (ret < 0) {
      vlog.debug(
          "Error during avcodec_receive_packet, ret != 0, ret %d, error %s",
          ret, av_err2str(ret));
      return;
    }
    if (packet->size) {
      os->writeU32(packet->size);
      os->writeU32(1U);
      os->writeBytes(packet->data, packet->size);
#ifdef __DEBUG_FILE_OUTPUT
      // write to file
      fwrite(packet->data, packet->size, 1, h_file);
#endif
    }
    vlog.debug("package_size(%d), package_data(%p), ret(%d)", packet->size,
               packet->data, ret);
    av_packet_unref(packet);
  }
}

void H264LibavEncoderContext::getYuvFrame(const PixelBuffer* pb)
{
  // init sws context
  // get yuv frame buffer
  yuv_frame->format = avctx->pix_fmt;
  yuv_frame->width = avctx->width;
  yuv_frame->height = avctx->height;
  int ret = av_frame_get_buffer(yuv_frame, 0);
  if (ret < 0) {
    vlog.error("Could not alloc yuv frame data");
    return;
  }

  // get rgb frame
  AVFrame* rgbFrame = av_frame_alloc();
  rgbFrame->format = AV_PIX_FMT_RGB32;
  rgbFrame->width = avctx->width;
  rgbFrame->height = avctx->height;
  ret = av_frame_get_buffer(rgbFrame, 0);
  if (ret < 0) {
    vlog.error("Could not allocate the video rgb frame data");
    return;
  }

  // copy pb to rgbFrame
  int stride; // in pixels
  const uint8_t* buffer;
  int line_bytes = pb->width() * pb->getPF().bpp / 8;
  buffer = pb->getBuffer(pb->getRect(), &stride);
  int stride_bytes = stride * pb->getPF().bpp / 8;
  if (rgbFrame->linesize[0] == stride_bytes) {
    // no line padding
    vlog.debug("pb->rgbFrame: memcpy");
    memcpy(rgbFrame->data[0], buffer, pb->height() * stride_bytes);
  }
  else {
    // has line padding
    vlog.debug("pb->rgbFrame: line copy");
    int h = pb->height();
    uint8_t* dst = rgbFrame->data[0];
    const uint8_t* src = buffer;
    while (h--) {
      memcpy(dst, src, line_bytes);
      dst += rgbFrame->linesize[0];
      src += stride_bytes;
    }
  }
  if (pb->height() != avctx->height) {
    // fill extra even line
    memset(rgbFrame->data[0] + pb->height() * rgbFrame->linesize[0], 0,
           line_bytes);
  }
  vlog.debug("rgbFrame->linesize[0]:(%d), line_bytes:(%d)",
             rgbFrame->linesize[0], line_bytes);

#ifdef __DEBUG_FILE_OUTPUT
  // save rgb_file
  /*   save_rgb_frame(rgbFrame->data[0], rgbFrame->linesize[0], rgbFrame->width,
                 rgbFrame->height, rgb_file); */
  save_rgb_frame(buffer, stride_bytes, pb->width(), pb->height(), rgb_file);
#endif

  sws = sws_getCachedContext(sws, pb->width(), pb->height(), AV_PIX_FMT_RGB32,
                             avctx->width, avctx->height, avctx->pix_fmt,
                             SWS_BICUBIC, NULL, NULL, NULL);

  vlog.debug("pb->width:(%d), ->height:(%d), ->stride:(%d), "
             "avctx->width:(%d), ->height(%d)",
             pb->width(), pb->height(), stride, avctx->width, avctx->height);
  ret = sws_scale(sws, rgbFrame->data, rgbFrame->linesize, 0, pb->height(),
                  yuv_frame->data, yuv_frame->linesize);

  if (ret < 0) {
    vlog.error("sws_cale failed, ret: %d", ret);
  }
  vlog.debug("GetYuvFrame frame->data[0](%p), [1](%p), [2](%p), "
             "linsize[0](%d), [1](%d), [2](%d), format(%d)",
             yuv_frame->data[0], yuv_frame->data[1], yuv_frame->data[2],
             yuv_frame->linesize[0], yuv_frame->linesize[1],
             yuv_frame->linesize[2], rgbFrame->format);
  av_frame_free(&rgbFrame);
}

#ifdef __DEBUG_FILE_OUTPUT
static void save_gray_frame(unsigned char* buf, int wrap, int xsize, int ysize,
                            FILE* f)
{
  int i;
  // writing the minimal required header for a pgm file format
  // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

  // writing line by line
  for (i = 0; i < ysize; i++)
    fwrite(buf + i * wrap, 1, xsize, f);
}

static void save_rgb_frame(const unsigned char* buf, int wrap, int xsize,
                           int ysize, FILE* f)
{
  int i;
  // writing the minimal required header for a pgm file format
  // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
  fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);

  // writing line by line
  for (i = 0; i < ysize; i++) {
    for (int j = 0; j < 4 * xsize; j++) {
      if (j % 4 == 0)
        continue;
      const unsigned char* p = buf + i * wrap + j;
      fwrite(p, 1, 1, f);
    }
  }
}
#endif