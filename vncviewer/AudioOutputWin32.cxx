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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <core/LogWriter.h>
#include <core/i18n.h>

#include <rfb/qemuTypes.h>

#include "AudioOutputWin32.h"

static core::LogWriter vlog("AudioOutputWin32");

static void fillWaveFormat(WAVEFORMATEX* wfx)
{
  memset(wfx, 0, sizeof(*wfx));
  wfx->wFormatTag = WAVE_FORMAT_PCM;
  wfx->nChannels = audioChannels;
  wfx->nSamplesPerSec = audioFrequency;
  // Sample formats are ordered by width in pairs, unsigned then signed
  wfx->wBitsPerSample = 8 << (audioSampleFormat >> 1);
  wfx->nBlockAlign = audioChannels * (wfx->wBitsPerSample / 8);
  wfx->nAvgBytesPerSec = wfx->nSamplesPerSec * wfx->nBlockAlign;
  wfx->cbSize = 0;
}

AudioOutputWin32::AudioOutputWin32()
  : available(false), opened(false), device(nullptr),
    buffer(nullptr), bufferSize(0), bufferFree(0), pendingSize(0),
    writtenHead(0), pendingHead(0),
    streamId(0), extraDelayMs(0),
    doneBuffers(nullptr), buffersInFlight(0)
{
  WAVEFORMATEX wfx;

  fillWaveFormat(&wfx);

  if (waveOutOpen(nullptr, WAVE_MAPPER, &wfx, 0, 0,
                  CALLBACK_NULL | WAVE_FORMAT_QUERY) != MMSYSERR_NOERROR) {
    vlog.debug("No audio playback device supports the format we need");
    return;
  }

  available = true;
}

AudioOutputWin32::~AudioOutputWin32()
{
  Buffer* done;

  if (!opened)
    return;

  // Cancels anything still playing, so that every buffer we handed
  // over comes back before we free it
  waveOutReset(device);

  done = (Buffer*)InterlockedExchangePointer(&doneBuffers, nullptr);
  while (done != nullptr) {
    Buffer* next = done->next;
    waveOutUnprepareHeader(device, &done->whdr, sizeof(WAVEHDR));
    free(done);
    done = next;
  }

  waveOutClose(device);

  free(buffer);
}

size_t AudioOutputWin32::getSampleSize() const
{
  return audioChannels << (audioSampleFormat >> 1);
}

bool AudioOutputWin32::open()
{
  WAVEFORMATEX wfx;
  size_t samples, sampleSize;

  if (opened)
    return true;
  if (!available)
    return false;

  fillWaveFormat(&wfx);

  if (waveOutOpen(&device, WAVE_MAPPER, &wfx,
                  (DWORD_PTR)&AudioOutputWin32::waveOutCallback,
                  (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
    vlog.error(_("Could not open audio playback device"));
    available = false;
    return false;
  }

  // Round the sample count up to a power of two so that the wrapping
  // arithmetic below can be a mask. The sample size is a power of two
  // as well, so the byte size ends up being one too.
  samples = 1;
  while (samples < (4 * audioMaxJitterMs * audioFrequency) / 1000)
    samples <<= 1;

  sampleSize = getSampleSize();

  buffer = (uint8_t*)calloc(samples, sampleSize);
  if (buffer == nullptr) {
    waveOutClose(device);
    device = nullptr;
    available = false;
    return false;
  }

  bufferSize = bufferFree = samples * sampleSize;
  pendingSize = writtenHead = pendingHead = 0;

  opened = true;

  return true;
}

void AudioOutputWin32::addSilence(size_t samples)
{
  size_t left;

  left = samples * getSampleSize();

  while (left > 0) {
    size_t chunk = left;

    if (chunk > bufferFree)
      chunk = bufferFree;
    if (chunk > bufferSize - pendingHead)
      chunk = bufferSize - pendingHead;
    if (chunk == 0)
      break;

    memset(buffer + pendingHead,
           audioSampleFormat == rfb::qemuAudioFormatU8 ? 0x80 : 0x00,
           chunk);

    pendingHead = (pendingHead + chunk) & (bufferSize - 1);
    bufferFree -= chunk;
    pendingSize += chunk;
    left -= chunk;
  }
}

void AudioOutputWin32::addSamples(const uint8_t* samples, size_t length)
{
  // A partial sample is of no use to anyone, and would put every
  // channel after it in the wrong place
  length -= length % getSampleSize();

  while (length > 0) {
    size_t chunk = length;

    if (chunk > bufferFree)
      chunk = bufferFree;
    if (chunk > bufferSize - pendingHead)
      chunk = bufferSize - pendingHead;
    if (chunk == 0) {
      // We are further behind than the buffer is long, so the samples
      // we are dropping are ones we could never have played in time
      vlog.debug("Audio buffer full, discarding %d bytes", (int)length);
      break;
    }

    memcpy(buffer + pendingHead, samples, chunk);

    pendingHead = (pendingHead + chunk) & (bufferSize - 1);
    bufferFree -= chunk;
    pendingSize += chunk;
    samples += chunk;
    length -= chunk;
  }
}

unsigned long long AudioOutputWin32::getTimestamp()
{
  FILETIME now;
  ULARGE_INTEGER value;

  GetSystemTimeAsFileTime(&now);

  value.LowPart = now.dwLowDateTime;
  value.HighPart = now.dwHighDateTime;

  return value.QuadPart;
}

// Called by the system on a thread of its own once it is done with a
// buffer. None of the waveOut functions may be called from here, so all
// we do is put the buffer on a list for submit() to deal with.
void CALLBACK AudioOutputWin32::waveOutCallback(HWAVEOUT hwo, UINT msg,
                                                DWORD_PTR instance,
                                                DWORD_PTR param1,
                                                DWORD_PTR /*param2*/)
{
  AudioOutputWin32* self = (AudioOutputWin32*)instance;
  Buffer* buf = (Buffer*)param1;
  PVOID head;

  if (msg != WOM_DONE)
    return;
  if (!self->opened || (self->device != hwo))
    return;
  if (!(buf->whdr.dwFlags & WHDR_DONE))
    return;

  // Nothing left to play means the device ran dry waiting for us, and
  // how long it stays dry is how much further ahead we need to buffer
  if (InterlockedDecrement(&self->buffersInFlight) == 0) {
    buf->starved = true;
    buf->starvedAt = getTimestamp();
  }

  head = self->doneBuffers;
  while (true) {
    PVOID previous;

    InterlockedExchangePointer(&buf->volatileNext, head);
    previous = InterlockedCompareExchangePointer(&self->doneBuffers,
                                                 buf, head);
    if (previous == head)
      break;
    head = previous;
  }
}

void AudioOutputWin32::submit()
{
  Buffer* spare;

  if (!opened)
    return;

  spare = (Buffer*)InterlockedExchangePointer(&doneBuffers, nullptr);

  for (Buffer* buf = spare; buf != nullptr; buf = buf->next) {
    bufferFree += buf->whdr.dwBufferLength;
    waveOutUnprepareHeader(device, &buf->whdr, sizeof(WAVEHDR));

    if (buf->starved && (buf->streamId == streamId)) {
      unsigned long long now = getTimestamp();
      if (now > buf->starvedAt) {
        // FILETIME counts 100 ns intervals
        unsigned long long ms = (now - buf->starvedAt + 9999) / 10000;
        if (ms > audioMaxJitterMs)
          ms = audioMaxJitterMs;
        if (extraDelayMs < ms)
          extraDelayMs = (unsigned)ms;
      }
    }
  }

  while (pendingSize > 0) {
    Buffer* buf;
    size_t length;

    length = pendingSize;
    if (length > bufferSize - writtenHead)
      length = bufferSize - writtenHead;

    if (spare != nullptr) {
      buf = spare;
      spare = buf->next;
    } else {
      buf = (Buffer*)malloc(sizeof(Buffer));
      if (buf == nullptr)
        break;
    }

    memset(buf, 0, sizeof(Buffer));
    buf->whdr.lpData = (LPSTR)(buffer + writtenHead);
    buf->whdr.dwBufferLength = (DWORD)length;
    buf->streamId = streamId;

    if (waveOutPrepareHeader(device, &buf->whdr,
                             sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
      buf->next = spare;
      spare = buf;
      break;
    }

    // Has to be counted before the write, as the callback can run
    // before waveOutWrite() has even returned
    InterlockedIncrement(&buffersInFlight);

    if (waveOutWrite(device, &buf->whdr,
                     sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
      InterlockedDecrement(&buffersInFlight);
      waveOutUnprepareHeader(device, &buf->whdr, sizeof(WAVEHDR));
      buf->next = spare;
      spare = buf;
      break;
    }

    writtenHead = (writtenHead + length) & (bufferSize - 1);
    pendingSize -= length;
  }

  while (spare != nullptr) {
    Buffer* next = spare->next;
    free(spare);
    spare = next;
  }
}

void AudioOutputWin32::start()
{
  if (!open())
    return;

  streamId++;

  // Play a little silence first, so that a sample arriving later than
  // the one before it does not leave the device with nothing to play
  addSilence((audioMinStreamDelayMs + extraDelayMs) *
             audioFrequency / 1000);
  submit();
}

void AudioOutputWin32::stop()
{
  // Whatever is already buffered should still be played, so there is
  // nothing to do but let it drain
}

void AudioOutputWin32::play(const uint8_t* samples, size_t length)
{
  if (!opened)
    return;

  addSamples(samples, length);
  submit();
}
