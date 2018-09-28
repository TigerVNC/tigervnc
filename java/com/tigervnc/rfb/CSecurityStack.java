/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
 * Copyright (C) 2010 TigerVNC Team
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

public class CSecurityStack extends CSecurity {

  public CSecurityStack(int Type, String Name, CSecurity s0,
  			       CSecurity s1)
  {
    name = Name;
    type = Type;
    state = 0;
    state0 = s0;
    state1 = s1;
  }

  public boolean processMsg(CConnection cc)
  {
    boolean res = true;
    if (state == 0) {
      if (state0 != null)
        res = state0.processMsg(cc);

      if (!res)
        return res;

      state++;
    }

    if (state == 1) {
      if(state1 != null)
        res = state1.processMsg(cc);

      if(!res)
        return res;

      state++;
    }

    return res;
  }

  public boolean isSecure()
  {
    if (state0 != null && state0.isSecure())
      return true;
    if (state == 1 && state1 != null && state1.isSecure())
      return true;
    return false;
  }

  public final int getType() { return type; }
  public final String description() { return name; }

  private int state;
  private CSecurity state0;
  private CSecurity state1;
  private String name;
  private int type;

}
