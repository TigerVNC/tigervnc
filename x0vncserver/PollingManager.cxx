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

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <rfb/VNCServer.h>
#include <rfb/Configuration.h>

#include <x0vncserver/Image.h>
#include <x0vncserver/PollingManager.h>

IntParameter pollingType("PollingType", "Polling algorithm to use (0..3)", 3);

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
                               ImageFactory *factory)
  : m_dpy(dpy), m_server(0), m_image(image), m_pollingStep(0)
{
  // Save width and height of the screen (and the image).
  m_width = m_image->xim->width;
  m_height = m_image->xim->height;

  // Compute width and height in 32x32 tiles.
  m_widthTiles = (m_width + 31) / 32;
  m_heightTiles = (m_height + 31) / 32;

  // Create two additional images used in the polling algorithm.
  // FIXME: verify that these images use the same pixel format as in m_image.
  m_rowImage = factory->newImage(m_dpy, m_width, 1);
  m_tileImage = factory->newImage(m_dpy, 32, 32);

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
}

//
// Register VNCServer object.
//

void PollingManager::setVNCServer(VNCServer *s)
{
  m_server = s;
}

//
// DEBUG: a version of poll() measuring time spent in the function.
//

void PollingManager::pollDebug()
{
  struct timeval timeSaved, timeNow;
  struct timezone tz;
  timeSaved.tv_sec = 0;
  timeSaved.tv_usec = 0;
  gettimeofday(&timeSaved, &tz);

  poll();

  gettimeofday(&timeNow, &tz);
  int diff = (int)((timeNow.tv_usec - timeSaved.tv_usec + 500) / 1000 +
                   (timeNow.tv_sec - timeSaved.tv_sec) * 1000);
  if (diff != 0)
    fprintf(stderr, "DEBUG: poll(): %4d ms\n", diff);
}

//
// Search for changed rectangles on the screen.
//

void PollingManager::poll()
{
  bool someChanges = false;

  switch((int)pollingType) {
  case 0:
    someChanges = poll_Dumb();
    break;
  case 1:
    someChanges = poll_Traditional();
    break;
  case 2:
    someChanges = poll_SkipCycles();
    break;
//case 3:
  default:
    someChanges = poll_DetectVideo();
    break;
  }

  if (someChanges)
    m_server->tryUpdate();
}

bool PollingManager::poll_DetectVideo()
{
  if (!m_server)
    return false;

  const int GRAND_STEP_DIVISOR = 8;
  const int VIDEO_THRESHOLD_0 = 3;
  const int VIDEO_THRESHOLD_1 = 5;

  bool grandStep = (m_pollingStep % GRAND_STEP_DIVISOR == 0);

  // FIXME: Save shortcuts in member variables.
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
    m_rowImage->get(DefaultRootWindow(m_dpy), 0, scan_y);
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
        if (tile_w == 32 && tile_h == 32) {
          m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32);
        } else {
          m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32,
                           tile_w, tile_h);
        }
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

  return (nTilesChanged != 0);
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
    m_rowImage->get(DefaultRootWindow(m_dpy), 0, scan_y);
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
            if (tile_w == 32 && tile_h == 32) {
              m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32);
            } else {
              m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32,
                               tile_w, tile_h);
            }
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
    m_rowImage->get(DefaultRootWindow(m_dpy), 0, scan_y);
    char *ptr_old = m_image->xim->data + scan_y * bytesPerLine;
    char *ptr_new = m_rowImage->xim->data;
    for (int x = 0; x * 32 < m_width; x++) {
      int tile_w = (m_width - x * 32 >= 32) ? 32 : m_width - x * 32;
      int nBytes = tile_w * bytesPerPixel;
      if (memcmp(ptr_old, ptr_new, nBytes)) {
        if (tile_w == 32 && tile_h == 32) {
          m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32);
        } else {
          m_tileImage->get(DefaultRootWindow(m_dpy), x * 32, y * 32,
                           tile_w, tile_h);
        }
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

  m_image->get(DefaultRootWindow(m_dpy));
  Rect rect(0, 0, m_width, m_height);
  m_server->add_changed(rect);

  // As we have no idea if any pixels were changed,
  // always report that some changes have been detected.
  return true;
}
