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

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)

#define UBPP CONCAT2E(U,BPP)

inline bool EncodeManager::checkSolidTile(const Rect& r,
                                          rdr::UBPP colourValue,
                                          const PixelBuffer *pb)
{
  int w, h;
  const rdr::UBPP* buffer;
  int stride, pad;

  w = r.width();
  h = r.height();

  buffer = (const rdr::UBPP*)pb->getBuffer(r, &stride);
  pad = stride - w;

  while (h--) {
    int w_ = w;
    while (w_--) {
      if (*buffer != colourValue)
        return false;
      buffer++;
    }
    buffer += pad;
  }

  return true;
}

inline bool EncodeManager::analyseRect(int width, int height,
                                       const rdr::UBPP* buffer, int stride,
                                       struct RectInfo *info, int maxColours)
{
  int pad;

  rdr::UBPP colour;
  int count;

  info->rleRuns = 0;
  info->palette.clear();

  pad = stride - width;

  // For efficiency, we only update the palette on changes in colour
  colour = buffer[0];
  count = 0;
  while (height--) {
    int w_ = width;
    while (w_--) {
      if (*buffer != colour) {
        if (!info->palette.insert(colour, count))
          return false;
        if (info->palette.size() > maxColours)
          return false;

        // FIXME: This doesn't account for switching lines
        info->rleRuns++;

        colour = *buffer;
        count = 0;
      }
      buffer++;
      count++;
    }
    buffer += pad;
  }

  // Make sure the final pixels also get counted
  if (!info->palette.insert(colour, count))
    return false;
  if (info->palette.size() > maxColours)
    return false;

  return true;
}
