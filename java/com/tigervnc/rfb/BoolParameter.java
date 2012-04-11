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

public class BoolParameter extends VoidParameter {
  public BoolParameter(String name_, String desc_, boolean v) {
    super(name_, desc_);
    value = v;
    defValue = v;
  }

  public boolean setParam(String v) {
    if (v.equals("1") || v.equalsIgnoreCase("on") ||
        v.equalsIgnoreCase("true") || v.equalsIgnoreCase("yes"))
      value = true;
    else if (v.equals("0") || v.equalsIgnoreCase("off") ||
        v.equalsIgnoreCase("false") || v.equalsIgnoreCase("no"))
      value = false;
    else
      return false;
    return true;
  }

  public boolean setParam() { setParam(true); return true; }
  public void setParam(boolean b) { value = b; }

  public String getDefaultStr() { return defValue ? "1" : "0"; }
  public String getValueStr() { return value ? "1" : "0"; }
  public boolean isBool() { return true; }

  final public boolean getValue() { return value; }

  protected boolean value;
  protected boolean defValue;
}
