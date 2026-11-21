/* Copyright 2026 Andri Yngvason
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

// This is inspired by RTP receiver in PipeWire.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <core/LogWriter.h>
#include <core/i18n.h>

#include <rfb/qemuTypes.h>

#include "AudioAdapter.h"

static core::LogWriter vlog("AudioAdapter");

static const double maxErrorMs = 2.0;

static_assert(audioSampleFormat == rfb::qemuAudioFormatS16,
              "AudioAdapter only handles signed 16 bit samples");

static const size_t sampleSize = audioChannels * sizeof(int16_t);

static inline double clampf(double v, double low, double high)
{
  if (v < low)
    return low;
  if (v > high)
    return high;
  return v;
}

static void s16ToFloat(float* dst, const uint8_t* src,
                       size_t stride, size_t frames)
{
  for (size_t i = 0; i < frames; i++) {
    int16_t sample;
    memcpy(&sample, src, sizeof(sample));
    dst[i] = sample / 32768.0;
    src += stride;
  }
}

static void s16FromFloat(uint8_t* dst, const float* src,
                         size_t stride, size_t frames)
{
  for (size_t i = 0; i < frames; i++) {
    int16_t sample;
    // resampler can overshoot
    sample = clampf(rint(src[i] * 32768.0), INT16_MIN, INT16_MAX);
    memcpy(dst, &sample, sizeof(sample));
    dst += stride;
  }
}

AudioAdapter::AudioAdapter()
  : resamplerReady(false), rate(1.0), dllPeriod(0), pendingFrames(0)
{
  int ret;

  memset(&resampler, 0, sizeof(resampler));

  resampler.channels = audioChannels;
  resampler.i_rate = audioFrequency;
  resampler.o_rate = audioFrequency;
  resampler.quality = RESAMPLE_DEFAULT_QUALITY;

  ret = resample_init(&resampler);
  if (ret < 0) {
    vlog.error(_("Could not set up audio resampler: %s"), strerror(-ret));
    return;
  }

  resamplerReady = true;
}

AudioAdapter::~AudioAdapter()
{
  if (resamplerReady)
    resample_free(&resampler);
}

void AudioAdapter::convert(size_t frames)
{
  size_t consumed;

  buffer.clear();

  consumed = 0;
  while (consumed < frames) {
    uint32_t inLen, outLen;
    const void* src[audioChannels];
    void* dst[audioChannels];
    size_t offset;

    inLen = frames - consumed;
    outLen = resample_out_len(&resampler, inLen);

    for (unsigned c = 0; c < audioChannels; c++) {
      if (out[c].size() < outLen)
        out[c].resize(outLen);
      src[c] = in[c].data() + consumed;
      dst[c] = out[c].data();
    }

    resample_process(&resampler, src, &inLen, dst, &outLen);

    if (outLen > 0) {
      offset = buffer.size();
      buffer.resize(offset + outLen * sampleSize);
      for (unsigned c = 0; c < audioChannels; c++) {
        s16FromFloat(buffer.data() + offset + c * sizeof(int16_t),
                     out[c].data(), sampleSize, outLen);
      }
    }

    if ((inLen == 0) && (outLen == 0))
      break;

    consumed += inLen;
  }
}

void AudioAdapter::reset()
{
  if (resamplerReady)
    resample_reset(&resampler);

  pendingFrames = 0;
}

double AudioAdapter::updateDelay(ssize_t delay, size_t targetDelay)
{
  size_t period;
  double error, maxError, corr;

  if (!resamplerReady)
    return NAN;

  period = pendingFrames;
  pendingFrames = 0;

  if (delay < 0)
    return NAN;

  delay += resample_delay(&resampler);

  error = (double)targetDelay - (double)delay;

  if (period == 0)
    return error;

  if (period != dllPeriod) {
    dll.setBw(dllBwMin, period, audioFrequency);
    dllPeriod = period;
  }

  maxError = maxErrorMs * audioFrequency / 1000;

  corr = dll.update(clampf(error, -maxError, maxError));

  rate = 1.0 / corr;
  resample_update_rate(&resampler, rate);

  return error;
}

const std::vector<uint8_t>& AudioAdapter::process(const uint8_t* samples,
                                                  size_t length)
{
  size_t frames;

  if (!resamplerReady) {
    buffer.assign(samples, samples + length);
    return buffer;
  }

  frames = length / sampleSize;
  pendingFrames += frames;

  for (unsigned c = 0; c < audioChannels; c++) {
    in[c].resize(frames);
    s16ToFloat(in[c].data(), samples + c * sizeof(int16_t),
               sampleSize, frames);
  }

  convert(frames);

  return buffer;
}

const std::vector<uint8_t>& AudioAdapter::flush()
{
  size_t frames;

  if (!resamplerReady) {
    buffer.clear();
    return buffer;
  }

  // Silence pushes the last real samples through the filter
  frames = resample_delay(&resampler) + 1;

  for (unsigned c = 0; c < audioChannels; c++)
    in[c].assign(frames, 0.0f);

  convert(frames);

  return buffer;
}
