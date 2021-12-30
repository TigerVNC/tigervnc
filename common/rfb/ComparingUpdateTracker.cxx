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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

  // Used to efficiently crop the left and right of the change rectangle
  int minCompareWidthInPixels = BLOCK_SIZE / 8;
  int minCompareWidthInBytes = minCompareWidthInPixels * bytesPerPixel;

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

      // Scan the block top to bottom, to identify the first row of change
      for (int y = blockTop; y < blockBottom; y++)
      {
        if (memcmp(oldPtr, newPtr, blockWidthInBytes) != 0)
        {
          // Define the change rectangle using pessimistic values to start
          int changeHeight = blockBottom - y;
          int changeLeft = blockLeft;
          int changeRight = blockRight;

          // For every unchanged row at the bottom of the block, decrement change height
          {
            const rdr::U8* newRowPtr = newPtr + ((changeHeight - 1) * newStrideBytes);
            const rdr::U8* oldRowPtr = oldPtr + ((changeHeight - 1) * oldStrideBytes);
            while (changeHeight > 1 && memcmp(oldRowPtr, newRowPtr, blockWidthInBytes) == 0)
            {
              newRowPtr -= newStrideBytes;
              oldRowPtr -= oldStrideBytes;

              changeHeight--;
            }
          }

          // For every unchanged column at the left of the block, increment change left
          {
            const rdr::U8* newColumnPtr = newPtr;
            const rdr::U8* oldColumnPtr = oldPtr;
            while (changeLeft + minCompareWidthInPixels < changeRight)
            {
              const rdr::U8* newRowPtr = newColumnPtr;
              const rdr::U8* oldRowPtr = oldColumnPtr;
              for (int row = 0; row < changeHeight; row++)
              {
                if (memcmp(oldRowPtr, newRowPtr, minCompareWidthInBytes) != 0)
                  goto endOfChangeLeft;

                newRowPtr += newStrideBytes;
                oldRowPtr += oldStrideBytes;
              }

              newColumnPtr += minCompareWidthInBytes;
              oldColumnPtr += minCompareWidthInBytes;

              changeLeft += minCompareWidthInPixels;
            }
          }
        endOfChangeLeft:

          // For every unchanged column at the right of the block, decrement change right
          {
            const rdr::U8* newColumnPtr = newPtr + blockWidthInBytes;
            const rdr::U8* oldColumnPtr = oldPtr + blockWidthInBytes;
            while (changeLeft + minCompareWidthInPixels < changeRight)
            {
              newColumnPtr -= minCompareWidthInBytes;
              oldColumnPtr -= minCompareWidthInBytes;

              const rdr::U8* newRowPtr = newColumnPtr;
              const rdr::U8* oldRowPtr = oldColumnPtr;
              for (int row = 0; row < changeHeight; row++)
              {
                if (memcmp(oldRowPtr, newRowPtr, minCompareWidthInBytes) != 0)
                  goto endOfChangeRight;

                newRowPtr += newStrideBytes;
                oldRowPtr += oldStrideBytes;
              }

              changeRight -= minCompareWidthInPixels;
            }
          }
        endOfChangeRight:

          // Block change extends from (changeLeft, y) to (changeRight, y + changeHeight)
          newChanged->assign_union(Region(Rect(changeLeft, y, changeRight, y + changeHeight)));

          // Copy the change from fb to oldFb to allow future changes to be identified
          for (int row = 0; row < changeHeight; row++)
          {
            memcpy(oldPtr, newPtr, blockWidthInBytes);
            newPtr += newStrideBytes;
            oldPtr += oldStrideBytes;
          }

          // No further processing is required for this block
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
