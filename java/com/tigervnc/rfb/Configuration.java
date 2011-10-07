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

//
// Configuration - class for dealing with configuration parameters.
//

package com.tigervnc.rfb;

public class Configuration {

  // - Set named parameter to value
  public static boolean setParam(String name, String value) {
    VoidParameter param = getParam(name);
    if (param == null) return false;
    return param.setParam(value);
  }

  // - Set parameter to value (separated by "=")
  public static boolean setParam(String config) {
    boolean hyphen = false;
    if (config.charAt(0) == '-') {
      hyphen = true;
      if (config.charAt(1) == '-')
        config = config.substring(2); // allow gnu-style --<option>
      else
        config = config.substring(1);
    }
    int equal = config.indexOf('=');
    if (equal != -1) {
      return setParam(config.substring(0, equal), config.substring(equal+1));
    } else if (hyphen) {
      VoidParameter param = getParam(config);
      if (param == null) return false;
      return param.setParam();
    }
    return false;
  }

  // - Get named parameter
  public static VoidParameter getParam(String name) {
    VoidParameter current = head;
    while (current != null) {
      if (name.equalsIgnoreCase(current.getName()))
        return current;
      current = current.next;
    }
    return null;
  }

  public static String listParams() {
    StringBuffer s = new StringBuffer();

    VoidParameter current = head;
    while (current != null) {
      String def_str = current.getDefaultStr();
      String desc = current.getDescription();
      s.append("  "+current.getName()+" - "+desc+" (default="+def_str+")\n");
      current = current.next;
    }

    return s.toString();
  }

  public static void readAppletParams(java.applet.Applet applet) {
    VoidParameter current = head;
    while (current != null) {
      String str = applet.getParameter(current.getName());
      if (str != null)
        current.setParam(str);
      current = current.next;
    }
  }

  public static VoidParameter head;
}
