/* Copyright (C) 2004-2008 Constantin Kaplinsky.  All Rights Reserved.
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
// PollingManager.cxx
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServer.h>
#include <rfb/Configuration.h>
#include <rfb/ServerCore.h>

#include <x0vncserver/PollingManager.h>

using namespace rfb;

static LogWriter vlog("PollingMgr");

const int PollingManager::m_pollingOrder[32] = {
   0, 16,  8, 24,  4, 20, 12, 28,
  10, 26, 18,  2, 22,  6, 30, 14,
   1, 17,  9, 25,  7, 23, 15, 31,
  19,  3, 27, 11, 29, 13,  5, 21
};

//
// Constructor.
//
// Note that dpy and image should remain valid during the object
// lifetime, while factory is used only in the constructor itself.
//

PollingManager::PollingManager(Display *dpy, const Image *image,
                               ImageFactory &factory,
                               int offsetLeft, int offsetTop)
  : m_dpy(dpy),
    m_image(image),
    m_bytesPerPixel(image->xim->bits_per_pixel / 8),
    m_offsetLeft(offsetLeft),
    m_offsetTop(offsetTop),
    m_width(image->xim->width),
    m_height(image->xim->height),
    m_widthTiles((image->xim->width + 31) / 32),
    m_heightTiles((image->xim->height + 31) / 32),
    m_numTiles(((image->xim->width + 31) / 32) *
               ((image->xim->height + 31) / 32)),
    m_pollingStep(0)
{
  // Create additional images used in polling algorithm, warn if
  // underlying class names are different from the class name of the
  // primary image.
  m_rowImage = factory.newImage(m_dpy, m_width, 1);
  m_columnImage = factory.newImage(m_dpy, 1, m_height);
  const char *primaryImgClass = m_image->className();
  const char *rowImgClass = m_rowImage->className();
  const char *columnImgClass = m_columnImage->className();
  if (strcmp(rowImgClass, primaryImgClass) != 0 ||
      strcmp(columnImgClass, primaryImgClass) != 0) {
    vlog.error("Image types do not match (%s, %s, %s)",
               primaryImgClass, rowImgClass, columnImgClass);
  }

  m_changeFlags = new bool[m_numTiles];
  memset(m_changeFlags, 0, m_numTiles * sizeof(bool));
}

PollingManager::~PollingManager()
{
  delete[] m_changeFlags;

  delete m_rowImage;
  delete m_columnImage;
}

//
// DEBUG: Measuring time spent in the poll() function,
//        as well as time intervals between poll() calls.
//

#ifdef DEBUG
void PollingManager::debugBeforePoll()
{
  TimeMillis timeNow;
  int diff = timeNow.diffFrom(m_timeSaved);
  fprintf(stderr, "[wait%4dms]\t[step %2d]\t", diff, m_pollingStep % 32);
  m_timeSaved = timeNow;
}

void PollingManager::debugAfterPoll()
{
  TimeMillis timeNow;
  int diff = timeNow.diffFrom(m_timeSaved);
  fprintf(stderr, "[poll%4dms]\n", diff);
  m_timeSaved = timeNow;
}

#endif

//
// Search for changed rectangles on the screen.
//

void PollingManager::poll(VNCServer *server)
{
#ifdef DEBUG
  debugBeforePoll();
#endif

  pollScreen(server);

#ifdef DEBUG
  debugAfterPoll();
#endif
}

#ifdef DEBUG_REPORT_CHANGED_TILES
#define DBG_REPORT_CHANGES(title)  printChanges((title))
#else
#define DBG_REPORT_CHANGES(title)
#endif

bool PollingManager::pollScreen(VNCServer *server)
{
  if (!server)
    return false;

  // Clear the m_changeFlags[] array, indicating that no changes have
  // been detected yet.
  memset(m_changeFlags, 0, m_numTiles * sizeof(bool));

  // First pass over the framebuffer. Here we scan 1/32 part of the
  // framebuffer -- that is, one line in each (32 * m_width) stripe.
  // We compare the pixels of that line with previous framebuffer
  // contents and raise corresponding elements of m_changeFlags[].
  int scanOffset = m_pollingOrder[m_pollingStep++ % 32];
  int nTilesChanged = 0;
  for (int y = scanOffset; y < m_height; y += 32) {
    nTilesChanged += checkRow(0, y, m_width);
  }

  DBG_REPORT_CHANGES("After 1st pass");

  // If some changes have been detected:
  if (nTilesChanged) {
    // Try to find more changes around.
    checkNeighbors();
    DBG_REPORT_CHANGES("After checking neighbors");
    // Inform the server about the changes.
    nTilesChanged = sendChanges(server);
  }

#ifdef DEBUG_PRINT_NUM_CHANGED_TILES
  printf("%3d ", nTilesChanged);
  if (m_pollingStep % 32 == 0) {
    printf("\n");
  }
#endif

#ifdef DEBUG
  if (nTilesChanged != 0) {
    fprintf(stderr, "#%d# ", nTilesChanged);
  }
#endif

  return (nTilesChanged != 0);
}

int PollingManager::checkRow(int x, int y, int w)
{
  // If necessary, expand the row to the left, to the tile border.
  // In other words, x must be a multiple of 32.
  if (x % 32 != 0) {
    int correction = x % 32;
    x -= correction;
    w += correction;
  }

  // Read a row from the screen into m_rowImage.
  getRow(x, y, w);

  // Compute a pointer to the initial element of m_changeFlags.
  bool *pChangeFlags = &m_changeFlags[getTileIndex(x, y)];

  // Compute pointers to image data to be compared.
  char *ptr_old = m_image->locatePixel(x, y);
  char *ptr_new = m_rowImage->xim->data;

  // Compare pixels, raise corresponding elements of m_changeFlags[].
  // First, handle full-size (32 pixels wide) tiles.
  int nTilesChanged = 0;
  int nBytesPerTile = 32 * m_bytesPerPixel;
  for (int i = 0; i < w / 32; i++) {
    if (memcmp(ptr_old, ptr_new, nBytesPerTile)) {
      *pChangeFlags = true;
      nTilesChanged++;
    }
    pChangeFlags++;
    ptr_old += nBytesPerTile;
    ptr_new += nBytesPerTile;
  }

  // Handle the rightmost pixels, if the width is not a multiple of 32.
  int nBytesLeft = (w % 32) * m_bytesPerPixel;
  if (nBytesLeft != 0) {
    if (memcmp(ptr_old, ptr_new, nBytesLeft)) {
      *pChangeFlags = true;
      nTilesChanged++;
    }
  }

  return nTilesChanged;
}

int PollingManager::checkColumn(int x, int y, int h, bool *pChangeFlags)
{
  getColumn(x, y, h);

  int nTilesChanged = 0;
  for (int nTile = 0; nTile < (h + 31) / 32; nTile++) {
    if (!*pChangeFlags) {
      int tile_h = (h - nTile * 32 >= 32) ? 32 : h - nTile * 32;
      for (int i = 0; i < tile_h; i++) {
        // FIXME: Do not compute these pointers in the inner cycle.
        char *ptr_old = m_image->locatePixel(x, y + nTile * 32 + i);
        char *ptr_new = m_columnImage->locatePixel(0, nTile * 32 + i);
        if (memcmp(ptr_old, ptr_new, m_bytesPerPixel)) {
          *pChangeFlags = true;
          nTilesChanged++;
          break;
        }
      }
    }
    pChangeFlags += m_widthTiles;
  }

  return nTilesChanged;
}

int PollingManager::sendChanges(VNCServer *server) const
{
  const bool *pChangeFlags = m_changeFlags;
  int nTilesChanged = 0;

  Rect rect;
  for (int y = 0; y < m_heightTiles; y++) {
    for (int x = 0; x < m_widthTiles; x++) {
      if (*pChangeFlags++) {
        // Count successive tiles marked as changed.
        int count = 1;
        while (x + count < m_widthTiles && *pChangeFlags++) {
          count++;
        }
        nTilesChanged += count;
        // Compute the coordinates and the size of this band.
        rect.setXYWH(x * 32, y * 32, count * 32, 32);
        if (rect.br.x > m_width)
          rect.br.x = m_width;
        if (rect.br.y > m_height)
          rect.br.y = m_height;
        // Add to the changed region maintained by the server.
        server->add_changed(rect);
        // Skip processed tiles.
        x += count;
      }
    }
  }
  return nTilesChanged;
}

void
PollingManager::checkNeighbors()
{
  int x, y;

  // Check neighboring pixels above and below changed tiles.
  // FIXME: Fast skip to the first changed tile (and to the last, too).
  // FIXME: Check the full-width line above the first changed tile?
  for (y = 0; y < m_heightTiles; y++) {
    bool doneAbove = false;
    bool doneBelow = false;
    for (x = 0; x < m_widthTiles; x++) {
      if (!doneAbove && y > 0 &&
          m_changeFlags[y * m_widthTiles + x] &&
          !m_changeFlags[(y - 1) * m_widthTiles + x]) {
        // FIXME: Check m_changeFlags[] to decrease height of the row.
        checkRow(x * 32, y * 32 - 1, m_width - x * 32);
        doneAbove = true;
      }
      if (!doneBelow && y < m_heightTiles - 1 &&
          m_changeFlags[y * m_widthTiles + x] &&
          !m_changeFlags[(y + 1) * m_widthTiles + x]) {
        // FIXME: Check m_changeFlags[] to decrease height of the row.
        checkRow(x * 32, (y + 1) * 32, m_width - x * 32);
        doneBelow = true;
      }
      if (doneBelow && doneAbove)
        break;
    }
  }

  // Check neighboring pixels at the right side of changed tiles.
  for (x = 0; x < m_widthTiles - 1; x++) {
    for (y = 0; y < m_heightTiles; y++) {
      if (m_changeFlags[y * m_widthTiles + x] &&
          !m_changeFlags[y * m_widthTiles + x + 1]) {
        // FIXME: Check m_changeFlags[] to decrease height of the column.
        checkColumn((x + 1) * 32, y * 32, m_height - y * 32,
                    &m_changeFlags[y * m_widthTiles + x + 1]);
        break;
      }
    }
  }

  // Check neighboring pixels at the left side of changed tiles.
  for (x = m_widthTiles - 1; x > 0; x--) {
    for (y = 0; y < m_heightTiles; y++) {
      if (m_changeFlags[y * m_widthTiles + x] &&
          !m_changeFlags[y * m_widthTiles + x - 1]) {
        // FIXME: Check m_changeFlags[] to decrease height of the column.
        checkColumn(x * 32 - 1, y * 32, m_height - y * 32,
                    &m_changeFlags[y * m_widthTiles + x - 1]);
        break;
      }
    }
  }
}

void
PollingManager::printChanges(const char *header) const
{
  fprintf(stderr, "%s:", header);

  const bool *pChangeFlags = m_changeFlags;

  for (int y = 0; y < m_heightTiles; y++) {
    for (int x = 0; x < m_widthTiles; x++) {
      if (*pChangeFlags++) {
        // Count successive tiles marked as changed.
        int count = 1;
        while (x + count < m_widthTiles && *pChangeFlags++) {
          count++;
        }
        // Print.
        fprintf(stderr, " (%d,%d)*%d", x, y, count);
        // Skip processed tiles.
        x += count;
      }
    }
  }

  fprintf(stderr, "\n");
}

