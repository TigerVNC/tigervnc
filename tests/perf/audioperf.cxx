/* This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org/>
 */

/*
 * Runs the AudioAdapter for audioperf.py, which simulates everything
 * around it. A text header first says what sample rate and delay the
 * adapter expects, and ends with "# input". Each packet on stdin is
 * then the delay the output reports as a signed 64 bit integer, a
 * frame count as an unsigned 32 bit one, and that many mono signed 16
 * bit samples. What the adapter makes of it is written back the same
 * way, but with the delay error the adapter worked out, as a double,
 * in place of the delay. A packet without any frames flushes the
 * adapter instead, and has NaN for the error. Everything is in native
 * byte order.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "../vncviewer/AudioAdapter.h"

static_assert(audioSampleFormat == rfb::qemuAudioFormatS16,
              "audioperf only handles signed 16 bit samples");

static const size_t frameSize = audioChannels * sizeof(int16_t);

int main()
{
  AudioAdapter adapter;
  size_t targetDelay;
  std::vector<int16_t> samples;
  std::vector<uint8_t> packet;

  targetDelay = audioMinStreamDelayMs * audioFrequency / 1000;

  printf("# param frequency %u\n", (unsigned)audioFrequency);
  printf("# param target %zu\n", targetDelay);
  printf("# input\n");
  fflush(stdout);

  adapter.reset();

  while (true) {
    int64_t delay;
    uint32_t frames;
    double error;

    if (fread(&delay, sizeof(delay), 1, stdin) != 1)
      break;
    if (fread(&frames, sizeof(frames), 1, stdin) != 1)
      return 1;

    samples.resize(frames);
    if (fread(samples.data(), sizeof(int16_t), frames, stdin) != frames)
      return 1;

    packet.resize(frames * frameSize);
    for (size_t i = 0; i < frames; i++) {
      for (unsigned c = 0; c < audioChannels; c++)
        memcpy(&packet[i * frameSize + c * sizeof(int16_t)],
               &samples[i], sizeof(int16_t));
    }

    error = NAN;
    if (frames > 0)
      error = adapter.updateDelay(delay, targetDelay);
    const std::vector<uint8_t>& out =
      frames > 0 ? adapter.process(packet.data(), packet.size())
                 : adapter.flush();

    frames = out.size() / frameSize;
    samples.resize(frames);
    for (size_t i = 0; i < frames; i++)
      memcpy(&samples[i], &out[i * frameSize], sizeof(int16_t));

    fwrite(&error, sizeof(error), 1, stdout);
    fwrite(&frames, sizeof(frames), 1, stdout);
    fwrite(samples.data(), sizeof(int16_t), frames, stdout);
    fflush(stdout);
  }

  return 0;
}
