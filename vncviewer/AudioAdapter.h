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

#ifndef __AUDIOADAPTER_H__
#define __AUDIOADAPTER_H__

#include <vector>

#include <sys/types.h>

#include "AudioOutput.h"
#include "DLL.h"
#include "resampler.h"

class AudioAdapter
{
public:
  AudioAdapter();
  ~AudioAdapter();

  void reset();

  double updateDelay(ssize_t delay, size_t targetDelay);

  const std::vector<uint8_t>& process(const uint8_t* samples,
                                      size_t length);

  const std::vector<uint8_t>& flush();

  double getRate() const { return rate; }

private:
  void convert(size_t frames);

  bool resamplerReady;
  double rate;
  struct resample resampler;
  DLL dll;
  size_t dllPeriod;
  size_t pendingFrames;

  std::vector<float> in[audioChannels];
  std::vector<float> out[audioChannels];
  std::vector<uint8_t> buffer;
};

#endif
