/* Copyright 2026 jose-pr
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

#ifndef __AUDIOOUTPUTPULSE_H__
#define __AUDIOOUTPUTPULSE_H__

#include <sys/time.h>

#include <pulse/pulseaudio.h>

#include "AudioOutput.h"

class AudioOutputPulse : public AudioOutput
{
public:
  AudioOutputPulse();
  ~AudioOutputPulse() override;

  bool isAvailable() const { return available; }

  void start() override;
  void stop() override;
  void play(const uint8_t* samples, size_t length) override;

private:
  bool connect();
  bool open();
  size_t getSampleSize() const;
  void addSilence(size_t samples);
  void addSamples(const uint8_t* samples, size_t length);
  void submit();
  void setError(const char* message);
  void reportError();

  static void contextStateCallback(pa_context* context, void* userdata);
  static void streamStateCallback(pa_stream* stream, void* userdata);
  static void streamWriteCallback(pa_stream* stream, size_t length,
                                  void* userdata);
  static void streamUnderflowCallback(pa_stream* stream, void* userdata);
  static void timeoutCallback(pa_mainloop_api* api, pa_time_event* event,
                              const struct timeval* tv, void* userdata);

  bool available, opened, timedOut;

  pa_threaded_mainloop* mainloop;
  pa_context* context;
  pa_stream* stream;

  // Circular buffer of samples handed to us but not yet written to the
  // sound server. Its size is always a power of two, which the wrapping
  // arithmetic relies on.
  //
  // This and everything below it is touched from both the main thread
  // and the mainloop's own thread, so only ever with the mainloop lock
  // held.
  uint8_t* buffer;
  size_t bufferSize;
  size_t bufferFree;
  size_t pendingSize;
  size_t writtenHead;
  size_t pendingHead;

  unsigned long long streamId;
  unsigned extraDelayMs;

  bool starved;
  unsigned long long starvedAt;
  unsigned long long starvedStreamId;

  const char* pendingError;
  int pendingErrorCode;
};

#endif
