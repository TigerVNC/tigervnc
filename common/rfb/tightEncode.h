/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
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

//
// tightEncode.h - Tight encoding function.
//
// This file is #included after having set the following macros:
// BPP                - 8, 16 or 32
// EXTRA_ARGS         - optional extra arguments
// GET_IMAGE_INTO_BUF - gets a rectangle of pixel data into a buffer
//

#include <rdr/OutStream.h>
#include <rdr/ZlibOutStream.h>
#include <assert.h>

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#define PIXEL_T rdr::CONCAT2E(U,BPP)
#define WRITE_PIXEL CONCAT2E(writeOpaque,BPP)
#define TIGHT_ENCODE CONCAT2E(tightEncode,BPP)
#define SWAP_PIXEL CONCAT2E(SWAP,BPP)
#define HASH_FUNCTION CONCAT2E(HASH_FUNC,BPP)
#define PACK_PIXELS CONCAT2E(packPixels,BPP)
#define DETECT_SMOOTH_IMAGE CONCAT2E(detectSmoothImage,BPP)
#define ENCODE_SOLID_RECT CONCAT2E(encodeSolidRect,BPP)
#define ENCODE_FULLCOLOR_RECT CONCAT2E(encodeFullColorRect,BPP)
#define ENCODE_MONO_RECT CONCAT2E(encodeMonoRect,BPP)
#define ENCODE_INDEXED_RECT CONCAT2E(encodeIndexedRect,BPP)
#define PREPARE_JPEG_ROW CONCAT2E(prepareJpegRow,BPP)
#define ENCODE_JPEG_RECT CONCAT2E(encodeJpegRect,BPP)
#define FILL_PALETTE CONCAT2E(fillPalette,BPP)

#ifndef TIGHT_ONCE
#define TIGHT_ONCE

//
// C-style structures to store palette entries and compression paramentes.
// Such code probably should be converted into C++ classes.
//

struct TIGHT_COLOR_LIST {
  TIGHT_COLOR_LIST *next;
  int idx;
  rdr::U32 rgb;
};

struct TIGHT_PALETTE_ENTRY {
  TIGHT_COLOR_LIST *listNode;
  int numPixels;
};

struct TIGHT_PALETTE {
  TIGHT_PALETTE_ENTRY entry[256];
  TIGHT_COLOR_LIST *hash[256];
  TIGHT_COLOR_LIST list[256];
};

// FIXME: Is it really a good idea to use static variables for this?
static int s_endianMismatch;      // local/remote formats differ in byte order
static bool s_pack24;             // use 24-bit packing for 32-bit pixels
static int s_rs, s_gs, s_bs;      // shifts for 24-bit pixel conversion

// FIXME: Make a separate class for palette operations.
static int s_palMaxColors, s_palNumColors;
static rdr::U32 s_monoBackground, s_monoForeground;
static TIGHT_PALETTE s_palette;

//
// Swapping bytes in pixels.
// FIXME: Use a sort of ImageGetter that does not convert pixel format?
//

#ifndef SWAP16
#define SWAP16(n) ((((n) & 0xff) << 8) | (((n) >> 8) & 0xff))
#endif
#ifndef SWAP32
#define SWAP32(n) (((n) >> 24) | (((n) & 0x00ff0000) >> 8) | \
                   (((n) & 0x0000ff00) << 8) | ((n) << 24))
#endif

//
// Functions to operate on palette structures.
//

#define HASH_FUNC16(rgb) ((int)(((rgb >> 8) + rgb) & 0xFF))
#define HASH_FUNC32(rgb) ((int)(((rgb >> 16) + (rgb >> 8)) & 0xFF))

static void paletteReset(void)
{
  s_palNumColors = 0;
  memset(s_palette.hash, 0, 256 * sizeof(TIGHT_COLOR_LIST *));
}

static int paletteInsert(rdr::U32 rgb, int numPixels, int bpp)
{
  TIGHT_COLOR_LIST *pnode;
  TIGHT_COLOR_LIST *prev_pnode = NULL;
  int hash_key, idx, new_idx, count;

  hash_key = (bpp == 16) ? HASH_FUNC16(rgb) : HASH_FUNC32(rgb);

  pnode = s_palette.hash[hash_key];

  while (pnode != NULL) {
    if (pnode->rgb == rgb) {
      // Such palette entry already exists.
      new_idx = idx = pnode->idx;
      count = s_palette.entry[idx].numPixels + numPixels;
      if (new_idx && s_palette.entry[new_idx-1].numPixels < count) {
        do {
          s_palette.entry[new_idx] = s_palette.entry[new_idx-1];
          s_palette.entry[new_idx].listNode->idx = new_idx;
          new_idx--;
        }
        while (new_idx &&
          s_palette.entry[new_idx-1].numPixels < count);
        s_palette.entry[new_idx].listNode = pnode;
        pnode->idx = new_idx;
      }
      s_palette.entry[new_idx].numPixels = count;
      return s_palNumColors;
    }
    prev_pnode = pnode;
    pnode = pnode->next;
  }

  // Check if palette is full.
  if ( s_palNumColors == 256 || s_palNumColors == s_palMaxColors ) {
    s_palNumColors = 0;
    return 0;
  }

  // Move palette entries with lesser pixel counts.
  for ( idx = s_palNumColors;
  idx > 0 && s_palette.entry[idx-1].numPixels < numPixels;
  idx-- ) {
    s_palette.entry[idx] = s_palette.entry[idx-1];
    s_palette.entry[idx].listNode->idx = idx;
  }

  // Add new palette entry into the freed slot.
  pnode = &s_palette.list[s_palNumColors];
  if (prev_pnode != NULL) {
    prev_pnode->next = pnode;
  } else {
    s_palette.hash[hash_key] = pnode;
  }
  pnode->next = NULL;
  pnode->idx = idx;
  pnode->rgb = rgb;
  s_palette.entry[idx].listNode = pnode;
  s_palette.entry[idx].numPixels = numPixels;

  return (++s_palNumColors);
}

//
// Compress the data (but do not perform actual compression if the data
// size is less than TIGHT_MIN_TO_COMPRESS bytes.
//

static void compressData(rdr::OutStream *os, rdr::ZlibOutStream *zos,
                         const void *buf, unsigned int length, int zlibLevel)
{
  if (length < TIGHT_MIN_TO_COMPRESS) {
    os->writeBytes(buf, length);
  } else {
    // FIXME: Using a temporary MemOutStream may be not efficient.
    //        Maybe use the same static object used in the JPEG coder?
    rdr::MemOutStream mem_os;
    zos->setUnderlying(&mem_os);
    zos->setCompressionLevel(zlibLevel);
    zos->writeBytes(buf, length);
    zos->flush();
    os->writeCompactLength(mem_os.length());
    os->writeBytes(mem_os.data(), mem_os.length());
  }
}

//
// Destination manager implementation for the JPEG library.
// FIXME: Implement JPEG compression in new rdr::JpegOutStream class.
//

// FIXME: Keeping a MemOutStream instance may consume too much space.
rdr::MemOutStream s_jpeg_os;

static struct jpeg_destination_mgr s_jpegDstManager;
static JOCTET *s_jpegDstBuffer;
static size_t s_jpegDstBufferLen;

static void
JpegInitDestination(j_compress_ptr cinfo)
{
  s_jpeg_os.clear();
  s_jpegDstManager.next_output_byte = s_jpegDstBuffer;
  s_jpegDstManager.free_in_buffer = s_jpegDstBufferLen;
}

static boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
  s_jpeg_os.writeBytes(s_jpegDstBuffer, s_jpegDstBufferLen);
  s_jpegDstManager.next_output_byte = s_jpegDstBuffer;
  s_jpegDstManager.free_in_buffer = s_jpegDstBufferLen;

  return TRUE;
}

static void
JpegTermDestination(j_compress_ptr cinfo)
{
  int dataLen = s_jpegDstBufferLen - s_jpegDstManager.free_in_buffer;
  s_jpeg_os.writeBytes(s_jpegDstBuffer, dataLen);
}

static void
JpegSetDstManager(j_compress_ptr cinfo, JOCTET *buf, size_t buflen)
{
  s_jpegDstBuffer = buf;
  s_jpegDstBufferLen = buflen;
  s_jpegDstManager.init_destination = JpegInitDestination;
  s_jpegDstManager.empty_output_buffer = JpegEmptyOutputBuffer;
  s_jpegDstManager.term_destination = JpegTermDestination;
  cinfo->dest = &s_jpegDstManager;
}

#endif  // #ifndef TIGHT_ONCE

static void ENCODE_SOLID_RECT     (rdr::OutStream *os,
                                   PIXEL_T *buf);
static void ENCODE_FULLCOLOR_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const Rect& r);
static void ENCODE_MONO_RECT      (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const Rect& r);
#if (BPP != 8)
static void ENCODE_INDEXED_RECT   (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const Rect& r);
static void ENCODE_JPEG_RECT      (rdr::OutStream *os,
                                   PIXEL_T *buf, const PixelFormat& pf, const Rect& r);
#endif

static void FILL_PALETTE (PIXEL_T *data, int count);

//
// Convert 32-bit color samples into 24-bit colors, in place.
// Performs packing only when redMax, greenMax and blueMax are all 255.
// Color components are assumed to be byte-aligned.
//

static inline unsigned int PACK_PIXELS (PIXEL_T *buf, unsigned int count)
{
#if (BPP != 32)
  return count * sizeof(PIXEL_T);
#else
  if (!s_pack24)
    return count * sizeof(PIXEL_T);

  rdr::U32 pix;
  rdr::U8 *dst = (rdr::U8 *)buf;
  for (unsigned int i = 0; i < count; i++) {
    pix = *buf++;
    *dst++ = (rdr::U8)(pix >> s_rs);
    *dst++ = (rdr::U8)(pix >> s_gs);
    *dst++ = (rdr::U8)(pix >> s_bs);
  }
  return count * 3;
#endif
}

//
// Function to guess if a given rectangle is suitable for JPEG compression.
// Returns true is it looks like a good data for JPEG, false otherwise.
//
// FIXME: Scan the image and determine is it really good for JPEG.
//

#if (BPP != 8)
static bool DETECT_SMOOTH_IMAGE (PIXEL_T *buf, const Rect& r)
{
  if (r.width() < TIGHT_DETECT_MIN_WIDTH ||
      r.height() < TIGHT_DETECT_MIN_HEIGHT ||
      r.area() < TIGHT_JPEG_MIN_RECT_SIZE ||
      s_pjconf == NULL)
    return 0;

  return 1;
}
#endif

// FIXME: Split rectangles into smaller ones!
// FIXME: Compare encoder code with 1.3 before the final version.

//
// Main function of the Tight encoder
//

void TIGHT_ENCODE (const Rect& r, rdr::OutStream *os,
                  rdr::ZlibOutStream zos[4], void* buf, ConnParams* cp
#ifdef EXTRA_ARGS
                  , EXTRA_ARGS
#endif
                  )
{
#if (BPP != 8) || (BPP == 32)
  const PixelFormat& pf = cp->pf();
#endif
  GET_IMAGE_INTO_BUF(r, buf);
  PIXEL_T* pixels = (PIXEL_T*)buf;

#if (BPP != 8)
  union {
    rdr::U32 value32;
    rdr::U8 test;
  } littleEndian;
  littleEndian.value32 = 1;
  s_endianMismatch = (littleEndian.test != !pf.bigEndian);
#endif

#if (BPP == 32)
  // Check if it's necessary to pack 24-bit pixels, and
  // compute appropriate shift values if necessary.
  s_pack24 = (pf.depth == 24 && pf.redMax == 0xFF &&
              pf.greenMax == 0xFF && pf.blueMax == 0xFF);
  if (s_pack24) {
    if (!s_endianMismatch) {
      s_rs = pf.redShift;
      s_gs = pf.greenShift;
      s_bs = pf.blueShift;
    } else {
      s_rs = 24 - pf.redShift;
      s_gs = 24 - pf.greenShift;
      s_bs = 24 - pf.blueShift;
    }
  }
#endif

  s_palMaxColors = r.area() / s_pconf->idxMaxColorsDivisor;
  if (s_palMaxColors < 2 && r.area() >= s_pconf->monoMinRectSize) {
    s_palMaxColors = 2;
  }
  // FIXME: Temporary limitation for switching to JPEG earlier.
  if (s_palMaxColors > 96 && s_pjconf != NULL) {
    s_palMaxColors = 96;
  }

  FILL_PALETTE(pixels, r.area());

  switch (s_palNumColors) {
  case 0:
    // Truecolor image
#if (BPP != 8)
    if (s_pjconf != NULL && DETECT_SMOOTH_IMAGE(pixels, r)) {
      ENCODE_JPEG_RECT(os, pixels, pf, r);
      break;
    }
#endif
    ENCODE_FULLCOLOR_RECT(os, zos, pixels, r);
    break;
  case 1:
    // Solid rectangle
    ENCODE_SOLID_RECT(os, pixels);
    break;
  case 2:
    // Two-color rectangle
    ENCODE_MONO_RECT(os, zos, pixels, r);
    break;
#if (BPP != 8)
  default:
    // Up to 256 different colors
    ENCODE_INDEXED_RECT(os, zos, pixels, r);
#endif
  }
}

//
// Subencoding implementations.
//

static void ENCODE_SOLID_RECT (rdr::OutStream *os, PIXEL_T *buf)
{
  os->writeU8(0x08 << 4);

  int length = PACK_PIXELS(buf, 1);
  os->writeBytes(buf, length);
}

static void ENCODE_FULLCOLOR_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                   PIXEL_T *buf, const Rect& r)
{
  const int streamId = 0;
  os->writeU8(streamId << 4);

  int length = PACK_PIXELS(buf, r.area());
  compressData(os, &zos[streamId], buf, length, s_pconf->rawZlibLevel);
}

static void ENCODE_MONO_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                              PIXEL_T *buf, const Rect& r)
{
  const int streamId = 1;
  os->writeU8((streamId | 0x04) << 4);
  os->writeU8(0x01);

  // Write the palette
  PIXEL_T pal[2] = { (PIXEL_T)s_monoBackground, (PIXEL_T)s_monoForeground };
  os->writeU8(1);
  os->writeBytes(pal, PACK_PIXELS(pal, 2));

  // Encode the data in-place
  PIXEL_T *src = buf;
  rdr::U8 *dst = (rdr::U8 *)buf;
  int w = r.width();
  int h = r.height();
  PIXEL_T bg;
  unsigned int value, mask;
  int aligned_width;
  int x, y, bg_bits;

  bg = (PIXEL_T) s_monoBackground;
  aligned_width = w - w % 8;

  for (y = 0; y < h; y++) {
    for (x = 0; x < aligned_width; x += 8) {
      for (bg_bits = 0; bg_bits < 8; bg_bits++) {
        if (*src++ != bg)
          break;
      }
      if (bg_bits == 8) {
        *dst++ = 0;
        continue;
      }
      mask = 0x80 >> bg_bits;
      value = mask;
      for (bg_bits++; bg_bits < 8; bg_bits++) {
        mask >>= 1;
        if (*src++ != bg) {
          value |= mask;
        }
      }
      *dst++ = (rdr::U8)value;
    }

    mask = 0x80;
    value = 0;
    if (x >= w)
      continue;

    for (; x < w; x++) {
      if (*src++ != bg) {
        value |= mask;
      }
      mask >>= 1;
    }
    *dst++ = (rdr::U8)value;
  }

  // Write the data
  int length = (w + 7) / 8;
  length *= h;
  compressData(os, &zos[streamId], buf, length, s_pconf->monoZlibLevel);
}

#if (BPP != 8)
static void ENCODE_INDEXED_RECT (rdr::OutStream *os, rdr::ZlibOutStream zos[4],
                                 PIXEL_T *buf, const Rect& r)
{
  const int streamId = 2;
  os->writeU8((streamId | 0x04) << 4);
  os->writeU8(0x01);

  // Write the palette
  {
    PIXEL_T pal[256];
    for (int i = 0; i < s_palNumColors; i++)
      pal[i] = (PIXEL_T)s_palette.entry[i].listNode->rgb;
    os->writeU8((rdr::U8)(s_palNumColors - 1));
    os->writeBytes(pal, PACK_PIXELS(pal, s_palNumColors));
  }

  // Encode data in-place
  PIXEL_T *src = buf;
  rdr::U8 *dst = (rdr::U8 *)buf;
  int count = r.area();
  PIXEL_T rgb;
  TIGHT_COLOR_LIST *pnode;
  int rep = 0;

  while (count--) {
    rgb = *src++;
    while (count && *src == rgb) {
      rep++, src++, count--;
    }
    pnode = s_palette.hash[HASH_FUNCTION(rgb)];
    while (pnode != NULL) {
      if ((PIXEL_T)pnode->rgb == rgb) {
        *dst++ = (rdr::U8)pnode->idx;
        while (rep) {
          *dst++ = (rdr::U8)pnode->idx;
          rep--;
        }
        break;
      }
      pnode = pnode->next;
    }
  }

  // Write the data
  compressData(os, &zos[streamId], buf, r.area(), s_pconf->idxZlibLevel);
}
#endif  // #if (BPP != 8)

//
// JPEG compression.
//

#if (BPP != 8)
static void PREPARE_JPEG_ROW (PIXEL_T *src, const PixelFormat& pf,
                              rdr::U8 *dst, int count)
{
  // FIXME: Add a version of this function optimized for 24-bit colors?
  PIXEL_T pix;
  while (count--) {
    pix = *src++;
    if (s_endianMismatch)
      pix = SWAP_PIXEL(pix);
    *dst++ = (rdr::U8)((pix >> pf.redShift   & pf.redMax)   * 255 / pf.redMax);
    *dst++ = (rdr::U8)((pix >> pf.greenShift & pf.greenMax) * 255 / pf.greenMax);
    *dst++ = (rdr::U8)((pix >> pf.blueShift  & pf.blueMax)  * 255 / pf.blueMax);
  }
}
#endif  // #if (BPP != 8)

#if (BPP != 8)
static void ENCODE_JPEG_RECT (rdr::OutStream *os, PIXEL_T *buf,
                              const PixelFormat& pf, const Rect& r)
{
  int w = r.width();
  int h = r.height();

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  // FIXME: Make srcBuf[] and/or dstBuf[] static?
  rdr::U8 *srcBuf = new rdr::U8[w * 3];
  JSAMPROW rowPointer[1];
  rowPointer[0] = (JSAMPROW)srcBuf;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, s_pjconf->jpegQuality, TRUE);

  rdr::U8 *dstBuf = new rdr::U8[2048];
  JpegSetDstManager(&cinfo, dstBuf, 2048);

  jpeg_start_compress(&cinfo, TRUE);
  for (int dy = 0; dy < h; dy++) {
    PREPARE_JPEG_ROW(&buf[dy * w], pf, srcBuf, w);
    jpeg_write_scanlines(&cinfo, rowPointer, 1);
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  delete[] srcBuf;
  delete[] dstBuf;

  os->writeU8(0x09 << 4);
  os->writeCompactLength(s_jpeg_os.length());
  os->writeBytes(s_jpeg_os.data(), s_jpeg_os.length());
}
#endif  // #if (BPP != 8)

//
// Determine the number of colors in the rectangle, and fill in the palette.
//

#if (BPP == 8)
static void FILL_PALETTE (PIXEL_T *data, int count)
{
  PIXEL_T c0, c1;
  int i, n0, n1;

  s_palNumColors = 0;

  c0 = data[0];
  for (i = 1; i < count && data[i] == c0; i++);
  if (i == count) {
    s_palNumColors = 1;
    return;                       // Solid rectangle
  }

  if (s_palMaxColors < 2)
    return;

  n0 = i;
  c1 = data[i];
  n1 = 0;
  for (i++; i < count; i++) {
    if (data[i] == c0) {
      n0++;
    } else if (data[i] == c1) {
      n1++;
    } else
      break;
  }
  if (i == count) {
    if (n0 > n1) {
      s_monoBackground = (rdr::U32)c0;
      s_monoForeground = (rdr::U32)c1;
    } else {
      s_monoBackground = (rdr::U32)c1;
      s_monoForeground = (rdr::U32)c0;
    }
    s_palNumColors = 2;           // Two colors
  }
}
#else   // (BPP != 8)
static void FILL_PALETTE (PIXEL_T *data, int count)
{
  PIXEL_T c0, c1, ci = 0;
  int i, n0, n1, ni;

  c0 = data[0];
  for (i = 1; i < count && data[i] == c0; i++);
  if (i >= count) {
    s_palNumColors = 1;           // Solid rectangle
    return;
  }

  if (s_palMaxColors < 2) {
    s_palNumColors = 0;           // Full-color format preferred
    return;
  }

  n0 = i;
  c1 = data[i];
  n1 = 0;
  for (i++; i < count; i++) {
    ci = data[i];
    if (ci == c0) {
      n0++;
    } else if (ci == c1) {
      n1++;
    } else
      break;
  }
  if (i >= count) {
    if (n0 > n1) {
      s_monoBackground = (rdr::U32)c0;
      s_monoForeground = (rdr::U32)c1;
    } else {
      s_monoBackground = (rdr::U32)c1;
      s_monoForeground = (rdr::U32)c0;
    }
    s_palNumColors = 2;           // Two colors
    return;
  }

  paletteReset();
  paletteInsert (c0, (rdr::U32)n0, BPP);
  paletteInsert (c1, (rdr::U32)n1, BPP);

  ni = 1;
  for (i++; i < count; i++) {
    if (data[i] == ci) {
      ni++;
    } else {
      if (!paletteInsert (ci, (rdr::U32)ni, BPP))
        return;
      ci = data[i];
      ni = 1;
    }
  }
  paletteInsert (ci, (rdr::U32)ni, BPP);
}
#endif  // #if (BPP == 8)

#undef PIXEL_T
#undef WRITE_PIXEL
#undef TIGHT_ENCODE
#undef SWAP_PIXEL
#undef HASH_FUNCTION
#undef PACK_PIXELS
#undef DETECT_SMOOTH_IMAGE
#undef ENCODE_SOLID_RECT
#undef ENCODE_FULLCOLOR_RECT
#undef ENCODE_MONO_RECT
#undef ENCODE_INDEXED_RECT
#undef PREPARE_JPEG_ROW
#undef ENCODE_JPEG_RECT
#undef FILL_PALETTE
}
