/* Copyright (C) 2016-2019 Brian P. Hinz
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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.PrintWriter;
import java.util.StringTokenizer;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;

public class Parameters {

  public static BoolParameter noLionFS
  = new BoolParameter("NoLionFS",
    "On Mac systems, setting this parameter will force the use of the old "+
    "(pre-Lion) full-screen mode, even if the viewer is running on OS X 10.7 "+
    "Lion or later.",
    false);

  public static BoolParameter dotWhenNoCursor
  = new BoolParameter("DotWhenNoCursor",
    "Show the dot cursor when the server sends an invisible cursor",
    false);

  public static BoolParameter sendLocalUsername
  = new BoolParameter("SendLocalUsername",
    "Send the local username for SecurityTypes "+
    "such as Plain rather than prompting",
    true);

  public static StringParameter passwordFile
  = new StringParameter("PasswordFile",
    "Password file for VNC authentication",
    "");

  public static AliasParameter passwd
  = new AliasParameter("passwd",
    "Alias for PasswordFile",
    passwordFile);

  public static BoolParameter autoSelect
  = new BoolParameter("AutoSelect",
    "Auto select pixel format and encoding",
    true);

  public static BoolParameter fullColor
  = new BoolParameter("FullColor",
    "Use full color - otherwise 6-bit colour is used "+
    "until AutoSelect decides the link is fast enough",
    true);

  public static AliasParameter fullColorAlias
  = new AliasParameter("FullColour",
    "Alias for FullColor",
    Parameters.fullColor);

  public static IntParameter lowColorLevel
  = new IntParameter("LowColorLevel",
    "Color level to use on slow connections. "+
    "0 = Very Low, 1 = Low, 2 = Medium",
    2);

  public static AliasParameter lowColorLevelAlias
  = new AliasParameter("LowColourLevel",
    "Alias for LowColorLevel",
    lowColorLevel);

  public static StringParameter preferredEncoding
  = new StringParameter("PreferredEncoding",
    "Preferred encoding to use (Tight, ZRLE, "+
    "hextile or raw) - implies AutoSelect=0",
    "Tight");

  public static BoolParameter remoteResize
  = new BoolParameter("RemoteResize",
    "Dynamically resize the remote desktop size as "+
    "the size of the local client window changes. "+
    "(Does not work with all servers)",
    true);

  public static BoolParameter viewOnly
  = new BoolParameter("ViewOnly",
    "Don't send any mouse or keyboard events to the server",
    false);

  public static BoolParameter shared
  = new BoolParameter("Shared",
    "Don't disconnect other viewers upon "+
    "connection - share the desktop instead",
    false);

  public static BoolParameter maximize
  = new BoolParameter("Maximize",
    "Maximize viewer window",
    false);

  public static BoolParameter fullScreen
  = new BoolParameter("FullScreen",
    "Enable full screen",
    false);

  public static BoolParameter fullScreenAllMonitors
  = new BoolParameter("FullScreenAllMonitors",
    "Enable full screen over all monitors",
    true);

  public static BoolParameter acceptClipboard
  = new BoolParameter("AcceptClipboard",
    "Accept clipboard changes from the server",
    true);

  public static BoolParameter sendClipboard
  = new BoolParameter("SendClipboard",
    "Send clipboard changes to the server",
    true);

  public static IntParameter maxCutText
  = new IntParameter("MaxCutText",
    "Maximum permitted length of an outgoing clipboard update",
    262144);

  public static StringParameter menuKey
  = new StringParameter("MenuKey",
    "The key which brings up the popup menu",
    "F8");

  public static StringParameter desktopSize
  = new StringParameter("DesktopSize",
    "Reconfigure desktop size on the server on connect (if possible)",
    "");

  public static BoolParameter listenMode
  = new BoolParameter("listen",
    "Listen for connections from VNC servers",
    false);

  public static StringParameter scalingFactor
  = new StringParameter("ScalingFactor",
    "Reduce or enlarge the remote desktop image. "+
    "The value is interpreted as a scaling factor "+
    "in percent. If the parameter is set to "+
    "\"Auto\", then automatic scaling is "+
    "performed. Auto-scaling tries to choose a "+
    "scaling factor in such a way that the whole "+
    "remote desktop will fit on the local screen. "+
    "If the parameter is set to \"FixedRatio\", "+
    "then automatic scaling is performed, but the "+
    "original aspect ratio is preserved.",
    "100");

  public static BoolParameter alwaysShowServerDialog
  = new BoolParameter("AlwaysShowServerDialog",
    "Always show the server dialog even if a server has been "+
    "specified in an applet parameter or on the command line",
    false);

  public static BoolParameter acceptBell
  = new BoolParameter("AcceptBell",
    "Produce a system beep when requested to by the server.",
    true);

  public static StringParameter via
  = new StringParameter("Via",
    "Automatically create an encrypted TCP tunnel to "+
    "the gateway machine, then connect to the VNC host "+
    "through that tunnel. By default, this option invokes "+
    "SSH local port forwarding using the embedded JSch "+
    "client, however an external SSH client may be specified "+
    "using the \"-extSSH\" parameter. Note that when using "+
    "the -via option, the VNC host machine name should be "+
    "specified from the point of view of the gateway machine, "+
    "e.g. \"localhost\" denotes the gateway, "+
    "not the machine on which the viewer was launched. "+
    "See the System Properties section below for "+
    "information on configuring the -Via option.", "");

  public static BoolParameter tunnel
  = new BoolParameter("Tunnel",
    "The -Tunnel command is basically a shorthand for the "+
    "-via command when the VNC server and SSH gateway are "+
    "one and the same. -Tunnel creates an SSH connection "+
    "to the server and forwards the VNC through the tunnel "+
    "without the need to specify anything else.", false);

  public static BoolParameter extSSH
  = new BoolParameter("extSSH",
    "By default, SSH tunneling uses the embedded JSch client "+
    "for tunnel creation. This option causes the client to "+
    "invoke an external SSH client application for all tunneling "+
    "operations. By default, \"/usr/bin/ssh\" is used, however "+
    "the path to the external application may be specified using "+
    "the -SSHClient option.", false);

  public static StringParameter extSSHClient
  = new StringParameter("extSSHClient",
    "Specifies the path to an external SSH client application "+
    "that is to be used for tunneling operations when the -extSSH "+
    "option is in effect.", "/usr/bin/ssh");

  public static StringParameter extSSHArgs
  = new StringParameter("extSSHArgs",
    "Specifies the arguments string or command template to be used "+
    "by the external SSH client application when the -extSSH option "+
    "is in effect. The string will be processed according to the same "+
    "pattern substitution rules as the VNC_TUNNEL_CMD and VNC_VIA_CMD "+
    "system properties, and can be used to override those in a more "+
    "command-line friendly way. If not specified, then the appropriate "+
    "VNC_TUNNEL_CMD or VNC_VIA_CMD command template will be used.", "");

  public static StringParameter sshConfig
  = new StringParameter("SSHConfig",
    "Specifies the path to an OpenSSH configuration file that to "+
    "be parsed by the embedded JSch SSH client during tunneling "+
    "operations.", FileUtils.getHomeDir()+".ssh/config");

  public static StringParameter sshKey
  = new StringParameter("SSHKey",
    "When using the Via or Tunnel options with the embedded SSH client, "+
    "this parameter specifies the text of the SSH private key to use when "+
    "authenticating with the SSH server. You can use \\n within the string "+
    "to specify a new line.", "");

  public static StringParameter sshKeyFile
  = new StringParameter("SSHKeyFile",
    "When using the Via or Tunnel options with the embedded SSH client, "+
    "this parameter specifies a file that contains an SSH private key "+
    "(or keys) to use when authenticating with the SSH server. If not "+
    "specified, ~/.ssh/id_dsa or ~/.ssh/id_rsa will be used (if they exist). "+
    "Otherwise, the client will fallback to prompting for an SSH password.",
    "");

  public static StringParameter sshKeyPass
  = new StringParameter("SSHKeyPass",
    "When using the Via or Tunnel options with the embedded SSH client, "+
    "this parameter specifies the passphrase for the SSH key.", "");

  public static BoolParameter customCompressLevel
  = new BoolParameter("CustomCompressLevel",
    "Use custom compression level. Default if CompressLevel is specified.",
    false);

  public static IntParameter compressLevel
  = new IntParameter("CompressLevel",
    "Use specified lossless compression level. 0 = Low, 9 = High. Default is 2.",
    2);

  public static BoolParameter noJpeg
  = new BoolParameter("NoJPEG",
    "Disable lossy JPEG compression in Tight encoding.",
    false);

  public static IntParameter qualityLevel
  = new IntParameter("QualityLevel",
    "JPEG quality level. 0 = Low, 9 = High",
    8);

  private static final String IDENTIFIER_STRING
  = "TigerVNC Configuration file Version 1.0";

  static VoidParameter[] parameterArray = {
    CSecurityTLS.X509CA,
    CSecurityTLS.X509CRL,
    SecurityClient.secTypes,
    dotWhenNoCursor,
    autoSelect,
    fullColor,
    lowColorLevel,
    preferredEncoding,
    customCompressLevel,
    compressLevel,
    noJpeg,
    qualityLevel,
    maximize,
    fullScreen,
    fullScreenAllMonitors,
    desktopSize,
    remoteResize,
    viewOnly,
    shared,
    acceptClipboard,
    sendClipboard,
    menuKey,
    noLionFS,
    sendLocalUsername,
    maxCutText,
    scalingFactor,
    acceptBell,
    via,
    tunnel,
    extSSH,
    extSSHClient,
    extSSHArgs,
    sshConfig,
    sshKeyFile,
  };


  static LogWriter vlog = new LogWriter("Parameters");

	public static void saveViewerParameters(String filename, String servername) {

	  // Write to the registry or a predefined file if no filename was specified.
    String filepath;
    if (filename == null || filename.isEmpty()) {
      saveToReg(servername);
      return;
      /*
      String homeDir = FileUtils.getVncHomeDir();
      if (homeDir == null)
        throw new Exception("Failed to read configuration file, "+
                            "can't obtain home directory path.");
      filepath = homeDir.concat("default.tigervnc");
      */
    } else {
      filepath = filename;
    }

	  /* Write parameters to file */
    File f = new File(filepath);
    if (f.exists() && !f.canWrite())
	    throw new Exception(String.format("Failed to write configuration file,"+
                                        "can't open %s", filepath)); 

    PrintWriter pw = null;
    try {
      pw = new PrintWriter(f, "UTF-8");
    } catch (java.lang.Exception e)	{
      throw new Exception(e.getMessage());
    }

    pw.println(IDENTIFIER_STRING);
    pw.println("");

	  if (servername != null && !servername.isEmpty()) {
	    pw.println(String.format("ServerName=%s\n", servername));
      updateConnHistory(servername);
    }

    for (int i = 0; i < parameterArray.length; i++) {
      if (parameterArray[i] instanceof StringParameter) {
        //if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName()))
          pw.println(String.format("%s=%s",parameterArray[i].getName(),
                                   parameterArray[i].getValueStr()));
      } else if (parameterArray[i] instanceof IntParameter) {
        //if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName()))
          pw.println(String.format("%s=%s",parameterArray[i].getName(),
                                   parameterArray[i].getValueStr()));
      } else if (parameterArray[i] instanceof BoolParameter) {
        //if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName()))
          pw.println(String.format("%s=%s",parameterArray[i].getName(),
                                   parameterArray[i].getValueStr()));
      } else {
        vlog.error(String.format("Unknown parameter type for parameter %s",
                   parameterArray[i].getName()));
      }
    }

    pw.flush();
    pw.close();
	}

  public static String loadViewerParameters(String filename) throws Exception {
    String servername = "";

    String filepath;
    if (filename == null) {

      return loadFromReg(); 

      /*
      String homeDir = FileUtils.getVncHomeDir();
      if (homeDir == null)
        throw new Exception("Failed to read configuration file, "+
                            "can't obtain home directory path.");
      filepath = homeDir.concat("default.tigervnc");
      */
    } else {
      filepath = filename;
    }

    /* Read parameters from file */
    File f = new File(filepath);
    if (!f.exists() || !f.canRead()) {
      if (filename == null || filename.isEmpty())
        return null;
      throw new Exception(String.format("Failed to read configuration file, can't open %s",
                          filepath)); 
    }

    String line = "";
    LineNumberReader reader;
    try {
      reader = new LineNumberReader(new FileReader(f));
    } catch (FileNotFoundException e) {
      throw new Exception(e.getMessage());
    }

    int lineNr = 0;
    while (line != null) {

      // Read the next line
      try {
        line = reader.readLine();
        lineNr = reader.getLineNumber();
        if (line == null)
          break;
      } catch (IOException e) {
        throw new Exception(String.format("Failed to read line %d in file %s: %s",
          lineNr, filepath, e.getMessage()));
      }

      // Make sure that the first line of the file has the file identifier string
      if(lineNr == 1) {
        if(line.equals(IDENTIFIER_STRING))
          continue;
        else
          throw new Exception(String.format("Configuration file %s is in an invalid format", filename));
      }

      // Skip empty lines and comments
      if (line.trim().isEmpty() || line.trim().startsWith("#"))
        continue;

      // Find the parameter value
      int idx = line.indexOf("=");
      if (idx == -1) {
        vlog.error(String.format("Failed to read line %d in file %s: %s",
                   lineNr, filename, "Invalid format"));
        continue;
      }
      String value = line.substring(idx+1).trim();
      boolean invalidParameterName = true; // Will be set to false below if 
                                           // the line contains a valid name.

      if (line.substring(0,idx).trim().equalsIgnoreCase("ServerName")) {
        if (value.length() > 256) {
          vlog.error(String.format("Failed to read line %d in file %s: %s",
                     lineNr, filepath, "Invalid format or too large value"));
          continue;
        }
        servername = value;
        invalidParameterName = false;
      } else {
        for (int i = 0; i < parameterArray.length; i++) {
          if (parameterArray[i] instanceof StringParameter) {
            if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName())) {
              if (value.length() > 256) {
                vlog.error(String.format("Failed to read line %d in file %s: %s",
                           lineNr, filepath, "Invalid format or too large value"));
                continue;
              }
              ((StringParameter)parameterArray[i]).setParam(value);
              invalidParameterName = false;
            }
          } else if (parameterArray[i] instanceof IntParameter) {
            if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName())) {
              ((IntParameter)parameterArray[i]).setParam(value);
              invalidParameterName = false;
            }
          } else if (parameterArray[i] instanceof BoolParameter) {
            if (line.substring(0,idx).trim().equalsIgnoreCase(parameterArray[i].getName())) {
              ((BoolParameter)parameterArray[i]).setParam(value);
              invalidParameterName = false;
            }
          } else {
            vlog.error(String.format("Unknown parameter type for parameter %s",
                       parameterArray[i].getName()));

          }
        }
      }

      if (invalidParameterName)
        vlog.info(String.format("Unknown parameter %s on line %d in file %s",
                  line, lineNr, filepath));
    }
    try {
      reader.close();
    } catch (IOException e) {
      vlog.info(e.getMessage());
    } finally {
      try {
        if (reader != null)
          reader.close();
        } catch (IOException e) { }
    }

    return servername;
  }

  public static void saveToReg(String servername) {
    String hKey = "global";

    if (servername != null && !servername.isEmpty()) {
      UserPreferences.set(hKey, "ServerName", servername);
      updateConnHistory(servername);
    }

    for (int i = 0; i < parameterArray.length; i++) {
      if (parameterArray[i] instanceof StringParameter) {
        UserPreferences.set(hKey, parameterArray[i].getName(),
                                  parameterArray[i].getValueStr());
      } else if (parameterArray[i] instanceof IntParameter) {
        UserPreferences.set(hKey, parameterArray[i].getName(),
                            ((IntParameter)parameterArray[i]).getValue());
      } else if (parameterArray[i] instanceof BoolParameter) {
        UserPreferences.set(hKey, parameterArray[i].getName(),
                            ((BoolParameter)parameterArray[i]).getValue());
      } else {
        vlog.error(String.format("Unknown parameter type for parameter %s",
                   parameterArray[i].getName()));
      }
    }

    UserPreferences.save(hKey);
  }

  public static String loadFromReg() {

    String hKey = "global";

    String servername = UserPreferences.get(hKey, "ServerName");
    if (servername == null)
      servername = "";

    for (int i = 0; i < parameterArray.length; i++) {
      if (parameterArray[i] instanceof StringParameter) {
        if (UserPreferences.get(hKey, parameterArray[i].getName()) != null) {
          String stringValue =
            UserPreferences.get(hKey, parameterArray[i].getName());
          ((StringParameter)parameterArray[i]).setParam(stringValue);
        }
      } else if (parameterArray[i] instanceof IntParameter) {
        if (UserPreferences.get(hKey, parameterArray[i].getName()) != null) {
          int intValue =
            UserPreferences.getInt(hKey, parameterArray[i].getName());
          ((IntParameter)parameterArray[i]).setParam(intValue);
        }
      } else if (parameterArray[i] instanceof BoolParameter) {
        if (UserPreferences.get(hKey, parameterArray[i].getName()) != null) {
          boolean booleanValue =
            UserPreferences.getBool(hKey, parameterArray[i].getName());
          ((BoolParameter)parameterArray[i]).setParam(booleanValue);
        }
      } else {
        vlog.error(String.format("Unknown parameter type for parameter %s",
                   parameterArray[i].getName()));
      }
    }

    return servername;
  }

  private static void updateConnHistory(String serverName) {
    String hKey = "ServerDialog";
    if (serverName != null && !serverName.isEmpty()) {
      String valueStr = UserPreferences.get(hKey, "history");
      String t = (valueStr == null) ? "" : valueStr;
      StringTokenizer st = new StringTokenizer(t, ",");
      StringBuffer sb = new StringBuffer().append(serverName);
      while (st.hasMoreTokens()) {
        String str = st.nextToken();
        if (!str.equals(serverName) && !str.equals(""))
          sb.append(',').append(str);
      }
      UserPreferences.set(hKey, "history", sb.toString());
      UserPreferences.save(hKey);
    }
  }

}
