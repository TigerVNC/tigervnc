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
// Geometry.cxx
//

#include <stdio.h>

#include <rfb/Rect.h>
#include <rfb/LogWriter.h>
#include <x0vncserver/Geometry.h>

static LogWriter vlog("Geometry");

StringParameter Geometry::m_geometryParam("Geometry",
  "Screen area shown to VNC clients. "
  "Format is <width>x<height>+<offset_x>+<offset_y>, "
  "more information in man X, section GEOMETRY SPECIFICATIONS. "
  "If the argument is empty, full screen is shown to VNC clients.",
  "");

Geometry::Geometry(int fullWidth, int fullHeight)
  : m_width(fullWidth), m_height(fullHeight),
    m_offsetLeft(0), m_offsetTop(0)
{
  const char *param = m_geometryParam.getData();
  if (strlen(param) != 0) {
    int w, h;
    int x = 0, y = 0;
    char sign_x[2] = "+";
    char sign_y[2] = "+";
    int n = sscanf(param, "%dx%d%1[+-]%d%1[+-]%d",
                   &w, &h, sign_x, &x, sign_y, &y);
    if ((n == 2 || n == 6) && w > 0 && h > 0 && x >= 0 && y >= 0) {
      if (sign_x[0] == '-')
        x = fullWidth - w - x;
      if (sign_y[0] == '-')
        y = fullHeight - h - y;
      Rect fullRect(0, 0, fullWidth, fullHeight);
      Rect partRect(x, y, x + w, y + h);
      Rect r = partRect.intersect(fullRect);
      if (r.area() > 0) {
        m_width = r.width();
        m_height = r.height();
        m_offsetLeft = r.tl.x;
        m_offsetTop = r.tl.y;
      } else {
        vlog.error("Requested area is out of the desktop boundaries");
      }
    } else {
      vlog.error("Wrong argument format");
    }
  }
  vlog.info("Desktop geometry is %dx%d+%d+%d",
            m_width, m_height, m_offsetLeft, m_offsetTop);
}

