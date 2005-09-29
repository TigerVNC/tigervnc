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

#include <x0vncserver/Image.h>
#include <x0vncserver/PollingManager.h>

const int PollingManager::m_pollingOrder[32] = {
   0, 16,  8, 24,  4, 20, 12, 28,
  10, 26, 18,  2, 22,  6, 30, 14,
   1, 17,  9, 25,  7, 23, 15, 31,
  19,  3, 27, 11, 29, 13,  5, 21
};

//
// Constructor. Note that dpy and image should remain valid during
// object lifetime, while factory is used only in the constructor
// itself.
//

PollingManager::PollingManager(Display *dpy, Image *image,
                               ImageFactory *factory)
  : m_dpy(dpy), m_server(0), m_image(image), m_pollingStep(0)
{
  // Create two additional images used in the polling algorithm.
  // FIXME: verify that these images use the same pixel format as in m_image.
  m_rowImage = factory->newImage(m_dpy, m_image->xim->width, 1);
  m_tileImage = factory->newImage(m_dpy, 32, 32);
}

void PollingManager::setVNCServer(VNCServer* s)
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
  if (!m_server)
    return;

  int nTilesChanged = 0;
  int scanLine = m_pollingOrder[m_pollingStep++ % 32];
  int bytesPerPixel = m_image->xim->bits_per_pixel / 8;
  int bytesPerLine = m_image->xim->bytes_per_line;
  int w = m_image->xim->width, h = m_image->xim->height;
  Rect rect;

  for (int y = 0; y * 32 < h; y++) {
    int tile_h = (h - y * 32 >= 32) ? 32 : h - y * 32;
    if (scanLine >= tile_h)
      continue;
    int scan_y = y * 32 + scanLine;
    m_rowImage->get(DefaultRootWindow(m_dpy), 0, scan_y);
    char *ptr_old = m_image->xim->data + scan_y * bytesPerLine;
    char *ptr_new = m_rowImage->xim->data;
    for (int x = 0; x * 32 < w; x++) {
      int tile_w = (w - x * 32 >= 32) ? 32 : w - x * 32;
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

  if (nTilesChanged)
    m_server->tryUpdate();
}

