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

abstract public class VoidParameter {
  public VoidParameter(String name_, String desc_) {
    name = name_;
    description = desc_;
    next = Configuration.head;
    Configuration.head = this;
  }

  final public String getName() { return name; }
  final public String getDescription() { return description; }

  abstract public boolean setParam(String value);
  public boolean setParam() { return false; }
  abstract public String getDefaultStr();
  abstract public String getValueStr();
  public boolean isBool() { return false; }

  public VoidParameter next;
  protected String name;
  protected String description;
}
