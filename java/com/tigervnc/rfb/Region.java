/* Copyright (C) 2016 Brian P. Hinz.  All Rights Reserved.
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

package com.tigervnc.rfb;

import java.awt.*;
import java.awt.geom.*;

public class Region extends Area {

  // Create an empty region
  public Region() {
    super();
  }

  // Create a rectangular region
  public Region(Rect r) {
    super(new Rectangle(r.tl.x, r.tl.y, r.width(), r.height()));
  }

  public Region(Region r) {
    super(r);
  }

  public void clear() { reset(); }

  public void reset(Rect r) {
    if (r.is_empty()) {
      clear();
    } else {
      clear();
      assign_union(new Region(r));
    }
  }

  public void translate(Point delta) {
    AffineTransform t = 
      AffineTransform.getTranslateInstance((double)delta.x, (double)delta.y);
    transform(t);
  }

  public void assign_intersect(Region r) {
    super.intersect(r);
  }

  public void assign_union(Region r) {
    super.add(r);
  }

  public void assign_subtract(Region r) {
    super.subtract(r);
  }

  public Region intersect(Region r) {
    Region reg = new Region(this);
    ((Area)reg).intersect(r);
    return reg;
  }

  public Region union(Region r) {
    Region reg = new Region(this);
    ((Area)reg).add(r);
    return reg;
  }

  public Region subtract(Region r) {
    Region reg = new Region(this);
    ((Area)reg).subtract(r);
    return reg;
  }

  public boolean is_empty() { return isEmpty(); }

  public Rect get_bounding_rect() {
    Rectangle b = getBounds();
    return new Rect((int)b.getX(), (int)b.getY(),
                    (int)b.getWidth(), (int)b.getHeight());
  }
}
