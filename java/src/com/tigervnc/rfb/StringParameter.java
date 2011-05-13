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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.tigervnc.rfb;

public class StringParameter extends VoidParameter {
  public StringParameter(String name_, String desc_, String v) {
    super(name_, desc_);
    value = v;
    defValue = v;
  }

  public boolean setParam(String v) {
    value = v;
    return value != null;
  }

  public boolean setDefaultStr(String v) {
    value = defValue = v;
    return defValue != null;
  }

  public String getDefaultStr() { return defValue; }
  public String getValueStr() { return value; }

  public String getValue() { return value; }
  public String getData() { return value; }

  protected String value;
  protected String defValue;
}
