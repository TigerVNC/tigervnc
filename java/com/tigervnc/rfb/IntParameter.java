/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

public class IntParameter extends VoidParameter {
  public IntParameter(String name_, String desc_, int v) {
    super(name_, desc_);
    value = v;
    defValue = v;
  }

  public boolean setParam(String v) {
    try {
      value = Integer.parseInt(v);
    } catch (NumberFormatException e) {
      return false;
    }
    return true;
  }

  public String getDefaultStr() { return Integer.toString(defValue); }
  public String getValueStr() { return Integer.toString(value); }

  public int getValue() { return value; }

  protected int value;
  protected int defValue;
}
