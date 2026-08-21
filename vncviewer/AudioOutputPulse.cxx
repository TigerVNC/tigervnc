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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <core/LogWriter.h>
#include <core/i18n.h>

#include <rfb/qemuTypes.h>

#include "AudioOutputPulse.h"

static core::LogWriter vlog("AudioOutputPulse");

// create() has to say whether audio is available before the viewer has
// connected to anything, so this wait is on the path to the first
// window. A sound server on the other end of a local socket answers in
// well under a millisecond; one that does not answer at all must not
// hold up the viewer.
static const unsigned connectTimeoutMs = 500;

// The wider RFB formats are in host endianness, which is exactly what
// PulseAudio's "NE" formats are, so this mapping holds on a big endian
// machine as well. There is no counterpart for signed 8 bit or unsigned
// 16 bit, but we never ask for either.
static bool fillSampleSpec(pa_sample_spec* spec)
{
  switch (audioSampleFormat) {
  case rfb::qemuAudioFormatU8:
    spec->format = PA_SAMPLE_U8;
    break;
  case rfb::qemuAudioFormatS16:
    spec->format = PA_SAMPLE_S16NE;
    break;
  case rfb::qemuAudioFormatS32:
    spec->format = PA_SAMPLE_S32NE;
    break;
  default:
    return false;
  }

  spec->rate = audioFrequency;
  spec->channels = audioChannels;

  return true;
}

AudioOutputPulse::AudioOutputPulse()
  : available(false), opened(false), timedOut(false),
    mainloop(nullptr), context(nullptr), stream(nullptr),
    buffer(nullptr), bufferSize(0), bufferFree(0), pendingSize(0),
    writtenHead(0), pendingHead(0),
    streamId(0), extraDelayMs(0),
    starved(false), starvedAt(0), starvedStreamId(0),
    pendingError(nullptr), pendingErrorCode(0)
{
  pa_sample_spec spec;

  if (!fillSampleSpec(&spec))
    return;

  available = connect();
}

AudioOutputPulse::~AudioOutputPulse()
{
  // Nothing may be running on the mainloop's thread while we take apart
  // the stream, and the buffer it has been reading from
  if (mainloop != nullptr)
    pa_threaded_mainloop_stop(mainloop);

  if (stream != nullptr) {
    pa_stream_disconnect(stream);
    pa_stream_unref(stream);
  }

  if (context != nullptr) {
    pa_context_disconnect(context);
    pa_context_unref(context);
  }

  if (mainloop != nullptr)
    pa_threaded_mainloop_free(mainloop);

  free(buffer);
}

// Connecting is the only thing here that waits, because create() has no
// way to say "ask me again later"
bool AudioOutputPulse::connect()
{
  pa_mainloop_api* api;
  pa_proplist* proplist;
  pa_time_event* timeout;
  bool ready;

  mainloop = pa_threaded_mainloop_new();
  if (mainloop == nullptr)
    return false;

  if (pa_threaded_mainloop_start(mainloop) < 0) {
    pa_threaded_mainloop_free(mainloop);
    mainloop = nullptr;
    return false;
  }

  pa_threaded_mainloop_lock(mainloop);

  api = pa_threaded_mainloop_get_api(mainloop);

  // The application id is how PulseAudio finds our .desktop file, and
  // the icon name is what it falls back on when it cannot
  proplist = pa_proplist_new();
  pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "TigerVNC");
  pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "vncviewer");
  pa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "tigervnc");

  context = pa_context_new_with_proplist(api, "TigerVNC", proplist);

  pa_proplist_free(proplist);

  if (context == nullptr) {
    pa_threaded_mainloop_unlock(mainloop);
    return false;
  }

  pa_context_set_state_callback(context, contextStateCallback, this);

  // Starting a sound server that isn't running is not our place
  if (pa_context_connect(context, nullptr,
                         PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0) {
    vlog.error(_("Could not connect to sound server: %s"),
               pa_strerror(pa_context_errno(context)));
    pa_threaded_mainloop_unlock(mainloop);
    return false;
  }

  // pa_threaded_mainloop_wait() has no timeout of its own, so something
  // on the loop has to wake us if the server never answers
  timeout = pa_context_rttime_new(context,
                                  pa_rtclock_now() +
                                    connectTimeoutMs * PA_USEC_PER_MSEC,
                                  timeoutCallback, this);

  while (true) {
    pa_context_state_t state;

    state = pa_context_get_state(context);
    if (state == PA_CONTEXT_READY) {
      ready = true;
      break;
    }
    if (!PA_CONTEXT_IS_GOOD(state)) {
      vlog.error(_("Could not connect to sound server: %s"),
                 pa_strerror(pa_context_errno(context)));
      ready = false;
      break;
    }
    if (timedOut) {
      vlog.error(_("Timed out connecting to sound server"));
      ready = false;
      break;
    }

    pa_threaded_mainloop_wait(mainloop);
  }

  if (timeout != nullptr)
    api->time_free(timeout);

  pa_threaded_mainloop_unlock(mainloop);

  return ready;
}

size_t AudioOutputPulse::getSampleSize() const
{
  return audioChannels << (audioSampleFormat >> 1);
}

bool AudioOutputPulse::open()
{
  pa_sample_spec spec;
  pa_buffer_attr attr;
  size_t samples, sampleSize;

  if (opened)
    return true;
  if (!available)
    return false;

  fillSampleSpec(&spec);

  // Round the sample count up to a power of two so that the wrapping
  // arithmetic below can be a mask. The sample size is a power of two
  // as well, so the byte size ends up being one too.
  samples = 1;
  while (samples < (4 * audioMaxJitterMs * audioFrequency) / 1000)
    samples <<= 1;

  sampleSize = getSampleSize();

  buffer = (uint8_t*)calloc(samples, sampleSize);
  if (buffer == nullptr) {
    available = false;
    return false;
  }

  bufferSize = bufferFree = samples * sampleSize;
  pendingSize = writtenHead = pendingHead = 0;

  pa_threaded_mainloop_lock(mainloop);

  stream = pa_stream_new(context, "Remote audio", &spec, nullptr);
  if (stream == nullptr) {
    vlog.error(_("Could not create audio playback stream: %s"),
               pa_strerror(pa_context_errno(context)));
    pa_threaded_mainloop_unlock(mainloop);
    free(buffer);
    buffer = nullptr;
    available = false;
    return false;
  }

  pa_stream_set_state_callback(stream, streamStateCallback, this);
  pa_stream_set_write_callback(stream, streamWriteCallback, this);
  pa_stream_set_underflow_callback(stream, streamUnderflowCallback, this);

  // How much the server should keep buffered, and how little it needs
  // before it starts playing. The first is what bounds how far ahead we
  // are allowed to write, so that a backlog stays in our buffer where it
  // is measured and dropped rather than growing inside the library. The
  // second is what lets a stream start promptly, and recover from an
  // underrun without a further gap of its own.
  memset(&attr, 0, sizeof(attr));
  attr.maxlength = (uint32_t)-1;
  attr.tlength = (uint32_t)(audioMaxJitterMs * audioFrequency /
                            1000 * sampleSize);
  attr.prebuf = (uint32_t)(audioMinStreamDelayMs * audioFrequency /
                           1000 * sampleSize);
  attr.minreq = (uint32_t)-1;
  attr.fragsize = (uint32_t)-1;

  if (pa_stream_connect_playback(stream, nullptr, &attr,
                                 PA_STREAM_NOFLAGS, nullptr, nullptr) < 0) {
    vlog.error(_("Could not open audio playback device: %s"),
               pa_strerror(pa_context_errno(context)));
    pa_stream_unref(stream);
    stream = nullptr;
    pa_threaded_mainloop_unlock(mainloop);
    free(buffer);
    buffer = nullptr;
    available = false;
    return false;
  }

  // Deliberately not waiting for the stream to be ready. Samples given
  // to us in the meantime go in the buffer, and the state callback
  // writes them out once there is somewhere to write them to.
  opened = true;

  pa_threaded_mainloop_unlock(mainloop);

  return true;
}

void AudioOutputPulse::addSilence(size_t samples)
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

void AudioOutputPulse::addSamples(const uint8_t* samples, size_t length)
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

// Hands as much of the buffer to the server as it currently wants. Must
// be called with the mainloop lock held, which is also what makes it
// safe to call from the mainloop's own callbacks.
void AudioOutputPulse::submit()
{
  if (!opened)
    return;
  if (pa_stream_get_state(stream) != PA_STREAM_READY)
    return;
  if (pendingSize == 0)
    return;

  // Having something to hand over after the server ran dry is what tells
  // us how long it stayed dry, and that is how much further ahead we
  // need to buffer
  if (starved) {
    starved = false;
    if (starvedStreamId == streamId) {
      unsigned long long now = pa_rtclock_now();
      if (now > starvedAt) {
        unsigned long long ms;
        ms = (now - starvedAt + PA_USEC_PER_MSEC - 1) / PA_USEC_PER_MSEC;
        if (ms > audioMaxJitterMs)
          ms = audioMaxJitterMs;
        if (extraDelayMs < ms)
          extraDelayMs = (unsigned)ms;
      }
    }
  }

  while (pendingSize > 0) {
    size_t writable, length;

    // Writing more than the server has asked for would only move the
    // backlog into the library, where nothing bounds it
    writable = pa_stream_writable_size(stream);
    if (writable == (size_t)-1)
      break;

    length = pendingSize;
    if (length > bufferSize - writtenHead)
      length = bufferSize - writtenHead;
    if (length > writable)
      length = writable;

    // Never split a sample across two writes
    length -= length % getSampleSize();
    if (length == 0)
      break;

    if (pa_stream_write(stream, buffer + writtenHead, length,
                        nullptr, 0, PA_SEEK_RELATIVE) < 0) {
      setError(N_("Could not write to audio playback device"));
      break;
    }

    // The samples have been copied out, so the space is ours again
    // straight away
    writtenHead = (writtenHead + length) & (bufferSize - 1);
    pendingSize -= length;
    bufferFree += length;
  }
}

// Records a failure noticed on the mainloop's thread. Nothing outside
// PulseAudio may be touched from there, logging included, so the main
// thread reports it later from reportError().
void AudioOutputPulse::setError(const char* message)
{
  if (pendingError != nullptr)
    return;

  pendingError = message;
  pendingErrorCode = pa_context_errno(context);
}

// Must be called from the main thread, without the mainloop lock held
void AudioOutputPulse::reportError()
{
  const char* message;
  int code;

  if (mainloop == nullptr)
    return;

  pa_threaded_mainloop_lock(mainloop);
  message = pendingError;
  code = pendingErrorCode;
  pendingError = nullptr;
  pa_threaded_mainloop_unlock(mainloop);

  if (message == nullptr)
    return;

  vlog.error("%s: %s", _(message), pa_strerror(code));
}

// Called on the mainloop's own thread, with its lock held, so the
// buffer and everything guarded by that lock may be touched directly
void AudioOutputPulse::contextStateCallback(pa_context* context,
                                            void* userdata)
{
  AudioOutputPulse* self = (AudioOutputPulse*)userdata;

  // Only FAILED is worth saying anything about: before we are available
  // this is just the connect above making progress, whose failures
  // connect() reports itself, and TERMINATED is our own disconnect,
  // which pa_context_disconnect() reports back to us from right here
  if (self->available &&
      (pa_context_get_state(context) == PA_CONTEXT_FAILED))
    self->setError(N_("Lost connection to sound server"));

  pa_threaded_mainloop_signal(self->mainloop, 0);
}

void AudioOutputPulse::streamStateCallback(pa_stream* stream, void* userdata)
{
  AudioOutputPulse* self = (AudioOutputPulse*)userdata;

  switch (pa_stream_get_state(stream)) {
  case PA_STREAM_READY:
    // Anything buffered while the stream was still connecting can go
    // out now
    self->submit();
    break;
  case PA_STREAM_FAILED:
    self->setError(N_("Audio playback stream failed"));
    break;
  default:
    break;
  }
}

void AudioOutputPulse::streamWriteCallback(pa_stream* /*stream*/,
                                           size_t /*length*/, void* userdata)
{
  AudioOutputPulse* self = (AudioOutputPulse*)userdata;

  self->submit();
}

void AudioOutputPulse::streamUnderflowCallback(pa_stream* /*stream*/,
                                               void* userdata)
{
  AudioOutputPulse* self = (AudioOutputPulse*)userdata;

  // Nothing left to play means the server ran dry waiting for us. How
  // long it stays dry is worked out in submit(), when we finally have
  // something to give it.
  if (!self->starved) {
    self->starved = true;
    self->starvedAt = pa_rtclock_now();
    self->starvedStreamId = self->streamId;
  }
}

void AudioOutputPulse::timeoutCallback(pa_mainloop_api* /*api*/,
                                       pa_time_event* /*event*/,
                                       const struct timeval* /*tv*/,
                                       void* userdata)
{
  AudioOutputPulse* self = (AudioOutputPulse*)userdata;

  self->timedOut = true;
  pa_threaded_mainloop_signal(self->mainloop, 0);
}

void AudioOutputPulse::start()
{
  if (!open())
    return;

  pa_threaded_mainloop_lock(mainloop);

  streamId++;

  // Play a little silence first, so that a sample arriving later than
  // the one before it does not leave the device with nothing to play
  addSilence((audioMinStreamDelayMs + extraDelayMs) *
             audioFrequency / 1000);
  submit();

  pa_threaded_mainloop_unlock(mainloop);

  reportError();
}

void AudioOutputPulse::stop()
{
  // Whatever is already buffered should still be played, so there is
  // nothing to do but let it drain
  reportError();
}

void AudioOutputPulse::play(const uint8_t* samples, size_t length)
{
  if (!opened)
    return;

  pa_threaded_mainloop_lock(mainloop);

  addSamples(samples, length);
  submit();

  pa_threaded_mainloop_unlock(mainloop);

  reportError();
}
