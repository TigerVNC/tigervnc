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

// Represents a single RFB virtual screen, which includes
// coordinates, an id and flags.

package com.tigervnc.rfb;

public class Screen {

    public Screen() { id=0; flags=0; dimensions = new Rect(); }

    public Screen(int id_, int x_, int y_, int w_, int h_, int flags_) {
      id = id_;
      dimensions = new Rect(x_, y_, x_+w_, y_+h_);
      flags = flags_;
    }

    public final static boolean operator(Screen r) {
      if (id != r.id)
        return false;
      if (!dimensions.equals(r.dimensions))
        return false;
      if (flags != r.flags)
        return false;
      return true;
    }

    public static int id;
    public static Rect dimensions;
    public static int flags;

}
