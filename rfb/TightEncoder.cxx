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
#include <rdr/OutStream.h>
#include <rfb/ImageGetter.h>
#include <rfb/encodings.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriter.h>
#include <rfb/TightEncoder.h>

using namespace rfb;

// Minimum amount of data to be compressed. This value should not be
// changed, doing so will break compatibility with existing clients.
#define TIGHT_MIN_TO_COMPRESS 12

// Adjustable parameters.
// FIXME: Get rid of #defines
#define TIGHT_JPEG_MIN_RECT_SIZE 2048
#define TIGHT_DETECT_SUBROW_WIDTH   7
#define TIGHT_DETECT_MIN_WIDTH      8
#define TIGHT_DETECT_MIN_HEIGHT     8

//
// Compression level stuff. The following array contains various
// encoder parameters for each of 10 compression levels (0..9).
// Last three parameters correspond to JPEG quality levels (0..9).
//
// NOTE: s_conf[9].maxRectSize should be >= s_conf[i].maxRectSize,
// where i in [0..8]. RequiredBuffSize() method depends on this.
// FIXME: Is this comment obsolete?
//

const TIGHT_CONF TightEncoder::conf[10] = {
  {   512,  32,   6, 0, 0, 0,   4,  5, 10000, 23000 },
  {   768,  32,   6, 1, 1, 1,   8, 10,  8000, 18000 },
  {  1024,  32,   8, 3, 3, 2,  24, 15,  6500, 15000 },
  {  1536,  48,  12, 5, 5, 3,  32, 25,  5000, 12000 },
  {  2048,  48,  12, 6, 6, 4,  32, 37,  4000, 10000 },
  {  3072,  64,  12, 7, 7, 5,  32, 50,  3000,  8000 },
  {  4096,  64,  16, 7, 7, 6,  48, 60,  2000,  5000 },
  {  6144,  64,  16, 8, 8, 7,  64, 70,  1000,  2500 },
  {  8192, 128,  24, 9, 9, 8,  64, 75,   500,  1200 },
  { 10240, 128,  32, 9, 9, 9,  96, 80,   200,   500 }
};
const int TightEncoder::defaultCompressLevel = 6;

// FIXME: Not good to mirror TightEncoder's members here.
static const TIGHT_CONF* s_pconf;
static const TIGHT_CONF* s_pjconf;

//
// Including BPP-dependent implementation of the encoder.
//

#define EXTRA_ARGS ImageGetter* ig
#define GET_IMAGE_INTO_BUF(r,buf) ig->getImage(buf, r);
#define BPP 8
#include <rfb/tightEncode.h>
#undef BPP
#define BPP 16
#include <rfb/tightEncode.h>
#undef BPP
#define BPP 32
#include <rfb/tightEncode.h>
#undef BPP

Encoder* TightEncoder::create(SMsgWriter* writer)
{
  return new TightEncoder(writer);
} 

TightEncoder::TightEncoder(SMsgWriter* writer_) : writer(writer_)
{
  setCompressLevel(defaultCompressLevel);
  setQualityLevel(-1);
}

TightEncoder::~TightEncoder()
{
}

void TightEncoder::setCompressLevel(int level)
{
  if (level >= 0 && level <= 9) {
    pconf = &conf[level];
  } else {
    pconf = &conf[defaultCompressLevel];
  }
}

void TightEncoder::setQualityLevel(int level)
{
  if (level >= 0 && level <= 9) {
    pjconf = &conf[level];
  } else {
    pjconf = NULL;
  }
}

//int TightEncoder::getNumRects(const Rect &r)
//{
//  const unsigned int w = r.width();
//  const unsigned int h = r.height();
//
//  // Will this rectangle split into subrects?
//  bool rectTooBig = w > pconf->maxRectWidth || w * h > pconf->maxRectSize;
//  if (!rectTooBig)
//    return 1;
//
//  // Compute max sub-rectangle size.
//  const unsigned int subrectMaxWidth =
//    (w > pconf->maxRectWidth) ? pconf->maxRectWidth : w;
//  const unsigned int subrectMaxHeight =
//    pconf->maxRectSize / subrectMaxWidth;
//
//  // Return the number of subrects.
//  return (((w - 1) / pconf->maxRectWidth + 1) *
//          ((h - 1) / subrectMaxHeight + 1));
//}
//
bool TightEncoder::writeRect(const Rect& r, ImageGetter* ig, Rect* actual)
{
  // Shortcuts to rectangle coordinates and dimensions.
  const int x = r.tl.x;
  const int y = r.tl.y;
  const unsigned int w = r.width();
  const unsigned int h = r.height();

  // Copy members of current TightEncoder instance to static variables.
  s_pconf = pconf;
  s_pjconf = pjconf;

  // Encode small rects as is.
  bool rectTooBig = w > pconf->maxRectWidth || w * h > pconf->maxRectSize;
  if (!rectTooBig) {
    writeSubrect(r, ig);
    return true;
  }

  // Compute max sub-rectangle size.
  const unsigned int subrectMaxWidth =
    (w > pconf->maxRectWidth) ? pconf->maxRectWidth : w;
  const unsigned int subrectMaxHeight =
    pconf->maxRectSize / subrectMaxWidth;

  // Split big rects into separately encoded subrects.
  Rect sr;
  unsigned int dx, dy, sw, sh;
  for (dy = 0; dy < h; dy += subrectMaxHeight) {
    for (dx = 0; dx < w; dx += pconf->maxRectWidth) {
      sw = (dx + pconf->maxRectWidth < w) ? pconf->maxRectWidth : w - dx;
      sh = (dy + subrectMaxHeight < h) ? subrectMaxHeight : h - dy;
      sr.setXYWH(x + dx, y + dy, sw, sh);
      writeSubrect(sr, ig);
    }
  }
  return true;
}

void TightEncoder::writeSubrect(const Rect& r, ImageGetter* ig)
{
  rdr::U8* imageBuf = writer->getImageBuf(r.area());
  ConnParams* cp = writer->getConnParams();
  mos.clear();

  switch (writer->bpp()) {
  case 8:
    tightEncode8(r, &mos, zos, imageBuf, cp, ig);  break;
  case 16:
    tightEncode16(r, &mos, zos, imageBuf, cp, ig); break;
  case 32:
    tightEncode32(r, &mos, zos, imageBuf, cp, ig); break;
  }

  writer->startRect(r, encodingTight);
  rdr::OutStream* os = writer->getOutStream();
  os->writeBytes(mos.data(), mos.length());
  writer->endRect();
}
