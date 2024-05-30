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

package com.tigervnc.vncviewer;

import javax.swing.filechooser.FileSystemView;

import com.tigervnc.rfb.LogWriter;

import java.io.File;

public class FileUtils {

  public static String getHomeDir() {
    String homeDir = null;
    try {
      String os = System.getProperty("os.name");
      try {
        if (os.startsWith("Windows")) {
          // JRE prior to 1.5 cannot reliably determine USERPROFILE
          // return user.home and hope it's right...
          if (Integer.parseInt(System.getProperty("java.version").split("\\.")[1]) < 5) {
            try {
              homeDir = System.getProperty("user.home");
            } catch(java.security.AccessControlException e) {
              vlog.error("Cannot access user.home system property:"+e.getMessage());
            }
          } else {
            homeDir = System.getenv("USERPROFILE");
          }
        } else {
          try {
          homeDir = FileSystemView.getFileSystemView().
            getDefaultDirectory().getCanonicalPath();
          } catch(java.security.AccessControlException e) {
            vlog.error("Cannot access system property:"+e.getMessage());
          }
        }
      } catch (java.lang.Exception e) {
        e.printStackTrace();
      }
    } catch(java.security.AccessControlException e) {
      vlog.error("Cannot access os.name system property:"+e.getMessage());
    }

    return homeDir + getFileSeparator();
  }

  public static String getVncDir(String xdgEnv, String xdgDefault) {
    File legacyDir = new File(getHomeDir() + ".vnc" + getFileSeparator());
    String os = System.getProperty("os.name");

    if (os.startsWith("Windows")) {
      File newDir = new File(System.getenv("APPDATA") + getFileSeparator() + "TigerVNC" + getFileSeparator());
      if (!newDir.exists()) {
        newDir.mkdirs();
      }
      File[] existingFiles = legacyDir.listFiles();
      if (existingFiles != null) {
        for (File file : existingFiles) {
          file.renameTo(new File(newDir.getPath() + file.getName()));
        }
        legacyDir.delete();
      }
      return newDir.getPath();
    } else {
      if (legacyDir.exists()) {
        vlog.info("WARNING: ~/.vnc is deprecated, please consult 'man vncviewer' for paths to migrate to.");
        return legacyDir.getPath();
      }
      String xdgBaseDir = System.getenv(xdgEnv);
      return (xdgBaseDir != null && xdgBaseDir.startsWith("/"))
        ? xdgBaseDir + getFileSeparator() + "tigervnc" + getFileSeparator()
        : getHomeDir() + xdgDefault + getFileSeparator() + "tigervnc" + getFileSeparator();
    }
  }

  public static String getVncConfigDir() {
    return getVncDir("XDG_CONFIG_HOME", ".config");
  }

  public static String getVncDataDir() {
    return getVncDir("XDG_DATA_HOME", ".local" + getFileSeparator() + "share");
  }

  public static String getVncStateDir() {
    return getVncDir("XDG_STATE_HOME", ".local" + getFileSeparator() + "state");
  }

  public static String getFileSeparator() {
    String separator = null;
    try {
      separator = Character.toString(java.io.File.separatorChar);
    } catch(java.security.AccessControlException e) {
      vlog.error("Cannot access file.separator system property:"+e.getMessage());
    }
    return separator;
  }

  static LogWriter vlog = new LogWriter("FileUtils");
}
