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

#ifndef __AUDIOOUTPUTWIN32_H__
#define __AUDIOOUTPUTWIN32_H__

#include <windows.h>
// Not pulled in by windows.h if WIN32_LEAN_AND_MEAN is defined
#include <mmsystem.h>

#include "AudioOutput.h"

class AudioOutputWin32 : public AudioOutput
{
public:
  AudioOutputWin32();
  ~AudioOutputWin32() override;

  bool isAvailable() const { return available; }

  void start() override;
  void stop() override;
  void play(const uint8_t* samples, size_t length) override;

private:
  // One asynchronous write to the mixer, plus what we need to know
  // about it once it comes back
  struct Buffer {
    WAVEHDR whdr;
    unsigned long long streamId;
    bool starved;
    unsigned long long starvedAt;
    union {
      Buffer* next;
      PVOID volatile volatileNext;
    };
  };

  bool open();
  size_t getSampleSize() const;
  void addSilence(size_t samples);
  void addSamples(const uint8_t* samples, size_t length);
  void submit();

  static unsigned long long getTimestamp();
  static void CALLBACK waveOutCallback(HWAVEOUT hwo, UINT msg,
                                       DWORD_PTR instance,
                                       DWORD_PTR param1, DWORD_PTR param2);

  bool available, opened;
  HWAVEOUT device;

  // Circular buffer of samples handed to us but not yet written to the
  // device. Its size is always a power of two, which the wrapping
  // arithmetic relies on.
  uint8_t* buffer;
  size_t bufferSize;
  size_t bufferFree;
  size_t pendingSize;
  size_t writtenHead;
  size_t pendingHead;

  unsigned long long streamId;
  unsigned extraDelayMs;

  // Written by the device callback, which runs on a thread of the
  // system's choosing and may not call back in to it
  PVOID volatile doneBuffers;
  LONG volatile buffersInFlight;
};

#endif
