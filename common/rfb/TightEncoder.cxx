/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#define TIGHT_MAX_SPLIT_TILE_SIZE      16
#define TIGHT_MIN_SPLIT_RECT_SIZE    4096
#define TIGHT_MIN_SOLID_SUBRECT_SIZE 2048

//
// Compression level stuff. The following array contains various
// encoder parameters for each of 10 compression levels (0..9).
// Last three parameters correspond to JPEG quality levels (0..9).
//
// NOTE: The parameters used in this encoder are the result of painstaking
// research by The VirtualGL Project using RFB session captures from a variety
// of both 2D and 3D applications.  See http://www.VirtualGL.org for the full
// reports.

// NOTE:  The JPEG quality and subsampling levels below were obtained
// experimentally by the VirtualGL Project.  They represent the approximate
// average compression ratios listed below, as measured across the set of
// every 10th frame in the SPECviewperf 9 benchmark suite.
//
// 9 = JPEG quality 100, no subsampling (ratio ~= 10:1)
//     [this should be lossless, except for round-off error]
// 8 = JPEG quality 92,  no subsampling (ratio ~= 20:1)
//     [this should be perceptually lossless, based on current research]
// 7 = JPEG quality 86,  no subsampling (ratio ~= 25:1)
// 6 = JPEG quality 79,  no subsampling (ratio ~= 30:1)
// 5 = JPEG quality 77,  4:2:2 subsampling (ratio ~= 40:1)
// 4 = JPEG quality 62,  4:2:2 subsampling (ratio ~= 50:1)
// 3 = JPEG quality 42,  4:2:2 subsampling (ratio ~= 60:1)
// 2 = JPEG quality 41,  4:2:0 subsampling (ratio ~= 70:1)
// 1 = JPEG quality 29,  4:2:0 subsampling (ratio ~= 80:1)
// 0 = JPEG quality 15,  4:2:0 subsampling (ratio ~= 100:1)

const TIGHT_CONF TightEncoder::conf[10] = {
  { 65536, 2048,   6, 0, 0, 0,   4, 24, 15, SUBSAMP_420 }, // 0
  { 65536, 2048,   6, 1, 1, 1,   8, 24, 29, SUBSAMP_420 }, // 1
  { 65536, 2048,   8, 3, 3, 2,  24, 96, 41, SUBSAMP_420 }, // 2
  { 65536, 2048,  12, 5, 5, 2,  32, 96, 42, SUBSAMP_422 }, // 3
  { 65536, 2048,  12, 6, 7, 3,  32, 96, 62, SUBSAMP_422 }, // 4
  { 65536, 2048,  12, 7, 8, 4,  32, 96, 77, SUBSAMP_422 }, // 5
  { 65536, 2048,  16, 7, 8, 5,  32, 96, 79, SUBSAMP_NONE }, // 6
  { 65536, 2048,  16, 8, 9, 6,  64, 96, 86, SUBSAMP_NONE }, // 7
  { 65536, 2048,  24, 9, 9, 7,  64, 96, 92, SUBSAMP_NONE }, // 8
  { 65536, 2048,  32, 9, 9, 9,  96, 96,100, SUBSAMP_NONE }  // 9
};
const int TightEncoder::defaultCompressLevel = 1;

//
// Including BPP-dependent implementation of the encoder.
//

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

bool TightEncoder::checkSolidTile(Rect& r, rdr::U32* colorPtr,
                                  bool needSameColor)
{
  switch (serverpf.bpp) {
  case 32:
    return checkSolidTile32(r, colorPtr, needSameColor);
  case 16:
    return checkSolidTile16(r, colorPtr, needSameColor);
  default:
    return checkSolidTile8(r, colorPtr, needSameColor);
  }
}

void TightEncoder::findBestSolidArea(Rect& r, rdr::U32 colorValue, Rect& bestr)
{
  int dx, dy, dw, dh;
  int w_prev;
  Rect sr;
  int w_best = 0, h_best = 0;

  bestr.tl.x = bestr.br.x = r.tl.x;
  bestr.tl.y = bestr.br.y = r.tl.y;

  w_prev = r.width();

  for (dy = r.tl.y; dy < r.br.y; dy += TIGHT_MAX_SPLIT_TILE_SIZE) {

    dh = (dy + TIGHT_MAX_SPLIT_TILE_SIZE <= r.br.y) ?
      TIGHT_MAX_SPLIT_TILE_SIZE : (r.br.y - dy);
    dw = (w_prev > TIGHT_MAX_SPLIT_TILE_SIZE) ?
      TIGHT_MAX_SPLIT_TILE_SIZE : w_prev;

    sr.setXYWH(r.tl.x, dy, dw, dh);
    if (!checkSolidTile(sr, &colorValue, true))
      break;

    for (dx = r.tl.x + dw; dx < r.tl.x + w_prev;) {
      dw = (dx + TIGHT_MAX_SPLIT_TILE_SIZE <= r.tl.x + w_prev) ?
        TIGHT_MAX_SPLIT_TILE_SIZE : (r.tl.x + w_prev - dx);
      sr.setXYWH(dx, dy, dw, dh);
      if (!checkSolidTile(sr, &colorValue, true))
        break;
      dx += dw;
    }

    w_prev = dx - r.tl.x;
    if (w_prev * (dy + dh - r.tl.y) > w_best * h_best) {
      w_best = w_prev;
      h_best = dy + dh - r.tl.y;
    }
  }

  bestr.br.x = bestr.tl.x + w_best;
  bestr.br.y = bestr.tl.y + h_best;
}

void TightEncoder::extendSolidArea(const Rect& r, rdr::U32 colorValue,
                                   Rect& er)
{
  int cx, cy;
  Rect sr;

  // Try to extend the area upwards.
  for (cy = er.tl.y - 1; ; cy--) {
    sr.setXYWH(er.tl.x, cy, er.width(), 1);
    if (cy < r.tl.y || !checkSolidTile(sr, &colorValue, true))
      break;
  }
  er.tl.y = cy + 1;

  // ... downwards.
  for (cy = er.br.y; ; cy++) {
    sr.setXYWH(er.tl.x, cy, er.width(), 1);
    if (cy >= r.br.y || !checkSolidTile(sr, &colorValue, true))
      break;
  }
  er.br.y = cy;

  // ... to the left.
  for (cx = er.tl.x - 1; ; cx--) {
    sr.setXYWH(cx, er.tl.y, 1, er.height());
    if (cx < r.tl.x || !checkSolidTile(sr, &colorValue, true))
      break;
  }
  er.tl.x = cx + 1;

  // ... to the right.
  for (cx = er.br.x; ; cx++) {
    sr.setXYWH(cx, er.tl.y, 1, er.height());
    if (cx >= r.br.x || !checkSolidTile(sr, &colorValue, true))
      break;
  }
  er.br.x = cx;
}

int TightEncoder::getNumRects(const Rect &r)
{
  ConnParams* cp = writer->getConnParams();
  const unsigned int w = r.width();
  const unsigned int h = r.height();

  // If last rect. encoding is enabled, we can use the higher-performance
  // code that pre-computes solid rectangles.  In that case, we don't care
  // about the rectangle count.
  if (cp->supportsLastRect && w * h >= TIGHT_MIN_SPLIT_RECT_SIZE)
    return 0;

  // Will this rectangle split into subrects?
  bool rectTooBig = w > pconf->maxRectWidth || w * h > pconf->maxRectSize;
  if (!rectTooBig)
    return 1;

  // Compute max sub-rectangle size.
  const unsigned int subrectMaxWidth =
    (w > pconf->maxRectWidth) ? pconf->maxRectWidth : w;
  const unsigned int subrectMaxHeight =
    pconf->maxRectSize / subrectMaxWidth;

  // Return the number of subrects.
  return (((w - 1) / pconf->maxRectWidth + 1) *
          ((h - 1) / subrectMaxHeight + 1));
}

void TightEncoder::sendRectSimple(const Rect& r)
{
  // Shortcuts to rectangle coordinates and dimensions.
  const int x = r.tl.x;
  const int y = r.tl.y;
  const unsigned int w = r.width();
  const unsigned int h = r.height();

  // Encode small rects as is.
  bool rectTooBig = w > pconf->maxRectWidth || w * h > pconf->maxRectSize;
  if (!rectTooBig) {
    writeSubrect(r);
    return;
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
      writeSubrect(sr);
    }
  }
}

bool TightEncoder::writeRect(const Rect& _r, TransImageGetter* _ig,
                             Rect* actual)
{
  ig = _ig;
  serverpf = ig->getPixelBuffer()->getPF();
  ConnParams* cp = writer->getConnParams();
  clientpf = cp->pf();

  // Shortcuts to rectangle coordinates and dimensions.
  Rect r = _r;
  int x = r.tl.x;
  int y = r.tl.y;
  unsigned int w = r.width();
  unsigned int h = r.height();

  // Encode small rects as is.
  if (!cp->supportsLastRect || w * h < TIGHT_MIN_SPLIT_RECT_SIZE) {
    sendRectSimple(r);
    return true;
  }

  // Split big rects into separately encoded subrects.
  Rect sr, bestr;
  unsigned int dx, dy, dw, dh;
  rdr::U32 colorValue;
  int maxRectSize = pconf->maxRectSize;
  int maxRectWidth = pconf->maxRectWidth;
  int nMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
  int nMaxRows = pconf->maxRectSize / nMaxWidth;

  // Try to find large solid-color areas and send them separately.
  for (dy = y; dy < y + h; dy += TIGHT_MAX_SPLIT_TILE_SIZE) {

    // If a rectangle becomes too large, send its upper part now.
    if (dy - y >= nMaxRows) {
      sr.setXYWH(x, y, w, nMaxRows);
      sendRectSimple(sr);
      r.tl.y += nMaxRows;
      y = r.tl.y;
      h = r.height();
    }

    dh = (dy + TIGHT_MAX_SPLIT_TILE_SIZE <= y + h) ?
      TIGHT_MAX_SPLIT_TILE_SIZE : (y + h - dy);

    for (dx = x; dx < x + w; dx += TIGHT_MAX_SPLIT_TILE_SIZE) {

      dw = (dx + TIGHT_MAX_SPLIT_TILE_SIZE <= x + w) ?
        TIGHT_MAX_SPLIT_TILE_SIZE : (x + w - dx);
 
      sr.setXYWH(dx, dy, dw, dh);
      if (checkSolidTile(sr, &colorValue, false)) {

        // Get dimensions of solid-color area.
        sr.setXYWH(dx, dy, r.br.x - dx, r.br.y - dy);
        findBestSolidArea(sr, colorValue, bestr);

        // Make sure a solid rectangle is large enough
        // (or the whole rectangle is of the same color).
        if (bestr.area() != r.area()
          && bestr.area() < TIGHT_MIN_SOLID_SUBRECT_SIZE)
          continue;

        // Try to extend solid rectangle to maximum size.
        extendSolidArea(r, colorValue, bestr);
 
        // Send rectangles at top and left to solid-color area.
        if (bestr.tl.y != y) {
          sr.setXYWH(x, y, w, bestr.tl.y - y);
          sendRectSimple(sr);
        }
        if (bestr.tl.x != x) {
          sr.setXYWH(x, bestr.tl.y, bestr.tl.x - x, bestr.height());
          writeRect(sr, _ig, NULL);
        }

        // Send solid-color rectangle.
        writeSubrect(bestr, true);

        // Send remaining rectangles (at right and bottom).
        if (bestr.br.x != r.br.x) {
          sr.setXYWH(bestr.br.x, bestr.tl.y, r.br.x - bestr.br.x,
            bestr.height());
          writeRect(sr, _ig, NULL);
        }
        if (bestr.br.y != r.br.y) {
          sr.setXYWH(x, bestr.br.y, w, r.br.y - bestr.br.y);
          writeRect(sr, _ig, NULL);
        }

        return true;
      }
    }
  }

  // No suitable solid-color rectangles found.
  sendRectSimple(r);
  return true;
}

void TightEncoder::writeSubrect(const Rect& r, bool forceSolid)
{
  mos.clear();

  switch (clientpf.bpp) {
  case 8:
    tightEncode8(r, &mos, forceSolid);  break;
  case 16:
    tightEncode16(r, &mos, forceSolid); break;
  case 32:
    tightEncode32(r, &mos, forceSolid); break;
  }

  writer->startRect(r, encodingTight);
  rdr::OutStream* os = writer->getOutStream();
  os->writeBytes(mos.data(), mos.length());
  writer->endRect();
}
