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

//
// Configuration - class for dealing with configuration parameters.
//

package com.tigervnc.rfb;

import java.io.FileInputStream;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;

import com.tigervnc.vncviewer.VncViewer;

public class Configuration {

  static LogWriter vlog = new LogWriter("Configuration");

  private static final String IDENTIFIER_STRING = "TigerVNC Configuration file Version 1.0";

  public enum ConfigurationObject { ConfGlobal, ConfServer, ConfViewer };

  // -=- The Global/server/viewer Configuration objects
  private static Configuration global_ = null;
  private static Configuration server_ = null;
  private static Configuration viewer_ = null;

  public static Configuration global() {
    if (global_ == null)
      global_ = new Configuration("Global");
    return global_;
  }

  public static Configuration server() {
    if (server_ == null)
      server_ = new Configuration("Server");
    return server_;
  }

  public static Configuration viewer() {
    if (viewer_ == null)
      viewer_ = new Configuration("Viewer");
    return viewer_;
  }

  // Enable server/viewer specific parameters
  public static void enableServerParams() { global().appendConfiguration(server()); }
  public static void enableViewerParams() { global().appendConfiguration(viewer()); }

  // Append configuration object to this instance.
  // NOTE: conf instance can be only one configuration object
  public void appendConfiguration(Configuration conf) {
    conf._next = _next; _next = conf;
  }

  // -=- Configuration implementation
  public Configuration(String name_, Configuration attachToGroup) {
    name = name_; head = null; _next = null;
    if (attachToGroup != null) {
      _next = attachToGroup._next;
      attachToGroup._next = this;
    }
  }

  public Configuration(String name_) {
    this(name_, null);
  }

  // - Return the buffer containing the Configuration's name
  final public String getName() { return name; }

  // - Assignment operator.  For every Parameter in this Configuration's
  //   group, get()s the corresponding source parameter and copies its
  //   content.
  public Configuration assign(Configuration src) {
    VoidParameter current = head;
    while (current != null) {
      VoidParameter srcParam = ((Configuration)src).get(current.getName());
      if (srcParam != null) {
        current.immutable = false;
        String value = srcParam.getValueStr();
        vlog.debug("operator=("+current.getName()+", "+value+")");
        current.setParam(value);
      }
      current = current._next;
    }
    if (_next != null)
      _next = src;
    return this;
  }

  // - Set named parameter to value
  public boolean set(String n, String v, boolean immutable) {
    return set(n, n.length(), v, immutable);
  }

  public boolean set(String n, String v) {
    return set(n, n.length(), v, false);
  }

  // - Set parameter to value (separated by "=")
  public boolean set(String name, int len,
                     String val, boolean immutable)
  {
    VoidParameter current = head;
    while (current != null) {
      if (current.getName().length() == len &&
          current.getName().equalsIgnoreCase(name.substring(0, len)))
      {
        boolean b = current.setParam(val);
        current.setHasBeenSet();
        if (b && immutable)
  	  current.setImmutable();
        return b;
      }
      current = current._next;
    }
    return (_next != null) ? _next.set(name, len, val, immutable) : false;
  }

  // - Set named parameter to value, with name truncated at len
  boolean set(String config, boolean immutable) {
    boolean hyphen = false;
    if (config.charAt(0) == '-') {
      hyphen = true;
      config = config.substring(1);
      if (config.charAt(0) == '-') config = config.substring(1); // allow gnu-style --<option>
    }
    int equal = config.indexOf('=');
    if (equal > -1) {
      return set(config, equal, config.substring(equal+1), immutable);
    } else if (hyphen) {
      VoidParameter current = head;
      while (current != null) {
        if (current.getName().equalsIgnoreCase(config)) {
          boolean b = current.setParam();
  	  current.setHasBeenSet();
          if (b && immutable)
  	    current.setImmutable();
          return b;
        }
        current = current._next;
      }
    }
    return (_next != null) ? _next.set(config, immutable) : false;
  }

  boolean set(String config) {
    return set(config, false);
  }

  // - Container for process-wide Global parameters
  public static boolean setParam(String param, String value, boolean immutable) {
    return global().set(param, value, immutable);
  }

  public static boolean setParam(String param, String value) {
    return setParam(param, value, false);
  }

  public static boolean setParam(String config, boolean immutable) {
    return global().set(config, immutable);
  }

  public static boolean setParam(String config) {
    return setParam(config, false);
  }

  public static boolean setParam(String name, int len,
                                 String val, boolean immutable) {
    return global().set(name, len, val, immutable);
  }


  // - Get named parameter
  public VoidParameter get(String param)
  {
    VoidParameter current = head;
    while (current != null) {
      if (current.getName().equalsIgnoreCase(param))
        return current;
      current = current._next;
    }
    return (_next != null) ? _next.get(param) : null;
  }

  public static VoidParameter getParam(String param) { return global().get(param); }

  public static void listParams(int width, int nameWidth) {
    global().list(width, nameWidth);
  }
  public static void listParams() {
    listParams(79, 10);
  }

  public void list(int width, int nameWidth) {
    VoidParameter current = head;

    System.err.format("%s Parameters:%n", name);
    while (current != null) {
      String def_str = current.getDefaultStr();
      String desc = current.getDescription().trim();
      String format = "  %-"+nameWidth+"s -";
      System.err.format(format, current.getName());
      int column = current.getName().length();
      if (column < nameWidth) column = nameWidth;
      column += 4;
      while (true) {
        int s = desc.indexOf(' ');
        int wordLen;
        if (s > -1) wordLen = s;
        else wordLen = desc.length();

        if (column + wordLen + 1 > width) {
          format = "%n%"+(nameWidth+4)+"s";
          System.err.format(format, "");
          column = nameWidth+4;
        }
        format = " %"+wordLen+"s";
        System.err.format(format, desc.substring(0, wordLen));
        column += wordLen + 1;

        if (s == -1) break;
        desc = desc.substring(wordLen+1);
      }

      if (def_str != null) {
        if (column + def_str.length() + 11 > width)
          System.err.format("%n%"+(nameWidth+4)+"s","");
        System.err.format(" (default=%s)%n",def_str);
        def_str = null;
      } else {
        System.err.format("%n");
      }
      current = current._next;
    }

    if (_next != null)
      _next.list(width, nameWidth);
  }

  public void list() {
    list(79, 10);
  }

  // Name for this Configuration
  private String name;

  // - Pointer to first Parameter in this group
  public VoidParameter head;

  // Pointer to next Configuration in this group
  public Configuration _next;

}
