/* Copyright 2009-2026 Pierre Ossman for Cendio AB
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

#include <stdint.h>

#include <list>
#include <string>

#include <core/Rect.h>

namespace rfb {

  // rfb::Screen
  //
  // Represents a single RFB virtual screen, which includes
  // coordinates, an id and flags.

  struct Screen {
    Screen(void) : id(0), flags(0) {};
    Screen(uint32_t id_, int x_, int y_, int w_, int h_, uint32_t flags_) :
      id(id_), dimensions(x_, y_, x_+w_, y_+h_), flags(flags_) {};

    inline bool operator==(const Screen& r) const {
      if (id != r.id)
        return false;
      if (dimensions != r.dimensions)
        return false;
      if (flags != r.flags)
        return false;
      return true;
    }

    uint32_t id;
    core::Rect dimensions;
    uint32_t flags;
  };

  // rfb::ScreenSet
  //
  // Represents a complete screen configuration, excluding framebuffer
  // dimensions.

  struct ScreenSet {
    ScreenSet(void) {};

    typedef std::list<Screen>::iterator iterator;
    typedef std::list<Screen>::const_iterator const_iterator;

    inline iterator begin(void) { return screens.begin(); };
    inline const_iterator begin(void) const { return screens.begin(); };
    inline iterator end(void) { return screens.end(); };
    inline const_iterator end(void) const { return screens.end(); };

    inline int num_screens(void) const { return screens.size(); };

    void add_screen(const Screen screen);
    void remove_screen(uint32_t id);

    bool validate(int fb_width, int fb_height) const;

    std::string print() const;

    bool operator==(const ScreenSet& r) const;
    bool operator!=(const ScreenSet& r) const;

    std::list<Screen> screens;
  };

};

#endif

