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

  void setPointerPos(const Point &pos);
  void unsetPointerPos();

  void poll();

  // Configurable parameters.
  static BoolParameter pollPointer;
  static IntParameter pollingType;

protected:

  //
  // Implementations of different polling algorithms.
  // Return value of true reports that some changes were detected.
  //
  bool poll_New();
  bool poll_DetectVideo();
  bool poll_SkipCycles();
  bool poll_Traditional();
  bool poll_Dumb();

  // Separate polling for the area around current pointer position.
  void computePointerArea(Rect *r);
  bool pollPointerArea();

  Display *m_dpy;
  VNCServer *m_server;

  Image *m_image;
  int m_offsetLeft;
  int m_offsetTop;
  int m_width;
  int m_height;
  int m_widthTiles;
  int m_heightTiles;

  // Tracking pointer position for polling improvements.
  bool m_pointerPosKnown;
  Point m_pointerPos;
  time_t m_pointerPosTime;

private:

  inline void getScreen() {
    m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);
  }

  inline void getScreenRect(const Rect& r) {
    m_image->get(DefaultRootWindow(m_dpy),
                 m_offsetLeft + r.tl.x, m_offsetTop + r.tl.y,
                 r.width(), r.height(), r.tl.x, r.tl.y);
  }

  inline void getRow(int y) {
    m_rowImage->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop + y);
  }

  inline void getTile32(int tx, int ty, int w, int h) {
    if (w == 32 && h == 32) {
      // This version of get() may be better optimized.
      m_tileImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + tx * 32, m_offsetTop + ty * 32);
    } else {
      // Generic version of get() for arbitrary width and height.
      m_tileImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + tx * 32, m_offsetTop + ty * 32, w, h);
    }
  }

  inline void getArea128(int x, int y, int w, int h) {
    if (w == 128 && h == 128) {
      // This version of get() may be better optimized.
      m_areaImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + x, m_offsetTop + y);
    } else {
      // Generic version of get() for arbitrary width and height.
      m_areaImage->get(DefaultRootWindow(m_dpy),
                       m_offsetLeft + x, m_offsetTop + y, w, h);
    }
  }

  void adjustVideoArea();

  void getVideoAreaRect(Rect *result);

  // Functions called by getVideoAreaRect().
  void constructLengthMatrices(int **pmx_h, int **pmx_v);
  void destroyLengthMatrices(int *mx_h, int *mx_v);
  void findMaxLocalRect(Rect *r, int *mx_h, int *mx_v);

  // Additional images used in polling algorithms.
  Image *m_rowImage;            // One row of the framebuffer
  Image *m_tileImage;           // One tile (32x32 or less)
  Image *m_areaImage;           // Area around the pointer (up to 128x128)

  char *m_statusMatrix;

  char *m_rateMatrix;
  char *m_videoFlags;
  char *m_changedFlags;

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
