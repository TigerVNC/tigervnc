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

public class StringParameter extends VoidParameter {
  public StringParameter(String name_, String desc_, String v,
                         Configuration.ConfigurationObject co)
  {
    super(name_, desc_, co);
    value = v;
    defValue = v;
  }

  public StringParameter(String name_, String desc_, String v)
  {
    this(name_, desc_, v, Configuration.ConfigurationObject.ConfGlobal);
  }

  public void setDefaultStr(String v) {
    value = defValue = v;
  }

  public boolean setParam(String v) {
    if (immutable) return true;
    if (v == null)
      throw new Exception("setParam(<null>) not allowed");
    value = v;
    return value != null;
  }

  public String getDefaultStr() { return defValue; }
  public String getValueStr() { return value; }

  public String getValue() { return value; }
  public String getData() { return value; }

  protected String value;
  protected String defValue;
}
