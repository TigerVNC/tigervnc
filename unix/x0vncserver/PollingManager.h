/* Copyright (C) 2004-2007 Constantin Kaplinsky.  All Rights Reserved.
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

using namespace rfb;

class PollingManager {

public:

  PollingManager(Display *dpy, Image *image, ImageFactory *factory,
                 int offsetLeft = 0, int offsetTop = 0);
  virtual ~PollingManager();

  void setVNCServer(VNCServer *s);

  // Currently, these functions do nothing. In past versions, we used
  // to poll area around the pointer if its position has been changed
  // recently. But that rather decreased overal polling performance,
  // so we don't do that any more. However, pointer position may be a
  // useful hint and might be used in future code, so we do not remove
  // these functions, just in case.
  void setPointerPos(const Point &pos) {}
  void unsetPointerPos() {}

  void poll();

protected:

  // Screen polling. Returns true if some changes were detected.
  bool pollScreen();

  Display *m_dpy;
  VNCServer *m_server;

  Image *m_image;
  int m_offsetLeft;
  int m_offsetTop;
  int m_width;
  int m_height;

private:

  inline void getScreen() {
    m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);
  }

  inline void getScreenRect(const Rect& r) {
    m_image->get(DefaultRootWindow(m_dpy),
                 m_offsetLeft + r.tl.x, m_offsetTop + r.tl.y,
                 r.width(), r.height(), r.tl.x, r.tl.y);
  }

  inline void getFullRow(int y) {
    m_rowImage->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop + y);
  }

  inline void getRow(int x, int y, int w) {
    m_rowImage->get(DefaultRootWindow(m_dpy),
                    m_offsetLeft + x, m_offsetTop + y, w, 1);
  }

  inline void getColumn(int x, int y, int h) {
    m_columnImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + x, m_offsetTop + y, 1, h);
  }

  int checkRow(int x, int y, int w, bool *pChangeFlags);
  int checkColumn(int x, int y, int h, bool *pChangeFlags);
  int sendChanges();
  void handleVideo();
  void flagVideoArea(bool value);

  // Check neighboring tiles and update m_changeFlags[].
  void checkNeighbors();

  // DEBUG: Print the list of changed tiles.
  void printChanges(const char *header) const;

  // Video detection functions.
  void detectVideo();
  void getVideoAreaRect(Rect *result);
  void constructLengthMatrices(int **pmx_h, int **pmx_v);
  void destroyLengthMatrices(int *mx_h, int *mx_v);
  void findMaxLocalRect(Rect *r, int *mx_h, int *mx_v);

  // Additional images used in polling algorithms.
  Image *m_rowImage;            // one row of the framebuffer
  Image *m_columnImage;         // one column of the framebuffer

  int m_widthTiles;		// shortcut for ((m_width + 31) / 32)
  int m_heightTiles;		// shortcut for ((m_height + 31) / 32)
  int m_numTiles;		// shortcut for (m_widthTiles * m_heightTiles)

  // m_changeFlags[] array will hold boolean values corresponding to
  // each 32x32 tile. If a value is true, then we've detected a change
  // in that tile.
  bool *m_changeFlags;

  char *m_rateMatrix;
  char *m_videoFlags;
  Rect m_videoRect;
  int m_numVideoPasses;

  unsigned int m_pollingStep;
  static const int m_pollingOrder[];

  static IntParameter m_videoPriority;

#ifdef DEBUG
private:

  void debugBeforePoll();
  void debugAfterPoll();

  TimeMillis m_timeSaved;
#endif

};

#endif // __POLLINGMANAGER_H__
