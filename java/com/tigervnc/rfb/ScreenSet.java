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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

// Management class for the RFB virtual screens

package com.tigervnc.rfb;

import java.util.*;

public class ScreenSet {

  // Represents a complete screen configuration, excluding framebuffer
  // dimensions.

  public ScreenSet() {
    screens = new ArrayList<Screen>();
  }

  public final ListIterator<Screen> begin() { return screens.listIterator(0); }
  public final ListIterator<Screen> end() {
    return screens.listIterator(screens.size());
  }
  public final int num_screens() { return screens.size(); }

  public final void add_screen(Screen screen) { screens.add(screen); }
  public final void remove_screen(int id) {
    ListIterator iter, nextiter;
    for (iter = begin(); iter != end(); iter = nextiter) {
      nextiter = iter; nextiter.next();
      if (((Screen)iter.next()).id == id)
        iter.remove();
    }
  }

  public final boolean validate(int fb_width, int fb_height) {
      List<Integer> seen_ids = new ArrayList<Integer>();
      Rect fb_rect = new Rect();

      if (screens.isEmpty())
        return false;
      if (num_screens() > 255)
        return false;

      fb_rect.setXYWH(0, 0, fb_width, fb_height);

      for (Iterator<Screen> iter = screens.iterator(); iter.hasNext(); ) {
        Screen refScreen = (Screen)iter.next();
        if (refScreen.dimensions.is_empty())
          return false;
        if (!refScreen.dimensions.enclosed_by(fb_rect))
          return false;
        if (seen_ids.lastIndexOf(refScreen.id) != -1)
          return false;
        seen_ids.add(refScreen.id);
      }

      return true;
  }

  public final void debug_print() {
    vlog.debug(num_screens()+" screen(s)");
    for (Iterator<Screen> iter = screens.iterator(); iter.hasNext(); ) {
      Screen refScreen = (Screen)iter.next();
      vlog.debug("    "+refScreen.id+" (0x"+refScreen.id+"): "+
                refScreen.dimensions.width()+"x"+refScreen.dimensions.height()+
                "+"+refScreen.dimensions.tl.x+"+"+refScreen.dimensions.tl.y+
                " (flags 0x"+refScreen.flags+")");
    }
  }

  public ArrayList<Screen> screens;

  static LogWriter vlog = new LogWriter("ScreenSet");

}

