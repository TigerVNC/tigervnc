/* Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
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

#include <rfb/Configuration.h>

using namespace rfb;

class Geometry
{
public:
  Geometry(int fullWidth, int fullHeight);

  int width() const { return m_width; }
  int height() const { return m_height; }
  int offsetLeft() const { return m_offsetLeft; }
  int offsetTop() const { return m_offsetTop; }

protected:
  static StringParameter m_geometryParam;

  int m_width;
  int m_height;
  int m_offsetLeft;
  int m_offsetTop;
};

#endif // __GEOMETRY_H__

