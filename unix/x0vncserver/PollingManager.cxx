/* Copyright (C) 2004-2005 Constantin Kaplinsky.  All Rights Reserved.
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

// FIXME: Don't compare pixels already marked as changed.
// FIXME: Use Image::copyPixels() instead of Image::updateRect()?
//        In that case, note the fact that arguments are not checked.

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServer.h>
#include <rfb/Configuration.h>
#include <rfb/ServerCore.h>

#include <x0vncserver/PollingManager.h>

static LogWriter vlog("PollingMgr");

BoolParameter PollingManager::pollPointer
("PollPointer",
 "DEBUG: Poll area under the pointer with higher priority",
 false);

IntParameter PollingManager::pollingType
("PollingType",
 "DEBUG: Select particular polling algorithm (0..4)",
 4);

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

PollingManager::PollingManager(Display *dpy, Image *image,
                               ImageFactory *factory,
                               int offsetLeft, int offsetTop)
  : m_dpy(dpy), m_server(0), m_image(image),
    m_offsetLeft(offsetLeft), m_offsetTop(offsetTop),
    m_pointerPosKnown(false), m_pollingStep(0)
{
  // Save width and height of the screen (and the image).
  m_width = m_image->xim->width;
  m_height = m_image->xim->height;

  // Compute width and height in 32x32 tiles.
  m_widthTiles = (m_width + 31) / 32;
  m_heightTiles = (m_height + 31) / 32;

  // Get initial screen image.
  m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);

  // Create additional images used in polling algorithms, warn if
  // underlying class names are different from the class name of the
  // primary image.
  m_rowImage = factory->newImage(m_dpy, m_width, 1);
  m_tileImage = factory->newImage(m_dpy, 32, 32);
  m_areaImage = factory->newImage(m_dpy, 128, 128);
  if (strcmp(m_image->className(), m_rowImage->className()) != 0 ||
      strcmp(m_image->className(), m_tileImage->className()) != 0 ||
      strcmp(m_image->className(), m_areaImage->className()) != 0) {
    vlog.error("Image types do not match (%s, %s, %s)",
               m_rowImage->className(),
               m_tileImage->className(),
               m_areaImage->className());
  }

  // FIXME: Extend the comment.
  // Create a matrix with one byte per each 32x32 tile. It will be
  // used to limit the rate of updates on continuously-changed screen
  // areas (like video).
  int numTiles = m_widthTiles * m_heightTiles;
  m_statusMatrix = new char[numTiles];
  memset(m_statusMatrix, 0, numTiles);

  // FIXME: Extend the comment.
  // Create a matrix with one byte per each 32x32 tile. It will be
  // used to limit the rate of updates on continuously-changed screen
  // areas (like video).
  m_rateMatrix = new char[numTiles];
  m_videoFlags = new char[numTiles];
  m_changedFlags = new char[numTiles];
  memset(m_rateMatrix, 0, numTiles);
  memset(m_videoFlags, 0, numTiles);
  memset(m_changedFlags, 0, numTiles);
}

PollingManager::~PollingManager()
{
  delete[] m_changedFlags;
  delete[] m_videoFlags;
  delete[] m_rateMatrix;

  delete[] m_statusMatrix;

  delete m_areaImage;
  delete m_tileImage;
  delete m_rowImage;
}

//
// Register VNCServer object.
//

void PollingManager::setVNCServer(VNCServer *s)
{
  m_server = s;
}

//
// Update current pointer position which may be used as a hint for
// polling algorithms.
//

void PollingManager::setPointerPos(const Point &pos)
{
  m_pointerPosTime = time(NULL);
  m_pointerPos = pos;
  m_pointerPosKnown = true;
}

//
// Indicate that current pointer position is unknown.
//

void PollingManager::unsetPointerPos()
{
  m_pointerPosKnown = false;
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

void PollingManager::poll()
{
#ifdef DEBUG
  debugBeforePoll();
#endif

  // First step: full-screen polling.

  bool changes1 = false;

  switch((int)pollingType) {
  case 0:
    changes1 = poll_Dumb();
    break;
  case 1:
    changes1 = poll_Traditional();
    break;
  case 2:
    changes1 = poll_SkipCycles();
    break;
  case 3:
    changes1 = poll_DetectVideo();
    break;
//case 4:
  default:
    changes1 = poll_New();
    break;
  }

  // Second step: optional thorough polling of the area around the pointer.
  // We do that only if the pointer position is known and was set recently.

  bool changes2 = false;
  if (pollPointer) {
    if (m_pointerPosKnown && time(NULL) - m_pointerPosTime >= 5) {
      unsetPointerPos();
    }
    if (m_pointerPosKnown) {
      changes2 = pollPointerArea();
    }
  }

  // Update if needed.

  if (changes1 || changes2)
    m_server->tryUpdate();

#ifdef DEBUG
  debugAfterPoll();
#endif
}

bool PollingManager::poll_New()
{
  if (!m_server)
    return false;

  // mxChanged[] array will hold boolean values corresponding to each
  // 32x32 tile. If a value is true, then we've detected a change in
  // that tile. Initially, we fill in the array with zero values.
  bool *mxChanged = new bool[m_widthTiles * m_heightTiles];
  memset(mxChanged, 0, m_widthTiles * m_heightTiles);

  // First pass over the framebuffer. Here we scan 1/32 part of the
  // framebuffer -- that is, one line in each (32 * m_width) stripe.
  // We compare the pixels of that line with previous framebuffer
  // contents and raise corresponding member values of mxChanged[].
  int scanOffset = m_pollingOrder[m_pollingStep++ % 32];
  bool *pmxChanged = mxChanged;
  int nTilesChanged = 0;
  for (int y = scanOffset; y < m_height; y += 32) {
    nTilesChanged += checkRow(0, y, m_width, pmxChanged);
    pmxChanged += m_widthTiles;
  }

  // Inform the server about the changes.
  if (nTilesChanged)
    sendChanges(mxChanged);

  // Cleanup.
  delete[] mxChanged;

  return (nTilesChanged != 0);
}

int PollingManager::checkRow(int x, int y, int w, bool *pmxChanged)
{
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int bytesPerLine = m_image->xim->bytes_per_line;

  getRow(x, y, w);

  char *ptr_old = m_image->xim->data + y * bytesPerLine + x * bytesPerPixel;
  char *ptr_new = m_rowImage->xim->data;

  int nTilesChanged = 0;
  for (int i = 0; i < (w + 31) / 32; i++) {
    int tile_w = (w - i * 32 >= 32) ? 32 : w - i * 32;
    int nBytes = tile_w * bytesPerPixel;
    if (memcmp(ptr_old, ptr_new, nBytes)) {
      *pmxChanged = true;
      nTilesChanged++;
    }
    pmxChanged++;
    ptr_old += nBytes;
    ptr_new += nBytes;
  }

  return nTilesChanged;
}

void PollingManager::sendChanges(bool *pmxChanged)
{
  Rect rect;
  for (int y = 0; y < m_heightTiles; y++) {
    for (int x = 0; x < m_widthTiles; x++) {
      if (*pmxChanged++) {
        // Count successive tiles marked as changed.
        int count = 1;
        while (x + count < m_widthTiles && *pmxChanged++) {
          count++;
        }
        // Compute the coordinates and the size of this band.
        rect.setXYWH(x * 32, y * 32, count * 32, 32);
        if (rect.br.x > m_width)
          rect.br.x = m_width;
        if (rect.br.y > m_height)
          rect.br.y = m_height;
        // Add to the changed region maintained by the server.
        getScreenRect(rect);
        m_server->add_changed(rect);
        // Skip processed tiles.
        x += count;
      }
    }
  }
}

bool PollingManager::poll_DetectVideo()
{
  if (!m_server)
    return false;

  const int GRAND_STEP_DIVISOR = 8;
  const int VIDEO_THRESHOLD_0 = 3;
  const int VIDEO_THRESHOLD_1 = 5;

  bool grandStep = (m_pollingStep % GRAND_STEP_DIVISOR == 0);

  // FIXME: Save shortcuts in member variables?
  int scanLine = m_pollingOrder[m_pollingStep++ % 32];
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int bytesPerLine = m_image->xim->bytes_per_line;

  Rect rect;
  int nTilesChanged = 0;
  int idx = 0;

  for (int y = 0; y * 32 < m_height; y++) {
    int tile_h = (m_height - y * 32 >= 32) ? 32 : m_height - y * 32;
    if (scanLine >= tile_h)
      break;
    int scan_y = y * 32 + scanLine;
    getFullRow(scan_y);
    char *ptr_old = m_image->xim->data + scan_y * bytesPerLine;
    char *ptr_new = m_rowImage->xim->data;
    for (int x = 0; x * 32 < m_width; x++) {
      int tile_w = (m_width - x * 32 >= 32) ? 32 : m_width - x * 32;
      int nBytes = tile_w * bytesPerPixel;

      char wasChanged = (memcmp(ptr_old, ptr_new, nBytes) != 0);
      m_rateMatrix[idx] += wasChanged;

      if (grandStep) {
        if (m_rateMatrix[idx] <= VIDEO_THRESHOLD_0) {
          m_videoFlags[idx] = 0;
        } else if (m_rateMatrix[idx] >= VIDEO_THRESHOLD_1) {
          m_videoFlags[idx] = 1;
        }
        m_rateMatrix[idx] = 0;
      }

      m_changedFlags[idx] |= wasChanged;
      if ( m_changedFlags[idx] && (!m_videoFlags[idx] || grandStep) ) {
        getTile32(x, y, tile_w, tile_h);
        m_image->updateRect(m_tileImage, x * 32, y * 32);
        rect.setXYWH(x * 32, y * 32, tile_w, tile_h);
        m_server->add_changed(rect);
        nTilesChanged++;
        m_changedFlags[idx] = 0;
      }

      ptr_old += nBytes;
      ptr_new += nBytes;
      idx++;
    }
  }

  if (grandStep)
    adjustVideoArea();

  bool videoDetected = false;
  Rect r;
  getVideoAreaRect(&r);
  m_server->set_video_area(r);
  videoDetected = !r.is_empty();
  if (videoDetected) {
    m_image->get(DefaultRootWindow(m_dpy),
                 m_offsetLeft + r.tl.x, m_offsetTop + r.tl.y,
                 r.width(), r.height(), r.tl.x, r.tl.y);
  }

#ifdef DEBUG
  if (nTilesChanged != 0) {
    fprintf(stderr, "#%d# ", nTilesChanged);
  }
#endif

  return (nTilesChanged != 0 || videoDetected);
}

bool PollingManager::poll_SkipCycles()
{
  if (!m_server)
    return false;

  enum {
    NOT_CHANGED, CHANGED_ONCE, CHANGED_AGAIN
  };

  bool grandStep = (m_pollingStep % 8 == 0);

  int nTilesChanged = 0;
  int scanLine = m_pollingOrder[m_pollingStep++ % 32];
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int bytesPerLine = m_image->xim->bytes_per_line;
  char *pstatus = m_statusMatrix;
  bool wasChanged;
  Rect rect;

  for (int y = 0; y * 32 < m_height; y++) {
    int tile_h = (m_height - y * 32 >= 32) ? 32 : m_height - y * 32;
    if (scanLine >= tile_h)
      scanLine %= tile_h;
    int scan_y = y * 32 + scanLine;
    getFullRow(scan_y);
    char *ptr_old = m_image->xim->data + scan_y * bytesPerLine;
    char *ptr_new = m_rowImage->xim->data;
    for (int x = 0; x * 32 < m_width; x++) {
      int tile_w = (m_width - x * 32 >= 32) ? 32 : m_width - x * 32;
      int nBytes = tile_w * bytesPerPixel;

      if (grandStep || *pstatus != CHANGED_AGAIN) {
        wasChanged = (*pstatus == CHANGED_AGAIN) ?
          true : (memcmp(ptr_old, ptr_new, nBytes) != 0);
        if (wasChanged) {
          if (grandStep || *pstatus == NOT_CHANGED) {
            getTile32(x, y, tile_w, tile_h);
            m_image->updateRect(m_tileImage, x * 32, y * 32);
            rect.setXYWH(x * 32, y * 32, tile_w, tile_h);
            m_server->add_changed(rect);
            nTilesChanged++;
            *pstatus = CHANGED_ONCE;
          } else {
            *pstatus = CHANGED_AGAIN;
          }
        } else if (grandStep) {
          *pstatus = NOT_CHANGED;
        }
      }

      ptr_old += nBytes;
      ptr_new += nBytes;
      pstatus++;
    }
  }

  return (nTilesChanged != 0);
}

bool PollingManager::poll_Traditional()
{
  if (!m_server)
    return false;

  int nTilesChanged = 0;
  int scanLine = m_pollingOrder[m_pollingStep++ % 32];
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int bytesPerLine = m_image->xim->bytes_per_line;
  Rect rect;

  for (int y = 0; y * 32 < m_height; y++) {
    int tile_h = (m_height - y * 32 >= 32) ? 32 : m_height - y * 32;
    if (scanLine >= tile_h)
      break;
    int scan_y = y * 32 + scanLine;
    getFullRow(scan_y);
    char *ptr_old = m_image->xim->data + scan_y * bytesPerLine;
    char *ptr_new = m_rowImage->xim->data;
    for (int x = 0; x * 32 < m_width; x++) {
      int tile_w = (m_width - x * 32 >= 32) ? 32 : m_width - x * 32;
      int nBytes = tile_w * bytesPerPixel;
      if (memcmp(ptr_old, ptr_new, nBytes)) {
        getTile32(x, y, tile_w, tile_h);
        m_image->updateRect(m_tileImage, x * 32, y * 32);
        rect.setXYWH(x * 32, y * 32, tile_w, tile_h);
        m_server->add_changed(rect);
        nTilesChanged++;
      }
      ptr_old += nBytes;
      ptr_new += nBytes;
    }
  }

  return (nTilesChanged != 0);
}

//
// Simplest polling method, from the original x0vncserver of VNC4.
//

bool PollingManager::poll_Dumb()
{
  if (!m_server)
    return false;

  getScreen();
  Rect rect(0, 0, m_width, m_height);
  m_server->add_changed(rect);

  // Report that some changes have been detected.
  return true;
}

//
// Compute coordinates of the rectangle around the pointer.
//
// ASSUMES: (m_pointerPosKnown != false)
//

void PollingManager::computePointerArea(Rect *r)
{
  int x = m_pointerPos.x - 64;
  int y = m_pointerPos.y - 64;
  int w = 128;
  int h = 128;
  if (x < 0) {
    w += x; x = 0;
  }
  if (x + w > m_width) {
    w = m_width - x;
  }
  if (y < 0) {
    h += y; y = 0;
  }
  if (y + h > m_height) {
    h = m_height - y;
  }

  r->setXYWH(x, y, w, h);
}

//
// Poll the area under current pointer position. Each pixel of the
// area should be compared. Using such polling option gives higher
// priority to screen area under the pointer.
//
// ASSUMES: (m_server != NULL && m_pointerPosKnown != false)
//

bool PollingManager::pollPointerArea()
{
  Rect r;
  computePointerArea(&r);

  // Shortcuts for coordinates.
  int x = r.tl.x, y = r.tl.y;
  int w = r.width(), h = r.height();

  // Get new pixels.
  getArea128(x, y, w, h);

  // Now, try to minimize the rectangle by cutting out unchanged
  // borders (at top and bottom).
  //
  // FIXME: Perhaps we should work on 32x32 tiles (properly aligned)
  //        to produce a region instead of a rectangle. If there would
  //        be just one universal polling algorithm, it could be
  //        better to integrate pointer area polling into that
  //        algorithm, instead of a separate pollPointerArea()
  //        function.

  // Shortcuts.
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int oldBytesPerLine = m_image->xim->bytes_per_line;
  int newBytesPerLine = m_areaImage->xim->bytes_per_line;
  char *oldPtr = m_image->xim->data + y * oldBytesPerLine + x * bytesPerPixel;
  char *newPtr = m_areaImage->xim->data;

  // Check and cut out unchanged rows at the top.
  int ty;
  for (ty = 0; ty < h; ty++) {
    if (memcmp(oldPtr, newPtr, w * bytesPerPixel) != 0)
      break;
    oldPtr += oldBytesPerLine;
    newPtr += newBytesPerLine;
  }
  if (ty == h) {
    return false;               // no changes at all
  }
  y += ty; h -= ty;

  // Check and cut out unchanged rows at the bottom.
  oldPtr = m_image->xim->data + (y+h-1) * oldBytesPerLine + x * bytesPerPixel;
  newPtr = m_areaImage->xim->data + (ty+h-1) * newBytesPerLine;
  int by;
  for (by = 0; by < h - 1; by++) {
    if (memcmp(oldPtr, newPtr, w * bytesPerPixel) != 0)
      break;
    oldPtr -= oldBytesPerLine;
    newPtr -= newBytesPerLine;
  }
  h -= by;

  // Copy pixels.
  m_image->updateRect(m_areaImage, x, y, 0, ty, w, h);

  // Report updates to the server.
  Rect rect(x, y, x+w, y+h);
  m_server->add_changed(rect);
  return true;
}

//
// Make video area pattern more regular.
//
// FIXME: Replace the above with a normal comment.
// FIXME: Is the function efficient enough?
//

void PollingManager::adjustVideoArea()
{
  char newFlags[m_widthTiles * m_heightTiles];
  char *ptr = newFlags;
  int x, y;

  // DEBUG:
  // int nVideoTiles = 0;

  for (y = 0; y < m_heightTiles; y++) {
    for (x = 0; x < m_widthTiles; x++) {

      // DEBUG:
      // nVideoTiles += m_videoFlags[y * m_widthTiles + x];

      int weightedSum = 0, n;
      if (y > 0 && x > 0) {
        n = (m_videoFlags[ y    * m_widthTiles + (x-1)] +
             m_videoFlags[(y-1) * m_widthTiles + (x-1)] +
             m_videoFlags[(y-1) * m_widthTiles +  x   ]);
        if (n == 3) {
          *ptr++ = 1;
          continue;
        }
        weightedSum += n;
      }
      if (y > 0 && x < m_widthTiles - 1) {
        n = (m_videoFlags[ y    * m_widthTiles + (x+1)] +
             m_videoFlags[(y-1) * m_widthTiles + (x+1)] +
             m_videoFlags[(y-1) * m_widthTiles +  x   ]);
        if (n == 3) {
          *ptr++ = 1;
          continue;
        }
        weightedSum += n;
      }
      if (y < m_heightTiles - 1 && x > 0) {
        n = (m_videoFlags[ y    * m_widthTiles + (x-1)] +
             m_videoFlags[(y+1) * m_widthTiles + (x-1)] +
             m_videoFlags[(y+1) * m_widthTiles +  x   ]);
        if (n == 3) {
          *ptr++ = 1;
          continue;
        }
        weightedSum += n;
      }
      if (y < m_heightTiles - 1 && x < m_widthTiles - 1) {
        n = (m_videoFlags[ y    * m_widthTiles + (x+1)] +
             m_videoFlags[(y+1) * m_widthTiles + (x+1)] +
             m_videoFlags[(y+1) * m_widthTiles +  x   ]);
        if (n == 3) {
          *ptr++ = 1;
          continue;
        }
        weightedSum += n;
      }
      *ptr++ = (weightedSum <= 3) ? 0 : m_videoFlags[y * m_widthTiles + x];
    }
  }

  /*
  /// DEBUG: ------------------------------------------------------
  if (nVideoTiles) {
    for (y = 0; y < m_heightTiles; y++) {
      for (x = 0; x < m_widthTiles; x++) {
        printf("%c", m_videoFlags[y * m_widthTiles + x] ? '@' : ':');
      }
      printf("        ");
      for (x = 0; x < m_widthTiles; x++) {
        printf("%c", newFlags[y * m_widthTiles + x] ? '@' : ':');
      }
      printf("\n");
    }
    printf("\n");
  }
  /// -------------------------------------------------------------
  */

  memcpy(m_videoFlags, newFlags, m_widthTiles * m_heightTiles);
}

void
PollingManager::getVideoAreaRect(Rect *result)
{
  int *mx_hlen, *mx_vlen;
  constructLengthMatrices(&mx_hlen, &mx_vlen);

  int full_h = m_heightTiles;
  int full_w = m_widthTiles;
  int x, y;
  Rect max_rect(0, 0, 0, 0);
  Rect local_rect;

  for (y = 0; y < full_h; y++) {
    for (x = 0; x < full_w; x++) {
      int max_w = mx_hlen[y * full_w + x];
      int max_h = mx_vlen[y * full_w + x];
      if (max_w > 2 && max_h > 1 && max_h * max_w > (int)max_rect.area()) {
        local_rect.tl.x = x;
        local_rect.tl.y = y;
        findMaxLocalRect(&local_rect, mx_hlen, mx_vlen);
        if (local_rect.area() > max_rect.area()) {
          max_rect = local_rect;
        }
      }
    }
  }

  destroyLengthMatrices(mx_hlen, mx_vlen);

  max_rect.tl.x *= 32;
  max_rect.tl.y *= 32;
  max_rect.br.x *= 32;
  max_rect.br.y *= 32;
  if (max_rect.br.x > m_width)
    max_rect.br.x = m_width;
  if (max_rect.br.y > m_height)
    max_rect.br.y = m_height;
  *result = max_rect;

  if (!result->is_empty()) {
    fprintf(stderr, "Video rect %dx%d\tat(%d,%d)\n",
            result->width(), result->height(), result->tl.x, result->tl.y);
  }
}

void
PollingManager::constructLengthMatrices(int **pmx_h, int **pmx_v)
{
  // Handy shortcuts.
  int h = m_heightTiles;
  int w = m_widthTiles;

  // Allocate memory.
  int *mx_h = new int[h * w];
  memset(mx_h, 0, h * w * sizeof(int));
  int *mx_v = new int[h * w];
  memset(mx_v, 0, h * w * sizeof(int));

  int x, y, len, i;

  // Fill in horizontal length matrix.
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      len = 0;
      while (x + len < w && m_videoFlags[y * w + x + len]) {
        len++;
      }
      for (i = 0; i < len; i++) {
        mx_h[y * w + x + i] = len - i;
      }
      x += len;
    }
  }

  // Fill in vertical length matrix.
  for (x = 0; x < w; x++) {
    for (y = 0; y < h; y++) {
      len = 0;
      while (y + len < h && m_videoFlags[(y + len) * w + x]) {
        len++;
      }
      for (i = 0; i < len; i++) {
        mx_v[(y + i) * w + x] = len - i;
      }
      y += len;
    }
  }

  *pmx_h = mx_h;
  *pmx_v = mx_v;
}

void
PollingManager::destroyLengthMatrices(int *mx_h, int *mx_v)
{
  delete[] mx_h;
  delete[] mx_v;
}

// NOTE: This function assumes that current tile has non-zero in mx_h[],
//       otherwise we get division by zero.
void
PollingManager::findMaxLocalRect(Rect *r, int mx_h[], int mx_v[])
{
  int idx = r->tl.y * m_widthTiles + r->tl.x;

  // NOTE: Rectangle's maximum width and height are 25 and 18
  //       (in tiles, where each tile is usually 32x32 pixels).
  int max_w = mx_h[idx];
  if (max_w > 25)
    max_w = 25;
  int cur_h = 18;

  int best_w = max_w;
  int best_area = 1 * best_w;

  for (int i = 0; i < max_w; i++) {
    int h = mx_v[idx + i];
    if (h < cur_h) {
      cur_h = h;
      if (cur_h * max_w <= best_area)
        break;
    }
    if (cur_h * (i + 1) > best_area) {
      best_w = i + 1;
      best_area = cur_h * best_w;
    }
  }

  r->br.x = r->tl.x + best_w;
  r->br.y = r->tl.y + best_area / best_w;
}

