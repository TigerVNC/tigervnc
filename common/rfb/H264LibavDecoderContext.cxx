/* Copyright (C) 2021 Vladimir Sukhonosov <xornet@xornet.org>
 * Copyright (C) 2021 Martins Mozeiko <martins.mozeiko@gmail.com>
 * All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <new>
#include <stdexcept>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavcodec/version.h>
}
#if LIBAVCODEC_VERSION_MAJOR > 57 || LIBAVCODEC_VERSION_MAJOR == 57 && LIBAVCODEC_VERSION_MINOR >= 37
#define FFMPEG_DECODE_VIDEO2_DEPRECATED
#endif
#if LIBAVCODEC_VERSION_MAJOR >= 58
#define FFMPEG_INIT_PACKET_DEPRECATED
#endif

#include <rfb/PixelBuffer.h>
#include <rfb/H264LibavDecoderContext.h>

using namespace rfb;

H264LibavDecoderContext::H264LibavDecoderContext(const core::Rect& r)
  : H264DecoderContext(r)
{
  sws = nullptr;
  h264WorkBuffer = nullptr;
  h264WorkBufferLength = 0;

  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if (!codec)
    throw std::runtime_error("Codec not found");

  parser = av_parser_init(codec->id);
  if (!parser)
    throw std::runtime_error("Could not create H264 parser");

  avctx = avcodec_alloc_context3(codec);
  if (!avctx)
  {
    av_parser_close(parser);
    throw std::runtime_error("Could not allocate video codec context");
  }

  frame = av_frame_alloc();
  if (!frame)
  {
    av_parser_close(parser);
    avcodec_free_context(&avctx);
    throw std::runtime_error("Could not allocate video frame");
  }

  if (avcodec_open2(avctx, codec, nullptr) < 0)
  {
    av_parser_close(parser);
    avcodec_free_context(&avctx);
    av_frame_free(&frame);
    throw std::runtime_error("Could not open video codec");
  }
}

H264LibavDecoderContext::~H264LibavDecoderContext()
{
  av_parser_close(parser);
  avcodec_free_context(&avctx);
  av_frame_free(&rgbFrame);
  av_frame_free(&frame);
  sws_freeContext(sws);
  free(h264WorkBuffer);
}

// We need to reallocate buffer because AVPacket uses non-const pointer.
// We don't want to const_cast our buffer somewhere. So we would rather to maintain context's own buffer
// Also avcodec requires a right padded buffer
uint8_t* H264LibavDecoderContext::makeH264WorkBuffer(const uint8_t* buffer, uint32_t len)
{
  uint32_t reserve_len = len + len % AV_INPUT_BUFFER_PADDING_SIZE;

  if (!h264WorkBuffer || reserve_len > h264WorkBufferLength)
  {
    h264WorkBuffer = (uint8_t*)realloc(h264WorkBuffer, reserve_len);
    if (h264WorkBuffer == nullptr) {
      throw std::bad_alloc();
    }
    h264WorkBufferLength = reserve_len;
  }

  memcpy(h264WorkBuffer, buffer, len);
  memset(h264WorkBuffer + len, 0, h264WorkBufferLength - len);
  return h264WorkBuffer;
}

void H264LibavDecoderContext::decode(const uint8_t* h264_in_buffer,
                                     uint32_t len,
                                     ModifiablePixelBuffer* pb) {
  uint8_t* h264_work_buffer = makeH264WorkBuffer(h264_in_buffer, len);

#ifdef FFMPEG_INIT_PACKET_DEPRECATED
  AVPacket *packet = av_packet_alloc();
#else
  AVPacket *packet = new AVPacket();
  av_init_packet(packet);
#endif

  int ret;
  int frames_received = 0;
  while (len)
  {
    ret = av_parser_parse2(parser, avctx, &packet->data, &packet->size, h264_work_buffer, len, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    // Silently ignore errors, hoping its a temporary encoding glitch
    if (ret < 0)
      break;
    // We need to slap on tv to make it work here (don't ask me why)
    if (!packet->size && len == static_cast<uint32_t>(ret))
      ret = av_parser_parse2(parser, avctx, &packet->data, &packet->size, h264_work_buffer, len, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    // Silently ignore errors, hoping its a temporary encoding glitch
    if (ret < 0)
      break;
    h264_work_buffer += ret;
    len -= ret;

    if (!ret)
    {
      packet->size = len;
      packet->data = h264_work_buffer;
      len = 0;
    }

    if (!packet->size)
      continue;

#ifndef FFMPEG_DECODE_VIDEO2_DEPRECATED
    int got_frame;
    ret = avcodec_decode_video2(avctx, frame, &got_frame, packet);
    // Silently ignore errors, hoping its a temporary encoding glitch
    if (ret < 0 || !got_frame)
      break;
#else
    ret = avcodec_send_packet(avctx, packet);
    // Silently ignore errors, hoping its a temporary encoding glitch
    if (ret < 0)
      break;

    ret = avcodec_receive_frame(avctx, frame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    // Silently ignore errors, hoping its a temporary encoding glitch
    if (ret < 0)
      break;
#endif
    frames_received++;
  }

#ifdef FFMPEG_INIT_PACKET_DEPRECATED
  packet->size = 0;
  packet->data = nullptr;
  av_packet_free(&packet);
#else
  delete packet;
#endif

  if (!frames_received)
    return;

  if (!frame->height)
    return;

  sws = sws_getCachedContext(sws, frame->width, frame->height, avctx->pix_fmt,
                             frame->width, frame->height, AV_PIX_FMT_RGB32,
                             SWS_POINT, nullptr, nullptr, nullptr);

  int inFull, outFull, brightness, contrast, saturation;
  const int* inTable;
  const int* outTable;

  sws_getColorspaceDetails(sws, (int**)&inTable, &inFull, (int**)&outTable,
      &outFull, &brightness, &contrast, &saturation);
  if (frame->colorspace != AVCOL_SPC_UNSPECIFIED) {
    inTable = sws_getCoefficients(frame->colorspace);
  }
  if (frame->color_range != AVCOL_RANGE_UNSPECIFIED) {
    inFull = frame->color_range == AVCOL_RANGE_JPEG;
  }
  sws_setColorspaceDetails(sws, inTable, inFull, outTable, outFull, brightness,
      contrast, saturation);

  if (rgbFrame && (rgbFrame->width != frame->width || rgbFrame->height != frame->height)) {
    av_frame_free(&rgbFrame);

  }

  if (!rgbFrame) {
    rgbFrame = av_frame_alloc();
    // TODO: Can we really assume that the pixel format will always be RGB32?
    rgbFrame->format = AV_PIX_FMT_RGB32;
    rgbFrame->width = frame->width;
    rgbFrame->height = frame->height;
    av_frame_get_buffer(rgbFrame, 0);
  }

  sws_scale(sws, frame->data, frame->linesize, 0, frame->height, rgbFrame->data,
            rgbFrame->linesize);

  pb->imageRect(rect, rgbFrame->data[0], rgbFrame->linesize[0] / 4);
}
