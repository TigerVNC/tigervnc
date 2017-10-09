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
// PollingManager.h
//

#ifndef __POLLINGMANAGER_H__
#define __POLLINGMANAGER_H__

#include <X11/Xlib.h>
#include <rfb/VNCServer.h>

#include <x0vncserver/Image.h>

#ifdef DEBUG
#include <x0vncserver/TimeMillis.h>
#endif

class PollingManager {

public:

  PollingManager(Display *dpy, const Image *image, ImageFactory &factory,
                 int offsetLeft = 0, int offsetTop = 0);
  virtual ~PollingManager();

  void poll(rfb::VNCServer *server);

protected:

  // Screen polling. Returns true if some changes were detected.
  bool pollScreen(rfb::VNCServer *server);

  Display *m_dpy;

  const Image *m_image;
  const int m_bytesPerPixel;

  const int m_offsetLeft;
  const int m_offsetTop;
  const int m_width;
  const int m_height;

private:

  inline void getRow(int x, int y, int w) {
    if (w == m_width) {
      // Getting full row may be more efficient.
      m_rowImage->get(DefaultRootWindow(m_dpy),
                      m_offsetLeft, m_offsetTop + y);
    } else {
      m_rowImage->get(DefaultRootWindow(m_dpy),
                      m_offsetLeft + x, m_offsetTop + y, w, 1);
    }
  }

  inline void getColumn(int x, int y, int h) {
    m_columnImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + x, m_offsetTop + y, 1, h);
  }

  inline int getTileIndex(int x, int y) {
    int tile_x = x / 32;
    int tile_y = y / 32;
    return tile_y * m_widthTiles + tile_x;
  }

  int checkRow(int x, int y, int w);
  int checkColumn(int x, int y, int h, bool *pChangeFlags);
  int sendChanges(rfb::VNCServer *server) const;

  // Check neighboring tiles and update m_changeFlags[].
  void checkNeighbors();

  // DEBUG: Print the list of changed tiles.
  void printChanges(const char *header) const;

  // Additional images used in polling algorithms.
  Image *m_rowImage;            // one row of the framebuffer
  Image *m_columnImage;         // one column of the framebuffer

  const int m_widthTiles;       // shortcut for ((m_width + 31) / 32)
  const int m_heightTiles;      // shortcut for ((m_height + 31) / 32)
  const int m_numTiles;         // shortcut for (m_widthTiles * m_heightTiles)

  // m_changeFlags[] array will hold boolean values corresponding to
  // each 32x32 tile. If a value is true, then we've detected a change
  // in that tile.
  bool *m_changeFlags;

  unsigned int m_pollingStep;
  static const int m_pollingOrder[];

#ifdef DEBUG
private:

  void debugBeforePoll();
  void debugAfterPoll();

  TimeMillis m_timeSaved;
#endif

};

#endif // __POLLINGMANAGER_H__
