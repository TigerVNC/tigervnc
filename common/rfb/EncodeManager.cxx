/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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
#include <rfb/EncodeManager.h>
#include <rfb/Encoder.h>
#include <rfb/Palette.h>
#include <rfb/SConnection.h>
#include <rfb/SMsgWriter.h>
#include <rfb/UpdateTracker.h>
#include <rfb/LogWriter.h>

#include <rfb/RawEncoder.h>
#include <rfb/RREEncoder.h>
#include <rfb/HextileEncoder.h>
#include <rfb/ZRLEEncoder.h>
#include <rfb/TightEncoder.h>
#include <rfb/TightJPEGEncoder.h>

using namespace rfb;

static LogWriter vlog("EncodeManager");

// Split each rectangle into smaller ones no larger than this area,
// and no wider than this width.
static const int SubRectMaxArea = 65536;
static const int SubRectMaxWidth = 2048;

// The size in pixels of either side of each block tested when looking
// for solid blocks.
static const int SolidSearchBlock = 16;
// Don't bother with blocks smaller than this
static const int SolidBlockMinArea = 2048;

namespace rfb {

enum EncoderClass {
  encoderRaw,
  encoderRRE,
  encoderHextile,
  encoderTight,
  encoderTightJPEG,
  encoderZRLE,
  encoderClassMax,
};

enum EncoderType {
  encoderSolid,
  encoderBitmap,
  encoderBitmapRLE,
  encoderIndexed,
  encoderIndexedRLE,
  encoderFullColour,
  encoderTypeMax,
};

struct RectInfo {
  int rleRuns;
  Palette palette;
};

};

static const char *encoderClassName(EncoderClass klass)
{
  switch (klass) {
  case encoderRaw:
    return "Raw";
  case encoderRRE:
    return "RRE";
  case encoderHextile:
    return "Hextile";
  case encoderTight:
    return "Tight";
  case encoderTightJPEG:
    return "Tight (JPEG)";
  case encoderZRLE:
    return "ZRLE";
  }

  return "Unknown Encoder Class";
}

static const char *encoderTypeName(EncoderType type)
{
  switch (type) {
  case encoderSolid:
    return "Solid";
  case encoderBitmap:
    return "Bitmap";
  case encoderBitmapRLE:
    return "Bitmap RLE";
  case encoderIndexed:
    return "Indexed";
  case encoderIndexedRLE:
    return "Indexed RLE";
  case encoderFullColour:
    return "Full Colour";
  }

  return "Unknown Encoder Type";
}

EncodeManager::EncodeManager(SConnection* conn_) : conn(conn_)
{
  StatsVector::iterator iter;

  encoders.resize(encoderClassMax, NULL);
  activeEncoders.resize(encoderTypeMax, encoderRaw);

  encoders[encoderRaw] = new RawEncoder(conn);
  encoders[encoderRRE] = new RREEncoder(conn);
  encoders[encoderHextile] = new HextileEncoder(conn);
  encoders[encoderTight] = new TightEncoder(conn);
  encoders[encoderTightJPEG] = new TightJPEGEncoder(conn);
  encoders[encoderZRLE] = new ZRLEEncoder(conn);

  updates = 0;
  stats.resize(encoderClassMax);
  for (iter = stats.begin();iter != stats.end();++iter) {
    StatsVector::value_type::iterator iter2;
    iter->resize(encoderTypeMax);
    for (iter2 = iter->begin();iter2 != iter->end();++iter2)
      memset(&*iter2, 0, sizeof(EncoderStats));
  }
}

EncodeManager::~EncodeManager()
{
  std::vector<Encoder*>::iterator iter;

  logStats();

  for (iter = encoders.begin();iter != encoders.end();iter++)
    delete *iter;
}

void EncodeManager::logStats()
{
  int i, j;

  unsigned rects;
  unsigned long long pixels, bytes, equivalent;

  double ratio;

  rects = 0;
  pixels = bytes = equivalent = 0;

  vlog.info("Framebuffer updates: %u", updates);

  for (i = 0;i < stats.size();i++) {
    // Did this class do anything at all?
    for (j = 0;j < stats[i].size();j++) {
      if (stats[i][j].rects != 0)
        break;
    }
    if (j == stats[i].size())
      continue;

    vlog.info("  %s:", encoderClassName((EncoderClass)i));

    for (j = 0;j < stats[i].size();j++) {
      if (stats[i][j].rects == 0)
        continue;

      rects += stats[i][j].rects;
      pixels += stats[i][j].pixels;
      bytes += stats[i][j].bytes;
      equivalent += stats[i][j].equivalent;

      ratio = (double)stats[i][j].equivalent / stats[i][j].bytes;

      vlog.info("    %s: %u rects, %llu pixels",
                encoderTypeName((EncoderType)j),
                stats[i][j].rects, stats[i][j].pixels);
      vlog.info("    %*s  %llu bytes (%g ratio)",
                strlen(encoderTypeName((EncoderType)j)), "",
                stats[i][j].bytes, ratio);
    }
  }

  ratio = (double)equivalent / bytes;

  vlog.info("  Total: %u rects, %llu pixels", rects, pixels);
  vlog.info("         %llu bytes (%g ratio)", bytes, ratio);
}

bool EncodeManager::supported(int encoding)
{
  switch (encoding) {
  case encodingRaw:
  case encodingRRE:
  case encodingHextile:
  case encodingZRLE:
  case encodingTight:
    return true;
  default:
    return false;
  }
}

void EncodeManager::writeUpdate(const UpdateInfo& ui, const PixelBuffer* pb,
                                const RenderedCursor* renderedCursor)
{
    int nRects;
    Region changed;

    updates++;

    prepareEncoders();

    if (conn->cp.supportsLastRect)
      nRects = 0xFFFF;
    else {
      nRects = ui.copied.numRects();
      nRects += computeNumRects(ui.changed);

      if (renderedCursor != NULL)
        nRects += 1;
    }

    conn->writer()->writeFramebufferUpdateStart(nRects);

    writeCopyRects(ui);

    /*
     * We start by searching for solid rects, which are then removed
     * from the changed region.
     */
    changed.copyFrom(ui.changed);

    if (conn->cp.supportsLastRect)
      writeSolidRects(&changed, pb);

    writeRects(changed, pb);

    if (renderedCursor != NULL) {
      Rect renderedCursorRect;

      renderedCursorRect = renderedCursor->getEffectiveRect();
      writeSubRect(renderedCursorRect, renderedCursor);
    }

    conn->writer()->writeFramebufferUpdateEnd();
}

void EncodeManager::prepareEncoders()
{
  enum EncoderClass solid, bitmap, bitmapRLE;
  enum EncoderClass indexed, indexedRLE, fullColour;

  rdr::S32 preferred;

  std::vector<int>::iterator iter;

  solid = bitmap = bitmapRLE = encoderRaw;
  indexed = indexedRLE = fullColour = encoderRaw;

  // Try to respect the client's wishes
  preferred = conn->getPreferredEncoding();
  switch (preferred) {
  case encodingRRE:
    // Horrible for anything high frequency and/or lots of colours
    bitmapRLE = indexedRLE = encoderRRE;
    break;
  case encodingHextile:
    // Slightly less horrible
    bitmapRLE = indexedRLE = fullColour = encoderHextile;
    break;
  case encodingTight:
    if (encoders[encoderTightJPEG]->isSupported() &&
        (conn->cp.pf().bpp >= 16))
      fullColour = encoderTightJPEG;
    else
      fullColour = encoderTight;
    indexed = indexedRLE = encoderTight;
    bitmap = bitmapRLE = encoderTight;
    break;
  case encodingZRLE:
    fullColour = encoderZRLE;
    bitmapRLE = indexedRLE = encoderZRLE;
    bitmap = indexed = encoderZRLE;
    break;
  }

  // Any encoders still unassigned?

  if (fullColour == encoderRaw) {
    if (encoders[encoderTightJPEG]->isSupported() &&
        (conn->cp.pf().bpp >= 16))
      fullColour = encoderTightJPEG;
    else if (encoders[encoderZRLE]->isSupported())
      fullColour = encoderZRLE;
    else if (encoders[encoderTight]->isSupported())
      fullColour = encoderTight;
    else if (encoders[encoderHextile]->isSupported())
      fullColour = encoderHextile;
  }

  if (indexed == encoderRaw) {
    if (encoders[encoderZRLE]->isSupported())
      indexed = encoderZRLE;
    else if (encoders[encoderTight]->isSupported())
      indexed = encoderTight;
    else if (encoders[encoderHextile]->isSupported())
      indexed = encoderHextile;
  }

  if (indexedRLE == encoderRaw)
    indexedRLE = indexed;

  if (bitmap == encoderRaw)
    bitmap = indexed;
  if (bitmapRLE == encoderRaw)
    bitmapRLE = bitmap;

  if (solid == encoderRaw) {
    if (encoders[encoderTight]->isSupported())
      solid = encoderTight;
    else if (encoders[encoderRRE]->isSupported())
      solid = encoderRRE;
    else if (encoders[encoderZRLE]->isSupported())
      solid = encoderZRLE;
    else if (encoders[encoderHextile]->isSupported())
      solid = encoderHextile;
  }

  // JPEG is the only encoder that can reduce things to grayscale
  if ((conn->cp.subsampling == subsampleGray) &&
      encoders[encoderTightJPEG]->isSupported()) {
    solid = bitmap = bitmapRLE = encoderTightJPEG;
    indexed = indexedRLE = fullColour = encoderTightJPEG;
  }

  activeEncoders[encoderSolid] = solid;
  activeEncoders[encoderBitmap] = bitmap;
  activeEncoders[encoderBitmapRLE] = bitmapRLE;
  activeEncoders[encoderIndexed] = indexed;
  activeEncoders[encoderIndexedRLE] = indexedRLE;
  activeEncoders[encoderFullColour] = fullColour;

  for (iter = activeEncoders.begin(); iter != activeEncoders.end(); ++iter) {
    Encoder *encoder;

    encoder = encoders[*iter];

    encoder->setCompressLevel(conn->cp.compressLevel);
    encoder->setQualityLevel(conn->cp.qualityLevel);
    encoder->setFineQualityLevel(conn->cp.fineQualityLevel,
                                 conn->cp.subsampling);
  }
}

int EncodeManager::computeNumRects(const Region& changed)
{
  int numRects;
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator rect;

  numRects = 0;
  changed.get_rects(&rects);
  for (rect = rects.begin(); rect != rects.end(); ++rect) {
    int w, h, sw, sh;

    w = rect->width();
    h = rect->height();

    // No split necessary?
    if (((w*h) < SubRectMaxArea) && (w < SubRectMaxWidth)) {
      numRects += 1;
      continue;
    }

    if (w <= SubRectMaxWidth)
      sw = w;
    else
      sw = SubRectMaxWidth;

    sh = SubRectMaxArea / sw;

    // ceil(w/sw) * ceil(h/sh)
    numRects += (((w - 1)/sw) + 1) * (((h - 1)/sh) + 1);
  }

  return numRects;
}

Encoder *EncodeManager::startRect(const Rect& rect, int type)
{
  Encoder *encoder;
  int klass, equiv;

  activeType = type;
  klass = activeEncoders[activeType];

  beforeLength = conn->getOutStream()->length();

  stats[klass][activeType].rects++;
  stats[klass][activeType].pixels += rect.area();
  equiv = 12 + rect.area() * conn->cp.pf().bpp/8;
  stats[klass][activeType].equivalent += equiv;

  encoder = encoders[klass];
  conn->writer()->startRect(rect, encoder->encoding);

  return encoder;
}

void EncodeManager::endRect()
{
  int klass;
  int length;

  conn->writer()->endRect();

  length = conn->getOutStream()->length() - beforeLength;

  klass = activeEncoders[activeType];
  stats[klass][activeType].bytes += length;
}

void EncodeManager::writeCopyRects(const UpdateInfo& ui)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator rect;

  ui.copied.get_rects(&rects, ui.copy_delta.x <= 0, ui.copy_delta.y <= 0);
  for (rect = rects.begin(); rect != rects.end(); ++rect) {
    conn->writer()->writeCopyRect(*rect, rect->tl.x - ui.copy_delta.x,
                                   rect->tl.y - ui.copy_delta.y);
  }
}

void EncodeManager::writeSolidRects(Region *changed, const PixelBuffer* pb)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator rect;

  changed->get_rects(&rects);
  for (rect = rects.begin(); rect != rects.end(); ++rect)
    findSolidRect(*rect, changed, pb);
}

void EncodeManager::findSolidRect(const Rect& rect, Region *changed,
                                  const PixelBuffer* pb)
{
  Rect sr;
  int dx, dy, dw, dh;

  // We start by finding a solid 16x16 block
  for (dy = rect.tl.y; dy < rect.br.y; dy += SolidSearchBlock) {

    dh = SolidSearchBlock;
    if (dy + dh > rect.br.y)
      dh = rect.br.y - dy;

    for (dx = rect.tl.x; dx < rect.br.x; dx += SolidSearchBlock) {
      // We define it like this to guarantee alignment
      rdr::U32 _buffer;
      rdr::U8* colourValue = (rdr::U8*)&_buffer;

      dw = SolidSearchBlock;
      if (dx + dw > rect.br.x)
        dw = rect.br.x - dx;

      pb->getImage(colourValue, Rect(dx, dy, dx+1, dy+1));

      sr.setXYWH(dx, dy, dw, dh);
      if (checkSolidTile(sr, colourValue, pb)) {
        Rect erb, erp;

        Encoder *encoder;

        // We then try extending the area by adding more blocks
        // in both directions and pick the combination that gives
        // the largest area.
        sr.setXYWH(dx, dy, rect.br.x - dx, rect.br.y - dy);
        extendSolidAreaByBlock(sr, colourValue, pb, &erb);

        // Did we end up getting the entire rectangle?
        if (erb.equals(rect))
          erp = erb;
        else {
          // Don't bother with sending tiny rectangles
          if (erb.area() < SolidBlockMinArea)
            continue;

          // Extend the area again, but this time one pixel
          // row/column at a time.
          extendSolidAreaByPixel(rect, erb, colourValue, pb, &erp);
        }

        // Send solid-color rectangle.
        encoder = startRect(erp, encoderSolid);
        if (encoder->flags & EncoderUseNativePF) {
          encoder->writeSolidRect(erp.width(), erp.height(),
                                  pb->getPF(), colourValue);
        } else {
          rdr::U32 _buffer2;
          rdr::U8* converted = (rdr::U8*)&_buffer2;

          conn->cp.pf().bufferFromBuffer(converted, pb->getPF(),
                                         colourValue, 1);

          encoder->writeSolidRect(erp.width(), erp.height(),
                                  conn->cp.pf(), converted);
        }
        endRect();

        changed->assign_subtract(Region(erp));

        // Search remaining areas by recursion
        // FIXME: Is this the best way to divide things up?

        // Left? (Note that we've already searched a SolidSearchBlock
        //        pixels high strip here)
        if ((erp.tl.x != rect.tl.x) && (erp.height() > SolidSearchBlock)) {
          sr.setXYWH(rect.tl.x, erp.tl.y + SolidSearchBlock,
                     erp.tl.x - rect.tl.x, erp.height() - SolidSearchBlock);
          findSolidRect(sr, changed, pb);
        }

        // Right?
        if (erp.br.x != rect.br.x) {
          sr.setXYWH(erp.br.x, erp.tl.y, rect.br.x - erp.br.x, erp.height());
          findSolidRect(sr, changed, pb);
        }

        // Below?
        if (erp.br.y != rect.br.y) {
          sr.setXYWH(rect.tl.x, erp.br.y, rect.width(), rect.br.y - erp.br.y);
          findSolidRect(sr, changed, pb);
        }

        return;
      }
    }
  }
}

void EncodeManager::writeRects(const Region& changed, const PixelBuffer* pb)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator rect;

  changed.get_rects(&rects);
  for (rect = rects.begin(); rect != rects.end(); ++rect) {
    int w, h, sw, sh;
    Rect sr;

    w = rect->width();
    h = rect->height();

    // No split necessary?
    if (((w*h) < SubRectMaxArea) && (w < SubRectMaxWidth)) {
      writeSubRect(*rect, pb);
      continue;
    }

    if (w <= SubRectMaxWidth)
      sw = w;
    else
      sw = SubRectMaxWidth;

    sh = SubRectMaxArea / sw;

    for (sr.tl.y = rect->tl.y; sr.tl.y < rect->br.y; sr.tl.y += sh) {
      sr.br.y = sr.tl.y + sh;
      if (sr.br.y > rect->br.y)
        sr.br.y = rect->br.y;

      for (sr.tl.x = rect->tl.x; sr.tl.x < rect->br.x; sr.tl.x += sw) {
        sr.br.x = sr.tl.x + sw;
        if (sr.br.x > rect->br.x)
          sr.br.x = rect->br.x;

        writeSubRect(sr, pb);
      }
    }
  }
}

void EncodeManager::writeSubRect(const Rect& rect, const PixelBuffer *pb)
{
  PixelBuffer *ppb;

  Encoder *encoder;

  struct RectInfo info;
  int divisor, maxColours;

  bool useRLE;
  EncoderType type;

  // FIXME: This is roughly the algorithm previously used by the Tight
  //        encoder. It seems a bit backwards though, that higher
  //        compression setting means spending less effort in building
  //        a palette. It might be that they figured the increase in
  //        zlib setting compensated for the loss.
  if (conn->cp.compressLevel == -1)
    divisor = 2 * 8;
  else
    divisor = conn->cp.compressLevel * 8;
  if (divisor < 4)
    divisor = 4;

  maxColours = rect.area()/divisor;

  // Special exception inherited from the Tight encoder
  if (activeEncoders[encoderFullColour] == encoderTightJPEG) {
    if (conn->cp.compressLevel < 2)
      maxColours = 24;
    else
      maxColours = 96;
  }

  if (maxColours < 2)
    maxColours = 2;

  encoder = encoders[activeEncoders[encoderIndexedRLE]];
  if (maxColours > encoder->maxPaletteSize)
    maxColours = encoder->maxPaletteSize;
  encoder = encoders[activeEncoders[encoderIndexed]];
  if (maxColours > encoder->maxPaletteSize)
    maxColours = encoder->maxPaletteSize;

  ppb = preparePixelBuffer(rect, pb, true);

  if (!analyseRect(ppb, &info, maxColours))
    info.palette.clear();

  // Different encoders might have different RLE overhead, but
  // here we do a guess at RLE being the better choice if reduces
  // the pixel count by 50%.
  useRLE = info.rleRuns <= (rect.area() * 2);

  switch (info.palette.size()) {
  case 0:
    type = encoderFullColour;
    break;
  case 1:
    type = encoderSolid;
    break;
  case 2:
    if (useRLE)
      type = encoderBitmapRLE;
    else
      type = encoderBitmap;
    break;
  default:
    if (useRLE)
      type = encoderIndexedRLE;
    else
      type = encoderIndexed;
  }

  encoder = startRect(rect, type);

  if (encoder->flags & EncoderUseNativePF)
    ppb = preparePixelBuffer(rect, pb, false);

  encoder->writeRect(ppb, info.palette);

  endRect();
}

bool EncodeManager::checkSolidTile(const Rect& r, const rdr::U8* colourValue,
                                   const PixelBuffer *pb)
{
  switch (pb->getPF().bpp) {
  case 32:
    return checkSolidTile(r, *(const rdr::U32*)colourValue, pb);
  case 16:
    return checkSolidTile(r, *(const rdr::U16*)colourValue, pb);
  default:
    return checkSolidTile(r, *(const rdr::U8*)colourValue, pb);
  }
}

void EncodeManager::extendSolidAreaByBlock(const Rect& r,
                                           const rdr::U8* colourValue,
                                           const PixelBuffer *pb, Rect* er)
{
  int dx, dy, dw, dh;
  int w_prev;
  Rect sr;
  int w_best = 0, h_best = 0;

  w_prev = r.width();

  // We search width first, back off when we hit a different colour,
  // and restart with a larger height. We keep track of the
  // width/height combination that gives us the largest area.
  for (dy = r.tl.y; dy < r.br.y; dy += SolidSearchBlock) {

    dh = SolidSearchBlock;
    if (dy + dh > r.br.y)
      dh = r.br.y - dy;

    // We test one block here outside the x loop in order to break
    // the y loop right away.
    dw = SolidSearchBlock;
    if (dw > w_prev)
      dw = w_prev;

    sr.setXYWH(r.tl.x, dy, dw, dh);
    if (!checkSolidTile(sr, colourValue, pb))
      break;

    for (dx = r.tl.x + dw; dx < r.tl.x + w_prev;) {

      dw = SolidSearchBlock;
      if (dx + dw > r.tl.x + w_prev)
        dw = r.tl.x + w_prev - dx;

      sr.setXYWH(dx, dy, dw, dh);
      if (!checkSolidTile(sr, colourValue, pb))
        break;

      dx += dw;
    }

    w_prev = dx - r.tl.x;
    if (w_prev * (dy + dh - r.tl.y) > w_best * h_best) {
      w_best = w_prev;
      h_best = dy + dh - r.tl.y;
    }
  }

  er->tl.x = r.tl.x;
  er->tl.y = r.tl.y;
  er->br.x = er->tl.x + w_best;
  er->br.y = er->tl.y + h_best;
}

void EncodeManager::extendSolidAreaByPixel(const Rect& r, const Rect& sr,
                                           const rdr::U8* colourValue,
                                           const PixelBuffer *pb, Rect* er)
{
  int cx, cy;
  Rect tr;

  // Try to extend the area upwards.
  for (cy = sr.tl.y - 1; cy >= r.tl.y; cy--) {
    tr.setXYWH(sr.tl.x, cy, sr.width(), 1);
    if (!checkSolidTile(tr, colourValue, pb))
      break;
  }
  er->tl.y = cy + 1;

  // ... downwards.
  for (cy = sr.br.y; cy < r.br.y; cy++) {
    tr.setXYWH(sr.tl.x, cy, sr.width(), 1);
    if (!checkSolidTile(tr, colourValue, pb))
      break;
  }
  er->br.y = cy;

  // ... to the left.
  for (cx = sr.tl.x - 1; cx >= r.tl.x; cx--) {
    tr.setXYWH(cx, er->tl.y, 1, er->height());
    if (!checkSolidTile(tr, colourValue, pb))
      break;
  }
  er->tl.x = cx + 1;

  // ... to the right.
  for (cx = sr.br.x; cx < r.br.x; cx++) {
    tr.setXYWH(cx, er->tl.y, 1, er->height());
    if (!checkSolidTile(tr, colourValue, pb))
      break;
  }
  er->br.x = cx;
}

PixelBuffer* EncodeManager::preparePixelBuffer(const Rect& rect,
                                               const PixelBuffer *pb,
                                               bool convert)
{
  const rdr::U8* buffer;
  int stride;

  // Do wo need to convert the data?
  if (convert && !conn->cp.pf().equal(pb->getPF())) {
    convertedPixelBuffer.setPF(conn->cp.pf());
    convertedPixelBuffer.setSize(rect.width(), rect.height());

    buffer = pb->getBuffer(rect, &stride);
    convertedPixelBuffer.imageRect(pb->getPF(),
                                   convertedPixelBuffer.getRect(),
                                   buffer, stride);

    return &convertedPixelBuffer;
  }

  // Otherwise we still need to shift the coordinates. We have our own
  // abusive subclass of FullFramePixelBuffer for this.

  buffer = pb->getBuffer(rect, &stride);

  offsetPixelBuffer.update(pb->getPF(), rect.width(), rect.height(),
                           buffer, stride);

  return &offsetPixelBuffer;
}

bool EncodeManager::analyseRect(const PixelBuffer *pb,
                                struct RectInfo *info, int maxColours)
{
  const rdr::U8* buffer;
  int stride;

  buffer = pb->getBuffer(pb->getRect(), &stride);

  switch (pb->getPF().bpp) {
  case 32:
    return analyseRect(pb->width(), pb->height(),
                       (const rdr::U32*)buffer, stride,
                       info, maxColours);
  case 16:
    return analyseRect(pb->width(), pb->height(),
                       (const rdr::U16*)buffer, stride,
                       info, maxColours);
  default:
    return analyseRect(pb->width(), pb->height(),
                       (const rdr::U8*)buffer, stride,
                       info, maxColours);
  }
}

void EncodeManager::OffsetPixelBuffer::update(const PixelFormat& pf,
                                              int width, int height,
                                              const rdr::U8* data_,
                                              int stride_)
{
  format = pf;
  width_ = width;
  height_ = height;
  // Forced cast. We never write anything though, so it should be safe.
  data = (rdr::U8*)data_;
  stride = stride_;
}

// Preprocessor generated, optimised methods

#define BPP 8
#include "EncodeManagerBPP.cxx"
#undef BPP
#define BPP 16
#include "EncodeManagerBPP.cxx"
#undef BPP
#define BPP 32
#include "EncodeManagerBPP.cxx"
#undef BPP
