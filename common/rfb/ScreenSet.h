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

#include <stdio.h>
#include <string.h>

#include <rdr/types.h>
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

    inline bool operator==(const Screen& r) const {
      if (id != r.id)
        return false;
      if (!dimensions.equals(r.dimensions))
        return false;
      if (flags != r.flags)
        return false;
      return true;
    }

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

    typedef std::list<Screen>::iterator iterator;
    typedef std::list<Screen>::const_iterator const_iterator;

    inline iterator begin(void) { return screens.begin(); };
    inline const_iterator begin(void) const { return screens.begin(); };
    inline iterator end(void) { return screens.end(); };
    inline const_iterator end(void) const { return screens.end(); };

    inline int num_screens(void) const { return screens.size(); };

    inline void add_screen(const Screen screen) { screens.push_back(screen); };
    inline void remove_screen(rdr::U32 id) {
      std::list<Screen>::iterator iter, nextiter;
      for (iter = screens.begin();iter != screens.end();iter = nextiter) {
        nextiter = iter; nextiter++;
        if (iter->id == id)
            screens.erase(iter);
      }
    }

    inline bool validate(int fb_width, int fb_height) const {
      std::list<Screen>::const_iterator iter;
      std::set<rdr::U32> seen_ids;
      Rect fb_rect;

      if (screens.empty())
        return false;
      if (num_screens() > 255)
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

    inline void print(char* str, size_t len) const {
      char buffer[128];
      std::list<Screen>::const_iterator iter;
      snprintf(buffer, sizeof(buffer), "%d screen(s)\n", num_screens());
      str[0] = '\0';
      strncat(str, buffer, len - 1 - strlen(str));
      for (iter = screens.begin();iter != screens.end();++iter) {
        snprintf(buffer, sizeof(buffer),
                 "    %10d (0x%08x): %dx%d+%d+%d (flags 0x%08x)\n",
                 (int)iter->id, (unsigned)iter->id,
                 iter->dimensions.width(), iter->dimensions.height(),
                 iter->dimensions.tl.x, iter->dimensions.tl.y,
                 (unsigned)iter->flags);
        strncat(str, buffer, len - 1 - strlen(str));
      }
    };

    inline bool operator==(const ScreenSet& r) const {
      std::list<Screen> a = screens;
      a.sort(compare_screen);
      std::list<Screen> b = r.screens;
      b.sort(compare_screen);
      return a == b;
    };
    inline bool operator!=(const ScreenSet& r) const { return !operator==(r); }

    std::list<Screen> screens;

  private:
    static inline bool compare_screen(const Screen& first, const Screen& second)
    {
      return first.id < second.id;
    }

  };

};

#endif

