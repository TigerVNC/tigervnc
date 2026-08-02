/* Copyright 2022 Mikhail Kupchik
 * Copyright 2026 jose-pr
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

#ifndef __AUDIOOUTPUT_H__
#define __AUDIOOUTPUT_H__

#include <stddef.h>
#include <stdint.h>

#include <rfb/qemuTypes.h>

// The format the server is asked for in CConnection, and which every
// backend therefore has to play
const uint8_t audioSampleFormat = rfb::qemuAudioFormatS16;
const uint8_t audioChannels = 2;
const uint32_t audioFrequency = 48000;

// How much audio to buffer ahead, and how much silence to play before
// a new stream, to hide variations in when samples arrive over the
// network. Neither depends on which sound system plays them.
const unsigned audioMaxJitterMs = 1000;
const unsigned audioMinStreamDelayMs = 20;

class AudioOutput
{
public:
  virtual ~AudioOutput() {}

  static AudioOutput* create();

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void play(const uint8_t* samples, size_t length) = 0;
};

#endif
