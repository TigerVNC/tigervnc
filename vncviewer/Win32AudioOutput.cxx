/* Copyright 2022 Mikhail Kupchik
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

#include "Win32AudioOutput.h"

//
// Lifecycle of audio samples is as follows:
//
// 1) Caller of this class must provide a pointer and length of continuous chunk of source
//    audio samples, e.g. in the socket input buffer. Win32AudioOutput::addSamples() may
//    be invoked multiple times per one audio data message (for partial chunks of audio data).
//
// 2) Until the end of an audio data message arrives, samples are copied from the socket input
//    buffer to the circular audio playback buffer. We prefer not to leave audio samples in the
//    socket input buffer, and move them to circular audio playback buffer as soon as they arrive.
//    If a bad-behaved VNC server sends unreasonably large audio data message, then we clamp it
//    here: samples which do not fit into the circular audio playback buffer are de facto discarded,
//    but reported as consumed to the caller, so the caller can release this space in the socket
//    input buffer, and continue to do so until the end of audio data message.
//
// 3) When the end of an audio data message arrives, Win32AudioOutput::submitSamples() should be
//    called. It submits already acculumated samples for asyncronous playback to the NT kernel
//    device driver, the audio mixer. Asyncronous I/O request in flight refer to the audio samples
//    in circular buffer owned by this class. So while audio playback I/O request is still in flight,
//    socket input buffer may contain different data, may be reallocated, may be empty etc.
//
// 4) When kernel completes asyncronous I/O request, it awakes internal worker thread started
//    by winmm.dll / wdmaud.drv in the address space of this process. This thread invokes
//    Win32AudioOutput::waveOutCallback(). There we link asyncronous I/O request header into the
//    linked list to dispose it later, during the next call to Win32AudioOutput::submitSamples()
//    or in the destructor of this class.
//
// 5) There may be multiple audio I/O requests in flight, they are played in the order of submission.
//

Win32AudioOutput::Win32AudioOutput()
  : haveWO(false), openedWO(false), sampleFormat(sampleFormatU8), numberOfChannels(0),
    samplingFreq(0), currentStreamId(0), handleWO(NULL), bufPtr(NULL), bufTotalSize(0),
    bufFreeSize(0), bufUnsubmittedSize(0), bufSubmittedHead(0), bufUnsubmittedHead(0),
    doneHdrsSlist(NULL), hdrsInFlight(0), extraDelayInMillisec(0)
{
  static const rdr::U32 freqTable[4] = {48000, 44100, 22050, 11025};

  // Default format (16-bit stereo, 48000 Hz) is tried first and usually succeeds,
  // but we also try other formats before giving up
  for (rdr::U8 bits_per_sample = 16; bits_per_sample != 0; bits_per_sample -= 8) {
    for (rdr::U8 n_channels = 2; n_channels != 0; n_channels--) {
      for (rdr::U8 freq_index = 0; freq_index < 4; freq_index++) {

        WAVEFORMATEX wfx;
        memset(&wfx, 0, sizeof(wfx));
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.nChannels       = n_channels;
        wfx.nSamplesPerSec  = freqTable[freq_index];
        wfx.nBlockAlign     = n_channels * (bits_per_sample / 8);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.wBitsPerSample  = bits_per_sample;
        wfx.cbSize          = 0;

        MMRESULT mmr = waveOutOpen(NULL, WAVE_MAPPER, &wfx, 0, 0,
                                   CALLBACK_NULL | WAVE_FORMAT_QUERY);
        if (mmr == MMSYSERR_NOERROR) {
          sampleFormat     = ((bits_per_sample == 8) ? sampleFormatU8 : sampleFormatS16);
          numberOfChannels = n_channels;
          samplingFreq     = freqTable[freq_index];
          haveWO           = true;
          return;
        }
      }
    }
  }
}

bool Win32AudioOutput::openAndAllocateBuffer()
{
  if (!haveWO)
    return false;

  if (!openedWO) {
    // open wave output
    WAVEFORMATEX wfx;
    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = numberOfChannels;
    wfx.nSamplesPerSec  = samplingFreq;
    wfx.nBlockAlign     = WORD(getSampleSize());
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.wBitsPerSample  = 8 << (sampleFormat >> 1);
    wfx.cbSize          = 0;

    MMRESULT mmr = waveOutOpen(&handleWO, WAVE_MAPPER, &wfx,
                               DWORD_PTR(&Win32AudioOutput::waveOutCallback),
                               DWORD_PTR(this), CALLBACK_FUNCTION);
    if (mmr == MMSYSERR_NOERROR)
      openedWO = true;
    else
      return false;

    // allocate buffer
    size_t buf_estim_size = (4 * maxNetworkJitterInMillisec * samplingFreq) / 1000;

    size_t buf_alloc_size = 1;
    while (buf_alloc_size < buf_estim_size)
      buf_alloc_size <<= 1;

    size_t sample_size = getSampleSize();

    bufPtr = ((rdr::U8*)( calloc(buf_alloc_size, sample_size) ));
    if (bufPtr == NULL) {
      waveOutClose(handleWO);
      handleWO = NULL;
      openedWO = false;
      return false;
    }

    bufTotalSize = bufFreeSize = buf_alloc_size * sample_size;
    bufUnsubmittedSize = bufSubmittedHead = bufUnsubmittedHead = 0;

    // try to change scheduling of this process (use minimum time slice)
    timeBeginPeriod(1);
  }

  return true;
}

void Win32AudioOutput::addSilentSamples(size_t numberOfSamples)
{
  if (openedWO) {
    size_t bytes_left_to_add = numberOfSamples * getSampleSize();
    while (bytes_left_to_add != 0) {
      size_t bytes_to_add = bytes_left_to_add;
      if (bytes_to_add > bufFreeSize)
        bytes_to_add = bufFreeSize;
      if (bytes_to_add + bufUnsubmittedHead > bufTotalSize)
        bytes_to_add = bufTotalSize - bufUnsubmittedHead;
      if (bytes_to_add == 0)
        break;

      memset(bufPtr + bufUnsubmittedHead, ((sampleFormat == sampleFormatU8) ? 0x80 : 0), bytes_to_add);
      bufUnsubmittedHead  = ((bufUnsubmittedHead + bytes_to_add) & (bufTotalSize - 1));
      bufFreeSize        -= bytes_to_add;
      bufUnsubmittedSize += bytes_to_add;
      bytes_left_to_add  -= bytes_to_add;
    }
  }
}

size_t Win32AudioOutput::addSamples(const rdr::U8* data, size_t size)
{
  if (haveWO) {
    size_t sample_size = getSampleSize();
    size -= (size & (sample_size - 1));
  }

  if (openedWO) {
    size_t bytes_left_to_copy = size;
    while (bytes_left_to_copy != 0) {
      size_t bytes_to_copy = bytes_left_to_copy;
      if (bytes_to_copy > bufFreeSize)
        bytes_to_copy = bufFreeSize;
      if (bytes_to_copy + bufUnsubmittedHead > bufTotalSize)
        bytes_to_copy = bufTotalSize - bufUnsubmittedHead;
      if (bytes_to_copy == 0)
        break;

      memcpy(bufPtr + bufUnsubmittedHead, data, bytes_to_copy);
      bufUnsubmittedHead  = ((bufUnsubmittedHead + bytes_to_copy) & (bufTotalSize - 1));
      bufFreeSize        -= bytes_to_copy;
      bufUnsubmittedSize += bytes_to_copy;
      data               += bytes_to_copy;
      bytes_left_to_copy -= bytes_to_copy;
    }
  }

  return size;
}

ULONGLONG Win32AudioOutput::getCurrentTimestamp()
{
  FILETIME ft_now;
  GetSystemTimeAsFileTime(&ft_now);

  ULARGE_INTEGER ul_now;
  ul_now.LowPart  = ft_now.dwLowDateTime;
  ul_now.HighPart = ft_now.dwHighDateTime;

  return ul_now.QuadPart;
}

void CALLBACK Win32AudioOutput::waveOutCallback(HWAVEOUT hwo, UINT msg, DWORD_PTR instance,
                                                DWORD_PTR param1, DWORD_PTR param2)
{
  if (msg == WOM_DONE) {
    Win32AudioOutput* p_this = ((Win32AudioOutput*)instance);
    HdrInSlist*       hdr    = ((HdrInSlist*)param1);

    if (p_this->openedWO && (p_this->handleWO == hwo) && (0 != (hdr->whdr.dwFlags & WHDR_DONE))) {
      if (0 == InterlockedDecrement(&(p_this->hdrsInFlight))) {
        hdr->starvedWhenDone     = TRUE;
        hdr->starvationTimestamp = getCurrentTimestamp();
      }

      PVOID next_hdr_ptr = p_this->doneHdrsSlist;
      while (true) {
        InterlockedExchangePointer(&(hdr->volatileNext), next_hdr_ptr);
        PVOID xchg_initial_value = InterlockedCompareExchangePointer(
          &(p_this->doneHdrsSlist), hdr, next_hdr_ptr
        );
        if (xchg_initial_value == next_hdr_ptr)
          break;
        next_hdr_ptr = xchg_initial_value;
      }
    }
  }
}

bool Win32AudioOutput::submitSamples()
{
  if (!openedWO)
    return false;

  HdrInSlist* spare_hdrs = ((HdrInSlist*)(InterlockedExchangePointer(&doneHdrsSlist, NULL)));
  for (HdrInSlist* hdr = spare_hdrs; hdr != NULL; hdr = hdr->next) {
    bufFreeSize += hdr->whdr.dwBufferLength;
    waveOutUnprepareHeader(handleWO, &(hdr->whdr), sizeof(WAVEHDR));
    if (hdr->starvedWhenDone && (hdr->streamId == currentStreamId)) {
      ULONGLONG now = getCurrentTimestamp();
      if (now > hdr->starvationTimestamp) {
        ULONGLONG delay_in_100nsec  = now - hdr->starvationTimestamp;     // delay in 100ns intervals
        ULONGLONG delay_in_millisec = (delay_in_100nsec + 9999) / 10000;  // convert to delay in milliseconds
        if (delay_in_millisec > maxNetworkJitterInMillisec)               // and clamp at a reasonable limit
          delay_in_millisec = maxNetworkJitterInMillisec;
        if (extraDelayInMillisec < ((rdr::U32)delay_in_millisec))
          extraDelayInMillisec = ((rdr::U32)delay_in_millisec);
      }
    }
  }

  while (bufUnsubmittedSize != 0) {
    size_t io_bytes = bufUnsubmittedSize;
    if (io_bytes + bufSubmittedHead > bufTotalSize)
      io_bytes = bufTotalSize - bufSubmittedHead;

    HdrInSlist* hdr = NULL;
    if (spare_hdrs != NULL) {
      hdr = spare_hdrs;
      spare_hdrs = hdr->next;
    } else {
      hdr = ((HdrInSlist*)(malloc(sizeof(HdrInSlist))));
      if (!hdr)
        break;
    }
    memset(hdr, 0, sizeof(HdrInSlist));
    hdr->whdr.lpData         = LPSTR(bufPtr + bufSubmittedHead);
    hdr->whdr.dwBufferLength = io_bytes; 
    hdr->streamId            = currentStreamId;

    MMRESULT mmr = waveOutPrepareHeader(handleWO, &(hdr->whdr), sizeof(WAVEHDR));
    if (mmr != MMSYSERR_NOERROR) {
      hdr->next  = spare_hdrs;
      spare_hdrs = hdr;
      break;
    }

    InterlockedIncrement(&hdrsInFlight);
    mmr = waveOutWrite(handleWO, &(hdr->whdr), sizeof(WAVEHDR));
    if (mmr != MMSYSERR_NOERROR) {
      InterlockedDecrement(&hdrsInFlight);
      waveOutUnprepareHeader(handleWO, &(hdr->whdr), sizeof(WAVEHDR));
      hdr->next  = spare_hdrs;
      spare_hdrs = hdr;
      break;
    }

    bufSubmittedHead = ((bufSubmittedHead + io_bytes) & (bufTotalSize - 1));
    bufUnsubmittedSize -= io_bytes;
  }

  while (spare_hdrs != NULL) {
    HdrInSlist* next_hdr = spare_hdrs->next;
    free(spare_hdrs);
    spare_hdrs = next_hdr;
  }

  return (bufUnsubmittedSize == 0);
}

void Win32AudioOutput::notifyStreamingStartStop(bool isStart)
{
  if (isStart) {
    ++currentStreamId;

    // suppress audio stuttering caused by network jitter:
    // add 20+ milliseconds of silence (playback delay) ahead of actual samples
    size_t delay_in_millisec = 20 + extraDelayInMillisec;
    addSilentSamples(delay_in_millisec * samplingFreq / 1000);
    submitSamples();
  }
}

Win32AudioOutput::~Win32AudioOutput()
{
  if (openedWO) {
    waveOutReset(handleWO);

    timeEndPeriod(1);

    HdrInSlist* spare_hdrs = ((HdrInSlist*)(InterlockedExchangePointer(&doneHdrsSlist, NULL)));
    while (spare_hdrs != NULL) {
      waveOutUnprepareHeader(handleWO, &(spare_hdrs->whdr), sizeof(WAVEHDR));
      HdrInSlist* next_hdr = spare_hdrs->next;
      free(spare_hdrs);
      spare_hdrs = next_hdr;
    }

    waveOutClose(handleWO);
    handleWO = NULL;
    openedWO = false;

    free(bufPtr);
  }
}
