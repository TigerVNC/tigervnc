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

#include <mfapi.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#define SAFE_RELEASE(obj) if (obj) { obj->Release(); obj = NULL; }

#include <os/Mutex.h>
#include <rfb/LogWriter.h>
#include <rfb/PixelBuffer.h>
#include <rfb/H264WinDecoderContext.h>

using namespace rfb;

static LogWriter vlog("H264WinDecoderContext");

bool H264WinDecoderContext::initCodec() {
  os::AutoMutex lock(&mutex);

  if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE)))
  {
    vlog.error("Could not initialize MediaFoundation");
    return false;
  }

  if (FAILED(CoCreateInstance(CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&decoder)))
  {
    vlog.error("MediaFoundation H264 codec not found");
    return false;
  }

  GUID CLSID_VideoProcessorMFT = { 0x88753b26, 0x5b24, 0x49bd, { 0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82 } };
  if (FAILED(CoCreateInstance(CLSID_VideoProcessorMFT, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&converter)))
  {
    vlog.error("Cannot create MediaFoundation Video Processor (available only on Windows 8+). Trying ColorConvert DMO.");
    if (FAILED(CoCreateInstance(CLSID_CColorConvertDMO, NULL, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&converter)))
    {
      decoder->Release();
      vlog.error("ColorConvert DMO not found");
      return false;
    }
  }

  // if possible, enable low-latency decoding (Windows 8 and up)
  IMFAttributes* attributes;
  if (SUCCEEDED(decoder->GetAttributes(&attributes)))
  {
    GUID MF_LOW_LATENCY = { 0x9c27891a, 0xed7a, 0x40e1, { 0x88, 0xe8, 0xb2, 0x27, 0x27, 0xa0, 0x24, 0xee } };
    if (SUCCEEDED(attributes->SetUINT32(MF_LOW_LATENCY, TRUE)))
    {
      vlog.info("Enabled low latency mode");
    }
    attributes->Release();
  }

  // set decoder input type
  IMFMediaType* input_type;
  if (FAILED(MFCreateMediaType(&input_type)))
  {
    decoder->Release();
    converter->Release();
    vlog.error("Could not create MF MediaType");
    return false;
  }
  input_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  input_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
  decoder->SetInputType(0, input_type, 0);
  input_type->Release();

  // set decoder output type (NV12)
  DWORD output_index = 0;
  IMFMediaType* output_type = NULL;
  while (SUCCEEDED(decoder->GetOutputAvailableType(0, output_index++, &output_type)))
  {
    GUID subtype;
    if (SUCCEEDED(output_type->GetGUID(MF_MT_SUBTYPE, &subtype)) && subtype == MFVideoFormat_NV12)
    {
      decoder->SetOutputType(0, output_type, 0);
      output_type->Release();
      break;
    }
    output_type->Release();
  }

  if (FAILED(decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0)))
  {
    decoder->Release();
    converter->Release();
    input_type->Release();
    vlog.error("Could not start H264 decoder");
    return false;
  }

  MFT_OUTPUT_STREAM_INFO info;
  decoder->GetOutputStreamInfo(0, &info);

  if (FAILED(MFCreateSample(&input_sample)) ||
      FAILED(MFCreateSample(&decoded_sample)) ||
      FAILED(MFCreateSample(&converted_sample)) ||
      FAILED(MFCreateMemoryBuffer(4 * 1024 * 1024, &input_buffer)) ||
      FAILED(MFCreateMemoryBuffer(info.cbSize, &decoded_buffer)))
  {
    decoder->Release();
    converter->Release();
    input_type->Release();
    SAFE_RELEASE(input_sample);
    SAFE_RELEASE(decoded_sample);
    SAFE_RELEASE(converted_sample);
    SAFE_RELEASE(input_buffer);
    SAFE_RELEASE(decoded_buffer);
    vlog.error("Could not allocate media samples/buffers");
    return false;
  }

  input_sample->AddBuffer(input_buffer);
  decoded_sample->AddBuffer(decoded_buffer);

  initialized = true;
  return true;
}

void H264WinDecoderContext::freeCodec() {
  os::AutoMutex lock(&mutex);

  if (!initialized)
    return;
  SAFE_RELEASE(decoder)
  SAFE_RELEASE(converter)
  SAFE_RELEASE(input_sample)
  SAFE_RELEASE(decoded_sample)
  SAFE_RELEASE(converted_sample)
  SAFE_RELEASE(input_buffer)
  SAFE_RELEASE(decoded_buffer)
  SAFE_RELEASE(converted_buffer)
  MFShutdown();
  initialized = false;
}

void H264WinDecoderContext::decode(const rdr::U8* h264_buffer, rdr::U32 len, rdr::U32 flags, ModifiablePixelBuffer* pb) {  
  os::AutoMutex lock(&mutex);
  if (!initialized)
    return;

  if (FAILED(input_buffer->SetCurrentLength(len)))
  {
    input_buffer->Release();
    if (FAILED(MFCreateMemoryBuffer(len, &input_buffer)))
    {
      vlog.error("Could not allocate media buffer");
      return;
    }
    input_buffer->SetCurrentLength(len);
    input_sample->RemoveAllBuffers();
    input_sample->AddBuffer(input_buffer);
  }

  BYTE* locked;
  input_buffer->Lock(&locked, NULL, NULL);
  memcpy(locked, h264_buffer, len);
  input_buffer->Unlock();

  vlog.debug("Received %u bytes, decoding", len);

  if (FAILED(decoder->ProcessInput(0, input_sample, 0)))
  {
    vlog.error("Error sending a packet to decoding");
    return;
  }

  bool decoded = false;

  // try to retrieve all decoded output, as input can submit multiple h264 packets in one buffer
  for (;;)
  {
    DWORD curlen;
    decoded_buffer->GetCurrentLength(&curlen);
    decoded_buffer->SetCurrentLength(0);

    MFT_OUTPUT_DATA_BUFFER decoded_data;
    decoded_data.dwStreamID = 0;
    decoded_data.pSample = decoded_sample;
    decoded_data.dwStatus = 0;
    decoded_data.pEvents = NULL;

    DWORD status;
    HRESULT hr = decoder->ProcessOutput(0, 1, &decoded_data, &status);
    SAFE_RELEASE(decoded_data.pEvents)

    if (SUCCEEDED(hr))
    {
      vlog.debug("Frame decoded");
      // successfully decoded next frame
      // but do not exit loop, try again if there is next frame
      decoded = true;
    }
    else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
    {
      // no more frames to decode
      if (decoded)
      {
        // restore previous buffer length for converter
        decoded_buffer->SetCurrentLength(curlen);
      }
      break;
    }
    else if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
      // something changed (resolution, framerate, h264 properties...)
      // need to setup output type and try decoding again

      DWORD output_index = 0;
      IMFMediaType* output_type = NULL;
      while (SUCCEEDED(decoder->GetOutputAvailableType(0, output_index++, &output_type)))
      {
        GUID subtype;
        if (SUCCEEDED(output_type->GetGUID(MF_MT_SUBTYPE, &subtype)) && subtype == MFVideoFormat_NV12)
        {
          decoder->SetOutputType(0, output_type, 0);
          break;
        }
        output_type->Release();
        output_type = NULL;
      }

      // reinitialize output type (NV12) that now has correct properties (width/height/framerate)
      decoder->SetOutputType(0, output_type, 0);

      UINT32 width = 0;
      UINT32 height = 0;
      MFGetAttributeSize(output_type, MF_MT_FRAME_SIZE, &width, &height);
      vlog.debug("Setting up decoded output with %ux%u size", width, height);

      // input type to converter, BGRX pixel format
      IMFMediaType* converted_type;
      if (FAILED(MFCreateMediaType(&converted_type)))
      {
        vlog.error("Error creating media type");
      }
      else
      {
        converted_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        converted_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        converted_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        MFSetAttributeSize(converted_type, MF_MT_FRAME_SIZE, width, height);
        MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, width, &stride);
        // bottom-up
        stride = -stride;
        converted_type->SetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32)stride);

        // setup NV12 -> BGRX converter
        converter->SetOutputType(0, converted_type, 0);
        converter->SetInputType(0, output_type, 0);
        converted_type->Release();

        // create converter output buffer

        MFT_OUTPUT_STREAM_INFO info;
        converter->GetOutputStreamInfo(0, &info);

        if (FAILED(MFCreateMemoryBuffer(info.cbSize, &converted_buffer)))
        {
          vlog.error("Error creating media buffer");
        }
        else
        {
          converted_sample->AddBuffer(converted_buffer);
        }
      }
      output_type->Release();
    }
  }

  // we care only about final image
  // we ignore previous images if decoded multiple in a row
  if (decoded)
  {
    if (FAILED(converter->ProcessInput(0, decoded_sample, 0)))
    {
      vlog.error("Error sending a packet to converter");
      return;
    }

    MFT_OUTPUT_DATA_BUFFER converted_data;
    converted_data.dwStreamID = 0;
    converted_data.pSample = converted_sample;
    converted_data.dwStatus = 0;
    converted_data.pEvents = NULL;

    DWORD status;
    HRESULT hr = converter->ProcessOutput(0, 1, &converted_data, &status);
    SAFE_RELEASE(converted_data.pEvents)

    if (FAILED(hr))
    {
      vlog.error("Error converting to RGB");
    }
    else
    {
      vlog.debug("Frame converted to RGB");

      BYTE* out;
      converted_buffer->Lock(&out, NULL, NULL);
      pb->imageRect(rect, out, (int)stride / 4);
      converted_buffer->Unlock();
    }
  }
}
