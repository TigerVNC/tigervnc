/*
 *  Copyright (C) 2012 Brian P. Hinz.  All Rights Reserved.
 *  Copyright (C) 2000 Const Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 *  USA.
 */

/*
 * tunnel.java - SSH tunneling support
 */

package com.tigervnc.vncviewer;

import java.io.File;
import java.lang.Character;
import java.util.ArrayList;
import java.util.Iterator;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.network.*;

import com.jcraft.jsch.JSch;
import com.jcraft.jsch.Session;

public class tunnel 
{
  private final static Integer SERVER_PORT_OFFSET = 5900;;
  private final static String DEFAULT_SSH_CMD = "/usr/bin/ssh";
  private final static String DEFAULT_TUNNEL_CMD  
    = DEFAULT_SSH_CMD+" -f -L %L:localhost:%R %H sleep 20";
  private final static String DEFAULT_VIA_CMD  
    = DEFAULT_SSH_CMD+" -f -L %L:%H:%R %G sleep 20";

  private final static int H = 17;
  private final static int G = 16;
  private final static int R = 27;
  private final static int L = 21;
  
  /* True if there was -tunnel or -via option in the command line. */
  private static boolean tunnelSpecified = false;
  
  /* True if it was -tunnel, not -via option. */
  private static boolean tunnelOption = false;
  
  /* "Hostname:display" pair in the command line will be substituted
     by this fake argument when tunneling is used. */
  private static String lastArgv;

  private static String tunnelEndpoint;
  
  public static Boolean
  createTunnel(int pargc, String[] argv, int tunnelArgIndex)
  {
    char[] pattern;
    char[] cmd = new char[1024];
    int[] localPort = new int[1];
    int[] remotePort = new int[1];
    char[] localPortStr = new char[8];
    char[] remotePortStr = new char[8];
    StringBuilder gatewayHost = new StringBuilder("");
    StringBuilder remoteHost = new StringBuilder("localhost");
  
    tunnelSpecified = true;
    if (argv[tunnelArgIndex].equalsIgnoreCase("-tunnel"))
      tunnelOption = true;
  
    pattern = getCmdPattern();
    if (pattern == null)
      return false;
  
    localPort[0] = TcpSocket.findFreeTcpPort();
    if (localPort[0] == 0)
      return false;
  
    if (tunnelOption) {
      processTunnelArgs(remoteHost, remotePort, localPort,
  		      pargc, argv, tunnelArgIndex);
    } else {
      processViaArgs(gatewayHost, remoteHost, remotePort, localPort,
  		   pargc, argv, tunnelArgIndex);
    }
  
    localPortStr = Integer.toString(localPort[0]).toCharArray();
    remotePortStr = Integer.toString(remotePort[0]).toCharArray();
  
    if (!fillCmdPattern(cmd, pattern, gatewayHost.toString().toCharArray(), 
		remoteHost.toString().toCharArray(), remotePortStr, localPortStr))
      return false;
  
    if (!runCommand(new String(cmd)))
      return false;
  
    return true;
  }
  
  private static void
  processTunnelArgs(StringBuilder remoteHost, int[] remotePort, 
                    int[] localPort, int pargc, String[] argv, 
                    int tunnelArgIndex)
  {
    String pdisplay;
  
    if (tunnelArgIndex >= pargc - 1)
      VncViewer.usage();
  
    pdisplay = argv[pargc - 1].split(":")[1];
    if (pdisplay == null || pdisplay == argv[pargc - 1])
      VncViewer.usage();
  
    if (pdisplay.matches("/[^0-9]/"))
      VncViewer.usage();
  
    remotePort[0] = Integer.parseInt(pdisplay);
    if (remotePort[0] < 100)
      remotePort[0] = remotePort[0] + SERVER_PORT_OFFSET;
  
    lastArgv = new String("localhost::"+localPort[0]);
  
    remoteHost.setLength(0);
    remoteHost.insert(0, argv[pargc - 1].split(":")[0]);
    argv[pargc - 1] = lastArgv;
  
    //removeArgs(pargc, argv, tunnelArgIndex, 1);
  }
  
  private static void
  processViaArgs(StringBuilder gatewayHost, StringBuilder remoteHost,
  	       int[] remotePort, int[] localPort,
  	       int pargc, String[] argv, int tunnelArgIndex)
  {
    String colonPos;
    int len, portOffset;
    int disp;
  
    if (tunnelArgIndex >= pargc - 2)
      VncViewer.usage();
  
    colonPos = argv[pargc - 1].split(":", 2)[1];
    if (colonPos == null) {
      /* No colon -- use default port number */
      remotePort[0] = SERVER_PORT_OFFSET;
    } else {
      len = colonPos.length();
      portOffset = SERVER_PORT_OFFSET;
      if (colonPos.startsWith(":")) {
        /* Two colons -- interpret as a port number */
        colonPos.replaceFirst(":", "");
        len--;
        portOffset = 0;
      }
      if (len == 0 || colonPos.matches("/[^0-9]/")) {
        VncViewer.usage();
      }
      disp = Integer.parseInt(colonPos);
      if (portOffset != 0 && disp >= 100)
        portOffset = 0;
      remotePort[0] = disp + portOffset;
    }
  
    lastArgv = "localhost::"+localPort[0];
  
    gatewayHost.setLength(0);
    gatewayHost.insert(0, argv[tunnelArgIndex + 1]);
  
    if (!argv[pargc - 1].split(":", 2)[0].equals("")) {
      remoteHost.setLength(0);
      remoteHost.insert(0, argv[pargc - 1].split(":", 2)[0]);
    }
  
    argv[pargc - 1] = lastArgv;
  
    //removeArgs(pargc, argv, tunnelArgIndex, 2);
  }
  
  private static char[]
  getCmdPattern()
  {
    String pattern = "";
  
    try {
      if (tunnelOption) {
        pattern = System.getProperty("com.tigervnc.VNC_TUNNEL_CMD");
      } else {
        pattern = System.getProperty("com.tigervnc.VNC_VIA_CMD");
      }
    } catch (java.lang.Exception e) { 
      vlog.info(e.toString());
    }
    if (pattern == null || pattern.equals(""))
      pattern = (tunnelOption) ? DEFAULT_TUNNEL_CMD : DEFAULT_VIA_CMD;
  
    return pattern.toCharArray();
  }
  
  /* Note: in fillCmdPattern() result points to a 1024-byte buffer */
  
  private static boolean
  fillCmdPattern(char[] result, char[] pattern,
  	       char[] gatewayHost, char[] remoteHost,
  	       char[] remotePort, char[] localPort)
  {
    int i, j;
    boolean H_found = false, G_found = false, R_found = false, L_found = false;

    for (i=0, j=0; i < pattern.length && j<1023; i++, j++) {
      if (pattern[i] == '%') {
        switch (pattern[++i]) {
        case 'H':
  	System.arraycopy(remoteHost, 0, result, j, remoteHost.length);
  	j += remoteHost.length;
  	H_found = true;
        tunnelEndpoint = new String(remoteHost);
  	continue;
        case 'G':
  	System.arraycopy(gatewayHost, 0, result, j, gatewayHost.length);
  	j += gatewayHost.length;
  	G_found = true;
        tunnelEndpoint = new String(gatewayHost);
  	continue;
        case 'R':
  	System.arraycopy(remotePort, 0, result, j, remotePort.length);
  	j += remotePort.length;
  	R_found = true;
  	continue;
        case 'L':
  	System.arraycopy(localPort, 0, result, j, localPort.length);
  	j += localPort.length;
  	L_found = true;
  	continue;
        case '\0':
  	i--;
  	continue;
        }
      }
      result[j] = pattern[i];
    }
  
    if (pattern.length > 1024) {
      vlog.error("Tunneling command is too long.");
      return false;
    }
  
    if (!H_found || !R_found || !L_found) {
      vlog.error("%H, %R or %L absent in tunneling command.");
      return false;
    }
    if (!tunnelOption && !G_found) {
      vlog.error("%G pattern absent in tunneling command.");
      return false;
    }
  
    return true;
  }
  
  private static Boolean
  runCommand(String cmd)
  {
    try{
      JSch jsch=new JSch();
      String homeDir = new String("");
      try {
        homeDir = System.getProperty("user.home");
      } catch(java.security.AccessControlException e) {
        System.out.println("Cannot access user.home system property");
      }
      // NOTE: jsch does not support all ciphers.  User may be
      //       prompted to accept host key authenticy even if
      //       the key is in the known_hosts file.
      File knownHosts = new File(homeDir+"/.ssh/known_hosts");
      if (knownHosts.exists() && knownHosts.canRead())
	      jsch.setKnownHosts(knownHosts.getAbsolutePath());
      ArrayList<File> privateKeys = new ArrayList<File>();
      privateKeys.add(new File(homeDir+"/.ssh/id_rsa"));
      privateKeys.add(new File(homeDir+"/.ssh/id_dsa"));
      for (Iterator i = privateKeys.iterator(); i.hasNext();) {
        File privateKey = (File)i.next();
        if (privateKey.exists() && privateKey.canRead())
	        jsch.addIdentity(privateKey.getAbsolutePath());
      }
      // username and passphrase will be given via UserInfo interface.
      PasswdDialog dlg = new PasswdDialog(new String("SSH Authentication"), false, false);
      dlg.userEntry.setText((String)System.getProperties().get("user.name"));

      Session session=jsch.getSession(dlg.userEntry.getText(), tunnelEndpoint, 22);
      session.setUserInfo(dlg);
      session.connect();

      String[] tokens = cmd.split("\\s");
      for (int i = 0; i < tokens.length; i++) {
        if (tokens[i].equals("-L")) {
          String[] par = tokens[++i].split(":");
          int localPort = Integer.parseInt(par[0].trim());
          String remoteHost = par[1].trim();
          int remotePort = Integer.parseInt(par[2].trim());
          session.setPortForwardingL(localPort, remoteHost, remotePort);
        } else if (tokens[i].equals("-R")) {
          String[] par = tokens[++i].split(":");
          int remotePort = Integer.parseInt(par[0].trim());
          String localHost = par[1].trim();
          int localPort = Integer.parseInt(par[2].trim());
          session.setPortForwardingR(remotePort, localHost, localPort);
        }
      }
    } catch (java.lang.Exception e) {
      System.out.println(" Tunneling command failed: "+e.toString());
      return false;
    }
    return true;
  }
  
  static LogWriter vlog = new LogWriter("tunnel");
}
