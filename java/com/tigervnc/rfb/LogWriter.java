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

public class LogWriter {

  public LogWriter(String name_) {
    name = name_;
    level = globalLogLevel;
    next = log_writers;
    log_writers = this;
  }

  public void setLevel(int level_) { level = level_; }

  public void write(int level, String str) {
    if (level <= this.level) {
      System.err.println(name+": "+str);
    }
  }

  public void error(String fmt, Object... args) {
    write(0, String.format(fmt, args));
  }
  public void status(String fmt, Object... args) {
    write(10, String.format(fmt, args));
  }
  public void info(String fmt, Object... args) {
    write(30, String.format(fmt, args));
  }
  public void debug(String fmt, Object... args) {
    write(100, String.format(fmt, args));
  }

  public static boolean setLogParams(String params) {
    globalLogLevel = Integer.parseInt(params);
    LogWriter current = log_writers;
    while (current != null) {
      current.setLevel(globalLogLevel);
      current = current.next;
    }
    return true;
//      int colon = params.indexOf(':');
//      String logwriter_name = params.substring(0, colon);
//      params = params.substring(colon+1);
//      colon = params.indexOf(':');
//      String logger_name = params.substring(0, colon);
//      params = params.substring(colon+1);
//      int level = Integer.parseInt(params);
//      // XXX ignore logger name for the moment

//      System.err.println("setting level to "+level);
//      System.err.println("logwriters is "+log_writers);
//      if (logwriter_name.equals("*")) {
//        LogWriter current = log_writers;
//        while (current != null) {
//          //current.setLog(logger);
//          System.err.println("setting level of "+current.name+" to "+level);
//          current.setLevel(level);
//          current = current.next;
//        }
//        return true;
//      }

//      LogWriter logwriter = getLogWriter(logwriter_name);
//      if (logwriter == null) {
//        System.err.println("no logwriter found: "+logwriter_name);
//        return false;
//      }

//      //logwriter.setLog(logger);
//      logwriter.setLevel(level);
//      return true;
  }


  static LogWriter getLogWriter(String name) {
    LogWriter current = log_writers;
    while (current != null) {
      if (name.equalsIgnoreCase(current.name)) return current;
      current = current.next;
    }
    return null;
  }

  String name;
  int level;
  LogWriter next;
  static LogWriter log_writers;
  static int globalLogLevel = 30;
}
