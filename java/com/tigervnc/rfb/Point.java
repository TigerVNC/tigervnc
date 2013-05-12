/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 Brian P. Hinz
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

public class Point {

  // Point
  //
  // Represents a point in 2D space, by X and Y coordinates.
  // Can also be used to represent a delta, or offset, between
  // two Points.
  // Functions are provided to allow Points to be compared for
  // equality and translated by a supplied offset.
  // Functions are also provided to negate offset Points.

  public Point() {x=0; y=0;}
  public Point(int x_, int y_) { x=x_; y=y_;}
  public final Point negate() {return new Point(-x, -y);}
  public final boolean equals(Point p) {return (x==p.x && y==p.y);}
  public final Point translate(Point p) {return new Point(x+p.x, y+p.y);}
  public final Point subtract(Point p) {return new Point(x-p.x, y-p.y);}
  public int x, y;

}
