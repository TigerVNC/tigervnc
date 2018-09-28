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

class Geometry
{
public:
  Geometry(int fullWidth, int fullHeight);
  void recalc(int fullWidth, int fullHeight);

  // Return coordinates and dimensions that specify a rectangular part
  // of the desktop that would be shown to RFB clients. This
  // information is extracted in the constructor from the "Geometry"
  // parameter.
  int width() const { return m_rect.width(); }
  int height() const { return m_rect.height(); }
  int offsetLeft() const { return m_rect.tl.x; }
  int offsetTop() const { return m_rect.tl.y; }

  // Return the same information as a Rect structure.
  const rfb::Rect& getRect() const { return m_rect; }

protected:
  // Parse a string, extract size and coordinates,
  // and return that rectangle clipped to m_rect.
  rfb::Rect parseString(const char *arg) const;

  static rfb::StringParameter m_geometryParam;

  int m_fullWidth;
  int m_fullHeight;
  rfb::Rect m_rect;
};

#endif // __GEOMETRY_H__

