/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
#include <stdio.h>
#include <string.h>
#include <vector>
#include <rdr/types.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/ComparingUpdateTracker.h>

using namespace rfb;

static LogWriter vlog("ComparingUpdateTracker");

ComparingUpdateTracker::ComparingUpdateTracker(PixelBuffer* buffer)
  : fb(buffer), oldFb(fb->getPF(), 0, 0), firstCompare(true),
    enabled(true), totalPixels(0), missedPixels(0)
{
    changed.assign_union(fb->getRect());
}

ComparingUpdateTracker::~ComparingUpdateTracker()
{
}


#define BLOCK_SIZE 64

bool ComparingUpdateTracker::compare()
{
  std::vector<Rect> rects;
  std::vector<Rect>::iterator i;

  if (!enabled)
    return false;

  if (firstCompare) {
    // NB: We leave the change region untouched on this iteration,
    // since in effect the entire framebuffer has changed.
    oldFb.setSize(fb->width(), fb->height());

    for (int y=0; y<fb->height(); y+=BLOCK_SIZE) {
      Rect pos(0, y, fb->width(), __rfbmin(fb->height(), y+BLOCK_SIZE));
      int srcStride;
      const rdr::U8* srcData = fb->getBuffer(pos, &srcStride);
      oldFb.imageRect(pos, srcData, srcStride);
    }

    firstCompare = false;

    return false;
  }

  copied.get_rects(&rects, copy_delta.x<=0, copy_delta.y<=0);
  for (i = rects.begin(); i != rects.end(); i++)
    oldFb.copyRect(*i, copy_delta);

  changed.get_rects(&rects);

  Region newChanged;
  for (i = rects.begin(); i != rects.end(); i++)
    compareRect(*i, &newChanged);

  changed.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++)
    totalPixels += i->area();
  newChanged.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++)
    missedPixels += i->area();

  if (changed.equals(newChanged))
    return false;

  changed = newChanged;

  return true;
}

void ComparingUpdateTracker::enable()
{
  enabled = true;
}

void ComparingUpdateTracker::disable()
{
  enabled = false;

  // Make sure we update the framebuffer next time we get enabled
  firstCompare = true;
}

void ComparingUpdateTracker::compareRect(const Rect& r, Region* newChanged)
{
  if (!r.enclosed_by(fb->getRect())) {
    Rect safe;
    // Crop the rect and try again
    safe = r.intersect(fb->getRect());
    if (!safe.is_empty())
      compareRect(safe, newChanged);
    return;
  }

  int bytesPerPixel = fb->getPF().bpp/8;
  int oldStride;
  rdr::U8* oldData = oldFb.getBufferRW(r, &oldStride);
  int oldStrideBytes = oldStride * bytesPerPixel;

  std::vector<Rect> changedBlocks;

  for (int blockTop = r.tl.y; blockTop < r.br.y; blockTop += BLOCK_SIZE)
  {
    // Get a strip of the source buffer
    Rect pos(r.tl.x, blockTop, r.br.x, __rfbmin(r.br.y, blockTop+BLOCK_SIZE));
    int fbStride;
    const rdr::U8* newBlockPtr = fb->getBuffer(pos, &fbStride);
    int newStrideBytes = fbStride * bytesPerPixel;

    rdr::U8* oldBlockPtr = oldData;
    int blockBottom = __rfbmin(blockTop+BLOCK_SIZE, r.br.y);

    for (int blockLeft = r.tl.x; blockLeft < r.br.x; blockLeft += BLOCK_SIZE)
    {
      const rdr::U8* newPtr = newBlockPtr;
      rdr::U8* oldPtr = oldBlockPtr;

      int blockRight = __rfbmin(blockLeft+BLOCK_SIZE, r.br.x);
      int blockWidthInBytes = (blockRight-blockLeft) * bytesPerPixel;

      for (int y = blockTop; y < blockBottom; y++)
      {
        if (memcmp(oldPtr, newPtr, blockWidthInBytes) != 0)
        {
          // A block has changed - copy the remainder to the oldFb
          changedBlocks.push_back(Rect(blockLeft, blockTop,
                                       blockRight, blockBottom));
          for (int y2 = y; y2 < blockBottom; y2++)
          {
            memcpy(oldPtr, newPtr, blockWidthInBytes);
            newPtr += newStrideBytes;
            oldPtr += oldStrideBytes;
          }
          break;
        }

        newPtr += newStrideBytes;
        oldPtr += oldStrideBytes;
      }

      oldBlockPtr += blockWidthInBytes;
      newBlockPtr += blockWidthInBytes;
    }

    oldData += oldStrideBytes * BLOCK_SIZE;
  }

  oldFb.commitBufferRW(r);

  if (!changedBlocks.empty()) {
    Region temp;
    temp.setOrderedRects(changedBlocks);
    newChanged->assign_union(temp);
  }
}

void ComparingUpdateTracker::logStats()
{
  double ratio;
  char a[1024], b[1024];

  siPrefix(totalPixels, "pixels", a, sizeof(a));
  siPrefix(missedPixels, "pixels", b, sizeof(b));

  ratio = (double)totalPixels / missedPixels;

  vlog.info("%s in / %s out", a, b);
  vlog.info("(1:%g ratio)", ratio);

  totalPixels = missedPixels = 0;
}
