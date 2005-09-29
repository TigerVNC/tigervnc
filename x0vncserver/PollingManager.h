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

using namespace rfb;

class PollingManager {

public:

  PollingManager(Display *dpy, Image *image, ImageFactory *factory);

  void setVNCServer(VNCServer* s);
  void pollDebug();
  void poll();

protected:

  Display* m_dpy;
  VNCServer* m_server;
  Image* m_image;
  Image* m_rowImage;
  Image* m_tileImage;
  unsigned int m_pollingStep;
  static const int m_pollingOrder[];

};

#endif // __POLLINGMANAGER_H__
