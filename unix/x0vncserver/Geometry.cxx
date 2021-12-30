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
// Geometry.cxx
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/LogWriter.h>
#include <x0vncserver/Geometry.h>

using namespace rfb;

static LogWriter vlog("Geometry");

StringParameter Geometry::m_geometryParam("Geometry",
  "Screen area shown to VNC clients. "
  "Format is <width>x<height>+<offset_x>+<offset_y>, "
  "more information in man X, section GEOMETRY SPECIFICATIONS. "
  "If the argument is empty, full screen is shown to VNC clients.",
  "");

Geometry::Geometry(int fullWidth, int fullHeight)
{
  recalc(fullWidth, fullHeight);
}

void Geometry::recalc(int fullWidth, int fullHeight)
{
  m_fullWidth = fullWidth;
  m_fullHeight = fullHeight;
  m_rect.setXYWH(0, 0, fullWidth, fullHeight);

  // Parse geometry specification and save the result in m_rect.
  const char *param = m_geometryParam.getData();
  bool geometrySpecified = (strlen(param) > 0);
  if (geometrySpecified) {
    m_rect = parseString(param);
  }
  delete[] param;               // don't forget to deallocate memory
                                // allocated by StringParameter::getData()
  if (m_rect.is_empty()) {
    vlog.info("Desktop geometry is invalid");
    return;                     // further processing does not make sense
  }

  // Everything went good so far.
  vlog.info("Desktop geometry is set to %dx%d+%d+%d",
            width(), height(), offsetLeft(), offsetTop());
}

Rect Geometry::parseString(const char *arg) const
{
  Rect result;                  // empty by default

  if (arg != NULL && strlen(arg) > 0) {
    int w, h;
    int x = 0, y = 0;
    char sign_x[2] = "+";
    char sign_y[2] = "+";
    int n = sscanf(arg, "%dx%d%1[+-]%d%1[+-]%d",
                   &w, &h, sign_x, &x, sign_y, &y);
    if ((n == 2 || n == 6) && w > 0 && h > 0 && x >= 0 && y >= 0) {
      if (sign_x[0] == '-')
        x = m_fullWidth - w - x;
      if (sign_y[0] == '-')
        y = m_fullHeight - h - y;
      Rect partRect(x, y, x + w, y + h);
      result = partRect.intersect(m_rect);
      if (result.area() <= 0) {
        vlog.error("Requested area is out of the desktop boundaries");
        result.clear();
      }
    } else {
      vlog.error("Wrong argument format");
    }
  } else {
    vlog.error("Missing argument");
  }

  return result;
}

