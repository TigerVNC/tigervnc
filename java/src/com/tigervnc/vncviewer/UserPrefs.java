/* Copyright (C) 2011 TigerVNC Team.  All Rights Reserved.
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

package com.tigervnc.vncviewer;

import java.util.Properties;
import java.io.FileOutputStream;
import javax.swing.filechooser.*;

/**
 * Simple, generic class to load up or save properties for an application.
 *  (eg user preferences)
 *
 * While reading in values, it will automatically convert from string format,
 * to your choice of int or boolean, if desired.
 *
 * We assume that you want properties stored in a file named
 * $HOME/.yourappname, where $HOME represents the users "home directory"
 *
 * Feel free to email comments or suggestions to the author
 *
 * @version 1.0, 09/16/1997
 * @author Philip Brown phil@bolthole.com
 */

public class UserPrefs extends Properties {
	String userhome=null; //This will have fileseperator on end if it
	String prefFile;
	String appName;
	
	Properties systemprops;
	

	/**
	 * We try to read in a preferences file, from the user's "HOME"
	 * directory. We base the name of the file, on the name of the
	 * application we are in.
	 * Use the getHomeDir() call if you want to know what directory
	 * this is in.
	 *
	 * @param appName_ name of application calling this class
	 */
	public UserPrefs(String appName_) {
		appName = appName_;
		
		systemprops=System.getProperties();
		// This is guaranteed as always being some valid directory,
		// according to spec.
		prefFile= getHomeDir()+getFileSeperator()+
                "."+appName;
		try {
			load(new java.io.FileInputStream(prefFile));
		} catch (Exception err)	{
			if(err instanceof java.io.FileNotFoundException) {
        try {
		      store(new FileOutputStream(prefFile), appName+" preferences");
        } catch (Exception e) { /* FIXME */ }
      } else {
        // FIXME - should be a dialog
			  System.out.println("Error opening prefs file:"+err.getMessage());
      }
		}
		
	}

	// Strip off any comments
	String trimValue(String prop) {
		if(prop==null)
			return null;
		
		int lastpos;
		lastpos=prop.indexOf('#');
		if(lastpos==-1)
			lastpos=prop.length()-1;
		while((prop.charAt(lastpos)==' ') ||
		      (prop.charAt(lastpos)=='\t'))	{
			lastpos--;
			if(lastpos==0)
				break;
		}
		      
		return prop.substring(0, lastpos+1);
	}

	/**
	 * The java spec guarantees that a "home" directory be
	 * specified. We look it up for our own uses at initialization
	 * If you want to do other things in the user's home dir,
	 * this routine is an easy way to find out where it is.
	 *
	 *  This returns string that will have trailing fileseparator, eg "/")
	 *  so you can slap together a filename directly after it, and
	 *  not worry about that sort of junk.
	 */
  final public static String getHomeDir() {
    String homeDir = null;
    String os = System.getProperty("os.name");
    try {
      if (os.startsWith("Windows")) {
        // JRE prior to 1.5 cannot reliably determine USERPROFILE
        // return user.home and hope it's right...
        if (Integer.parseInt(System.getProperty("java.version").split("\\.")[1]) < 5) {
          homeDir = System.getProperty("user.home");
        } else {
          homeDir = System.getenv("USERPROFILE");
        }
      } else {
        homeDir = FileSystemView.getFileSystemView().
          getDefaultDirectory().getCanonicalPath();
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    return homeDir;
  }

  final private String getUserName() {
    String userName = (String) System.getProperties().get("user.name");
    return userName;
  }

  final public static String getFileSeperator() {
    String seperator = System.getProperties().get("file.separator").toString();
    return seperator;
  }

	/**
	 * way to directly set a preference. You'll have to
	 * do your own type conversion.
	 * @param prefname name of property
	 * @param value string value of property
	 */
	public void setPref(String prefname, String value) {
		setProperty(prefname, value);
	}

	public void setPref(String prefname, int value) {
		systemprops.put(prefname, java.lang.Integer.toString(value));
	}

	public void setPref(String prefname, boolean value) {
		put(prefname, (value ? "true" : "false"));
	}
	
	/**
	 * Gets named resource, as a string value.
	 * returns null if no such resource defined.
	 * @param name name of property
	 */
	public String getString(String name) {
		return trimValue(getProperty(name));
	}
	/**
	 * Gets named resource, as a string value.
	 *
	 * @param name name of property
	 * @param defval default value to remember and return , if
	 *   no existing property name found.
	 */
	public String getString(String name, String defstr) {
		String val = trimValue(getProperty(name));
		if(val==null) {
			setPref(name, defstr);
			return defstr;
		}
		return val;
	}

	/**
	 * look up property that is an int value
	 * @param name name of property
	 */	
	public int getInt(String name) {
		String strint = trimValue(getProperty(name));
		int val=0;
		try {
			val = Integer.parseInt(strint);
		} catch (NumberFormatException err) {
			//we dont care
		}
		return val;
	}

	/**
	 * look up property that is an int value
	 * @param name name of property
	 * @param defval default value to remember and return , if
	 *   no existing property name found.
	 */	
	public int getInt(String name, int defval) {
		String strint = trimValue(getProperty(name));
		if(strint==null) {
			setPref(name, String.valueOf(defval));
			return defval;
		}
		int val=0;
		try {
			val = Integer.parseInt(strint);
		} catch (NumberFormatException err) {
			//we dont care
		}
		return val;
	}

	/**
	 * look up property that is a boolean value
	 * @param name name of property
	 * @param defval default value to remember and return , if
	 *   no existing property name found.
	 */	
	public boolean getBool(String name) {
		String strval = trimValue(getProperty(name));
		if(strval.equals("true"))
			return true;

		return false;
	}

	/**
	 * @param name name of property
	 * @param defval default value to remember and return , if
	 *   no existing property name found.
	 */	
	public boolean getBool(String name, boolean defval) {
		String strval = trimValue(getProperty(name));
		if(strval==null) {
			setPref(name, String.valueOf(defval));
			return defval;
		}

		if(strval.equals("true"))
			return true;

		return false;
	}

	/**
	* save user preferences to default file. Duh.
	*/
	public void Save() throws java.io.IOException {
		store(new FileOutputStream(prefFile), appName+" preferences");
	}
}
