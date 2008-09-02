/* Copyright (C) 2006-2008 Constantin Kaplinsky.  All Rights Reserved.
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
// Geometry.h
//

#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <rfb/Rect.h>
#include <rfb/Configuration.h>

using namespace rfb;

class Geometry
{
public:
  Geometry(int fullWidth, int fullHeight);

  // Return coordinates and dimensions that specify a rectangular part
  // of the desktop that would be shown to RFB clients. This
  // information is extracted in the constructor from the "Geometry"
  // parameter.
  int width() const { return m_rect.width(); }
  int height() const { return m_rect.height(); }
  int offsetLeft() const { return m_rect.tl.x; }
  int offsetTop() const { return m_rect.tl.y; }

  // Return the same information as a Rect structure.
  const Rect& getRect() const { return m_rect; }

  // Return coordinates of the video rectangle if one was set with the
  // "VideoArea" parameter. The coordinates are relative to the whole
  // rectangle as returned by getRect(). In other words, the
  // coordinate (0, 0) corresponds to the top left corner of the
  // rectangle shown to RFB clients.
  const Rect& getVideoRect() const { return m_videoRect; }

protected:
  // Parse a string, extract size and coordinates,
  // and return that rectangle clipped to m_rect.
  Rect parseString(const char *arg) const;

  static StringParameter m_geometryParam;
  static StringParameter m_videoAreaParam;

  int m_fullWidth;
  int m_fullHeight;
  Rect m_rect;
  Rect m_videoRect;
};

#endif // __GEOMETRY_H__

