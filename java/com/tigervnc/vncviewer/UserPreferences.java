/* Copyright (C) 2012 Brian P. Hinz
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

//
// For storing user preferences not specified as configuration parameters.
//

package com.tigervnc.vncviewer;

import java.util.prefs.Preferences;
import java.util.prefs.BackingStoreException;

import com.tigervnc.rfb.*;

public class UserPreferences {

  private static Preferences root = Preferences.userRoot().node("TigerVNC");

  public static void set(String nName, String key, String val) {
    Preferences node = root.node(nName);
    node.put(key, val);
  }

  public static void set(String nName, String key, int val) {
    Preferences node = root.node(nName);
    node.putInt(key, val);
  }

  public static void set(String nName, String key, boolean val) {
    Preferences node = root.node(nName);
    node.putBoolean(key, val);
  }

  public static String get(String nName, String key) {
    Preferences node = root.node(nName);
    VoidParameter p = Configuration.getParam(key);
    if (p != null)
      return node.get(key, p.getDefaultStr());
    return node.get(key, null);
  }

  public static boolean getBool(String nName, String key, boolean defval) {
    Preferences node = root.node(nName);
    VoidParameter p = Configuration.getParam(key);
    if (p != null && p.isBool())
      return node.getBoolean(key, ((p.getDefaultStr() == "1") ? true : false));
    // for non-parameter preferences
    return node.getBoolean(key, defval);
  }

  public static boolean getBool(String nName, String key) {
    // default value of "false" arbitrarily chosen
    return getBool(nName, key, false);
  }

  public static int getInt(String nName, String key) {
    Preferences node = root.node(nName);
    VoidParameter p = Configuration.getParam(key);
    if (p != null && !p.isBool())
      return node.getInt(key, Integer.parseInt(p.getDefaultStr()));
    // FIXME
    return -1;
  }

  public static void save() {
    try {
      root.sync();
      String[] keys = root.keys();
      for (int i = 0; i < keys.length; i++)
        vlog.debug(keys[i]+" = "+root.get(keys[i], null));
    } catch (BackingStoreException e) {
      vlog.error(e.getMessage());
    }
  }

  public static void save(String nName) {
    try {
      Preferences node = root.node(nName);
      node.sync();
      String[] keys = root.keys();
      for (int i = 0; i < keys.length; i++)
        vlog.debug(keys[i]+" = "+node.get(keys[i], null));
    } catch (BackingStoreException e) {
      vlog.error(e.getMessage());
    }
  }

  public static void clear() {
    try {
      root.clear();
      String[] children = root.childrenNames();
      for (int i = 0; i < children.length; i++) {
        Preferences node = root.node(children[i]);
        node.removeNode();
      }
      root.sync();
    } catch (BackingStoreException e) {
      vlog.error(e.getMessage());
    }
  }

  public static void clear(String nName) {
    try {
      Preferences node = root.node(nName);
      node.clear();
      node.sync();
    } catch (BackingStoreException e) {
      vlog.error(e.getMessage());
    }
  }

  public static void load(String nName) {
    // Sets the value of any corresponding Configuration parameters
    try {
      Preferences node = root.node(nName);
      String[] keys = node.keys();
      for (int i = 0; i < keys.length; i++) {
        String key = keys[i];
        VoidParameter p = Configuration.getParam(key);
        if (p == null)
          continue;
        String valueStr = node.get(key, null);
        if (valueStr == null)
          valueStr = p.getDefaultStr();
        Configuration.setParam(key, valueStr);
      }
    } catch (BackingStoreException e) {
      vlog.error(e.getMessage());
    }
  }

  static LogWriter vlog = new LogWriter("UserPreferences");
}

