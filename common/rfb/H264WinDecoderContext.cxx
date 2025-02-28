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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdexcept>

#include <mfapi.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#define SAFE_RELEASE(obj) if (obj) { obj->Release(); obj = nullptr; }

#include <rfb/PixelBuffer.h>
#include <rfb/H264WinDecoderContext.h>

using namespace rfb;

// Older MinGW lacks this definition
#ifndef HAVE_VIDEO_PROCESSOR_MFT
static GUID CLSID_VideoProcessorMFT = { 0x88753b26, 0x5b24, 0x49bd, { 0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82 } };
#endif

H264WinDecoderContext::H264WinDecoderContext(const core::Rect &r)
  : H264DecoderContext(r)
{
  if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE)))
    throw std::runtime_error("Could not initialize MediaFoundation");

  if (FAILED(CoCreateInstance(CLSID_CMSH264DecoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&decoder)))
    throw std::runtime_error("MediaFoundation H264 codec not found");

  if (FAILED(CoCreateInstance(CLSID_VideoProcessorMFT, nullptr, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&converter)))
  {
    if (FAILED(CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_IMFTransform, (LPVOID*)&converter)))
    {
      decoder->Release();
      throw std::runtime_error("MediaFoundation H264 codec not found");
    }
  }

  // if possible, enable low-latency decoding (Windows 8 and up)
  IMFAttributes* attributes;
  if (SUCCEEDED(decoder->GetAttributes(&attributes)))
  {
    GUID MF_LOW_LATENCY = { 0x9c27891a, 0xed7a, 0x40e1, { 0x88, 0xe8, 0xb2, 0x27, 0x27, 0xa0, 0x24, 0xee } };
    attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    attributes->Release();
  }

  // set decoder input type
  IMFMediaType* input_type;
  if (FAILED(MFCreateMediaType(&input_type)))
  {
    decoder->Release();
    converter->Release();
    throw std::runtime_error("Could not create MF MediaType");
  }
  input_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  input_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
  decoder->SetInputType(0, input_type, 0);
  input_type->Release();

  // set decoder output type (NV12)
  DWORD output_index = 0;
  IMFMediaType* output_type = nullptr;
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
    throw std::runtime_error("Could not start H264 decoder");
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
    throw std::runtime_error("Could not allocate media samples/buffers");
  }

  input_sample->AddBuffer(input_buffer);
  decoded_sample->AddBuffer(decoded_buffer);
}

H264WinDecoderContext::~H264WinDecoderContext() {
  SAFE_RELEASE(decoder)
  SAFE_RELEASE(converter)
  SAFE_RELEASE(input_sample)
  SAFE_RELEASE(decoded_sample)
  SAFE_RELEASE(converted_sample)
  SAFE_RELEASE(input_buffer)
  SAFE_RELEASE(decoded_buffer)
  SAFE_RELEASE(converted_buffer)
  MFShutdown();
}

void H264WinDecoderContext::decode(const uint8_t* h264_buffer,
                                   uint32_t len,
                                   ModifiablePixelBuffer* pb) {
  if (FAILED(input_buffer->SetCurrentLength(len)))
  {
    input_buffer->Release();
    if (FAILED(MFCreateMemoryBuffer(len, &input_buffer)))
      throw std::runtime_error("Could not allocate media buffer");
    input_buffer->SetCurrentLength(len);
    input_sample->RemoveAllBuffers();
    input_sample->AddBuffer(input_buffer);
  }

  BYTE* locked;
  input_buffer->Lock(&locked, nullptr, nullptr);
  memcpy(locked, h264_buffer, len);
  input_buffer->Unlock();

  // extract actual size, including possible cropping
  ParseSPS(h264_buffer, len);

  if (FAILED(decoder->ProcessInput(0, input_sample, 0)))
  {
    // Silently ignore errors, hoping its a temporary encoding glitch
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
    decoded_data.pEvents = nullptr;

    DWORD status;
    HRESULT hr = decoder->ProcessOutput(0, 1, &decoded_data, &status);
    SAFE_RELEASE(decoded_data.pEvents)

    if (SUCCEEDED(hr))
    {
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
      IMFMediaType* output_type = nullptr;
      while (SUCCEEDED(decoder->GetOutputAvailableType(0, output_index++, &output_type)))
      {
        GUID subtype;
        if (SUCCEEDED(output_type->GetGUID(MF_MT_SUBTYPE, &subtype)) && subtype == MFVideoFormat_NV12)
        {
          decoder->SetOutputType(0, output_type, 0);
          break;
        }
        output_type->Release();
        output_type = nullptr;
      }

      // reinitialize output type (NV12) that now has correct properties (width/height/framerate)
      decoder->SetOutputType(0, output_type, 0);

      UINT32 width, height;
      if FAILED(MFGetAttributeSize(output_type, MF_MT_FRAME_SIZE, &width, &height))
      {
          // Silently ignore errors, hoping its a temporary encoding glitch
          output_type->Release();
          break;
      }

      // if MFT reports different width or height than calculated cropped width/height
      if (crop_width != 0 && crop_height != 0 && (width != crop_width || height != crop_height))
      {
          // create NV12/RGB image with full size as we'll do manual cropping
          width = full_width;
          height = full_height;
      }
      else
      {
          // no manual cropping necessary
          offset_x = offset_y = 0;
          crop_width = width;
          crop_height = height;
      }

      // input type to converter, BGRX pixel format
      IMFMediaType* converted_type;
      if (FAILED(MFCreateMediaType(&converted_type)))
      {
        // Silently ignore errors, hoping its a temporary encoding glitch
      }
      else
      {
        converted_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        converted_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        converted_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        MFSetAttributeSize(converted_type, MF_MT_FRAME_SIZE, full_width, full_height);
        MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, full_width, &stride);
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
          // Silently ignore errors, hoping its a temporary encoding glitch
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
      // Silently ignore errors, hoping its a temporary encoding glitch
      return;
    }

    MFT_OUTPUT_DATA_BUFFER converted_data;
    converted_data.dwStreamID = 0;
    converted_data.pSample = converted_sample;
    converted_data.dwStatus = 0;
    converted_data.pEvents = nullptr;

    DWORD status;
    HRESULT hr = converter->ProcessOutput(0, 1, &converted_data, &status);
    SAFE_RELEASE(converted_data.pEvents)

    if (FAILED(hr))
    {
      // Silently ignore errors, hoping its a temporary encoding glitch
    }
    else
    {
      BYTE* out;
      DWORD buflen;
      converted_buffer->Lock(&out, nullptr, &buflen);
      pb->imageRect(rect, out + offset_y * stride + offset_x * 4, (int)stride / 4);
      converted_buffer->Unlock();
    }
  }
}

// "7.3.2.1.1 Sequence parameter set data syntax" on page 66 of https://www.itu.int/rec/T-REC-H.264-202108-I/en
void H264WinDecoderContext::ParseSPS(const uint8_t* buffer, int length)
{
#define EXPECT(cond) if (!(cond)) return;

#define GET_BIT(bit) do {            \
    if (available == 0)              \
    {                                \
        if (length == 0) return;     \
        byte_ = *buffer++;            \
        length--;                    \
        available = 8;               \
    }                                \
    bit = (byte_ >> --available) & 1; \
} while (0)

#define GET_BITS(n, var) do {      \
    var = 0;                       \
    for (int b = n-1; b >= 0; b--) \
    {                              \
        unsigned bit;              \
        GET_BIT(bit);              \
        var |= bit << b;           \
    }                              \
} while (0)

// "9.1 Parsing process for Exp-Golomb codes" on page 231

#define GET_UE(var) do {                   \
    int zeroes = -1;                       \
    for (unsigned bit = 0; !bit; zeroes++) \
        GET_BIT(bit);                      \
    GET_BITS(zeroes, var);                 \
    var += (1U << zeroes) - 1;             \
} while(0)

#define SKIP_UE() do { \
    unsigned var;      \
    GET_UE(var);       \
} while (0)

#define SKIP_BITS(bits) do { \
    unsigned var;            \
    GET_BITS(bits, var);     \
} while (0)

    // check for NAL header
    EXPECT((length >= 3 && buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) ||
           (length >= 4 && buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1));
    length -= 4 - buffer[2];
    buffer += 4 - buffer[2];

    // NAL unit type
    EXPECT(length > 1);
    uint8_t type = buffer[0];
    EXPECT((type & 0x80) == 0); // forbidden zero bit
    EXPECT((type & 0x1f) == 7); // SPS NAL unit type
    buffer++;
    length--;

    int available = 0;
    uint8_t byte_ = 0;

    unsigned profile_idc;
    unsigned seq_parameter_set_id;

    GET_BITS(8, profile_idc);
    SKIP_BITS(6); // constraint_set0..5_flag
    SKIP_BITS(2); // reserved_zero_2bits
    SKIP_BITS(8); // level_idc
    GET_UE(seq_parameter_set_id);

    unsigned chroma_format_idc = 1;
    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118 ||
        profile_idc == 128 || profile_idc == 138 ||
        profile_idc == 139 || profile_idc == 134 ||
        profile_idc == 135)
    {
        GET_UE(chroma_format_idc);
        if (chroma_format_idc == 3)
        {
            SKIP_BITS(1); // separate_colour_plane_flag
        }
        SKIP_UE(); // bit_depth_luma_minus8
        SKIP_UE(); // bit_depth_chroma_minus8;
        SKIP_BITS(1); // qpprime_y_zero_transform_bypass_flag
        unsigned seq_scaling_matrix_present_flag;
        GET_BITS(1, seq_scaling_matrix_present_flag);
        if (seq_scaling_matrix_present_flag)
        {
            for (int i = 0; i < (chroma_format_idc != 3 ? 8 : 12); i++)
            {
                int seq_scaling_list_present_flag;
                GET_BITS(1, seq_scaling_list_present_flag);
                for (int j = 0; j < (seq_scaling_list_present_flag ? 16 : 64); j++)
                {
                    SKIP_UE(); // delta_scale;
                }
            }
        }
    }

    unsigned log2_max_frame_num_minus4;
    GET_UE(log2_max_frame_num_minus4); // log2_max_frame_num_minus4
    unsigned pic_order_cnt_type;
    GET_UE(pic_order_cnt_type);
    if (pic_order_cnt_type == 0)
    {
        SKIP_UE(); // log2_max_pic_order_cnt_lsb_minus4
    }
    else if (pic_order_cnt_type == 1)
    {
        SKIP_BITS(1); // delta_pic_order_always_zero_flag
        SKIP_UE(); // offset_for_non_ref_pic
        SKIP_UE(); // offset_for_top_to_bottom_field
        unsigned num_ref_frames_in_pic_order_cnt_cycle;
        GET_UE(num_ref_frames_in_pic_order_cnt_cycle);
        for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            SKIP_UE(); // offset_for_ref_frame
        }
    }
    SKIP_UE(); // max_num_ref_frames
    SKIP_BITS(1); // gaps_in_frame_num_value_allowed_flag
    unsigned pic_width_in_mbs_minus1;
    GET_UE(pic_width_in_mbs_minus1);
    unsigned pic_height_in_map_units_minus1;
    GET_UE(pic_height_in_map_units_minus1);
    unsigned frame_mbs_only_flag;
    GET_BITS(1, frame_mbs_only_flag);
    if (!frame_mbs_only_flag)
    {
        SKIP_BITS(1); // mb_adaptive_frame_field_flag
    }
    SKIP_BITS(1); // direct_8x8_inference_flag
    unsigned frame_cropping_flag;
    GET_BITS(1, frame_cropping_flag);

    unsigned frame_crop_left_offset = 0;
    unsigned frame_crop_right_offset = 0;
    unsigned frame_crop_top_offset = 0;
    unsigned frame_crop_bottom_offset = 0;
    if (frame_cropping_flag)
    {
        GET_UE(frame_crop_left_offset);
        GET_UE(frame_crop_right_offset);
        GET_UE(frame_crop_top_offset);
        GET_UE(frame_crop_bottom_offset);
    }
    // ignore rest of bits

    full_width = 16 * (pic_width_in_mbs_minus1 + 1);
    full_height = 16 * (pic_height_in_map_units_minus1 + 1) * (2 - frame_mbs_only_flag);

    // "6.2 Source, decoded, and output picture formats" on page 44
    unsigned sub_width_c = (chroma_format_idc  == 1 || chroma_format_idc == 2) ? 2 : 1;
    unsigned sub_height_c = (chroma_format_idc == 1) ? 2 : 1;

    // page 101
    unsigned crop_unit_x = chroma_format_idc == 0 ? 1 : sub_width_c;
    unsigned crop_unit_y = chroma_format_idc == 0 ? 2 - frame_mbs_only_flag : sub_height_c * (2 - frame_mbs_only_flag);
    crop_width = full_width - crop_unit_x * (frame_crop_right_offset + frame_crop_left_offset);
    crop_height = full_height - crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);

    offset_x = frame_crop_left_offset;
    offset_y = frame_crop_bottom_offset;

#undef SKIP_BITS
#undef SKIP_UE
#undef GET_BITS
#undef GET_BIT
#undef GET_UE
#undef EXPECT
}
