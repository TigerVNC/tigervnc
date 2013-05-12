/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2012 Brian P. Hinz
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
  public IntParameter(String name_, String desc_, int v,
                      int minValue_, int maxValue_,
                      Configuration.ConfigurationObject co)
  {
    super(name_, desc_, co);
    value = v;
    defValue = v;
    minValue = minValue_;
    maxValue = maxValue_;
  }

  public IntParameter(String name_, String desc_, int v)
  {
    this(name_, desc_, v, Integer.MIN_VALUE, Integer.MAX_VALUE,
         Configuration.ConfigurationObject.ConfGlobal);
  }

  public boolean setParam(String v) {
    if (immutable) return true;
    vlog.debug("set "+getName()+"(Int) to "+v);
    try {
      int i;
      i = Integer.parseInt(v);
      if (i < minValue || i > maxValue)
        return false;
      value = i;
      return true;
    } catch (NumberFormatException e) {
      throw new Exception(e.toString());
    }
  }

  public boolean setParam(int v) {
    if (immutable) return true;
    vlog.debug("set "+getName()+"(Int) to "+v);
    if (v < minValue || v > maxValue)
      return false;
    value = v;
    return true;
  }

  public String getDefaultStr() { return Integer.toString(defValue); }
  public String getValueStr() { return Integer.toString(value); }

  //public int int() { return value; }
  public int getValue() { return value; }

  protected int value;
  protected int defValue;
  protected int minValue;
  protected int maxValue;
}
