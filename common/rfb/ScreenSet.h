/* Copyright 2009 Pierre Ossman for Cendio AB
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

// Management class for the RFB virtual screens

#ifndef __RFB_SCREENSET_INCLUDED__
#define __RFB_SCREENSET_INCLUDED__

#include <rfb/Rect.h>
#include <list>
#include <set>

namespace rfb {

  // rfb::Screen
  //
  // Represents a single RFB virtual screen, which includes
  // coordinates, an id and flags.

  struct Screen {
    Screen(void) : id(0), flags(0) {};
    Screen(rdr::U32 id_, int x_, int y_, int w_, int h_, rdr::U32 flags_) :
      id(id_), dimensions(x_, y_, x_+w_, y_+h_), flags(flags_) {};
    rdr::U32 id;
    Rect dimensions;
    rdr::U32 flags;
  };

  // rfb::ScreenSet
  //
  // Represents a complete screen configuration, excluding framebuffer
  // dimensions.

  struct ScreenSet {
    ScreenSet(void) {};
    inline void add_screen(const Screen screen) { screens.push_back(screen); };
    inline bool validate(int fb_width, int fb_height) const {
      std::list<Screen>::const_iterator iter;
      std::set<rdr::U32> seen_ids;
      Rect fb_rect;

      if (screens.empty())
        return false;

      fb_rect.setXYWH(0, 0, fb_width, fb_height);

      for (iter = screens.begin();iter != screens.end();++iter) {
        if (iter->dimensions.is_empty())
          return false;
        if (!iter->dimensions.enclosed_by(fb_rect))
          return false;
        if (seen_ids.find(iter->id) != seen_ids.end())
          return false;
        seen_ids.insert(iter->id);
      }

      return true;
    };
    std::list<Screen> screens;
  };

};

#endif

