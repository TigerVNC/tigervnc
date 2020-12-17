/*
 *  Copyright (C) 2012-2016 Brian P. Hinz. All Rights Reserved.
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

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.net.*;
import java.util.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;
import com.tigervnc.network.*;

import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.ConfigRepository;
import com.jcraft.jsch.Logger;
import com.jcraft.jsch.OpenSSHConfig;
import com.jcraft.jsch.Session;

import static com.tigervnc.vncviewer.Parameters.*;

public class Tunnel {

  private final static String DEFAULT_TUNNEL_TEMPLATE
    = "-f -L %L:localhost:%R %H sleep 20";
  private final static String DEFAULT_VIA_TEMPLATE
    = "-f -L %L:%H:%R %G sleep 20";

  public static void createTunnel(String gatewayHost,
                                  String remoteHost,
                                  int remotePort,
                                  int localPort) throws Exception {
    if (extSSH.getValue()) {
      String pattern = extSSHArgs.getValueStr();
      if (pattern == null || pattern.isEmpty()) {
        if (tunnel.getValue() && via.getValue().isEmpty()) {
          pattern = System.getProperty("VNC_TUNNEL_CMD");
          if (pattern == null || pattern.isEmpty())
            pattern = DEFAULT_TUNNEL_TEMPLATE;
        } else {
          pattern = System.getProperty("VNC_VIA_CMD");
          if (pattern == null || pattern.isEmpty())
            pattern = DEFAULT_VIA_TEMPLATE;
        }
      }
      createTunnelExt(gatewayHost, remoteHost, remotePort, localPort, pattern);
    } else {
      createTunnelJSch(gatewayHost, remoteHost, remotePort, localPort);
    }
  }

  private static class MyJSchLogger implements Logger {
    public boolean isEnabled(int level){
      return true;
    }

    public void log(int level, String msg){
      switch (level) {
      case Logger.INFO:
        vlog.info(msg);
        break;
      case Logger.ERROR:
        vlog.error(msg);
        break;
      default:
        vlog.debug(msg);
      }
    }
  }

  public static String getSshHost() {
    String sshHost = via.getValue();
    if (!sshHost.isEmpty()) {
      int end = sshHost.indexOf(":");
      if (end < 0)
        end = sshHost.length();
      sshHost = sshHost.substring(sshHost.indexOf("@")+1, end);
    }
    return sshHost;
  }

  public static String getSshUser() {
    String sshUser = (String)System.getProperties().get("user.name");
    String viaStr = via.getValue();
    if (!viaStr.isEmpty() && viaStr.indexOf("@") > 0)
      sshUser = viaStr.substring(0, viaStr.indexOf("@"));
    return sshUser;
  }

  public static int getSshPort() {
    String sshPort = "22";
    String viaStr = via.getValue();
    if (!viaStr.isEmpty() && viaStr.indexOf(":") > 0)
      sshPort = viaStr.substring(viaStr.indexOf(":")+1, viaStr.length());
    return Integer.parseInt(sshPort);
  }

  public static String getSshKeyFile() {
    if (!sshKeyFile.getValue().isEmpty())
      return sshKeyFile.getValue();
    String[] ids = { "id_dsa", "id_rsa" };
    for (String id : ids) {
      File f = new File(FileUtils.getHomeDir()+".ssh"+
                        FileUtils.getFileSeparator()+id);
      if (f.exists() && f.canRead())
        return(f.getAbsolutePath());
    }
    return "";
  }

  public static String getSshKey() {
    if (!sshKey.getValue().isEmpty())
      return sshKeyFile.getValue().replaceAll("\\\\n", "\n");
    return "";
  }

  private static void createTunnelJSch(String gatewayHost, String remoteHost,
                                       int remotePort, int localPort) throws Exception {
    JSch.setLogger(new MyJSchLogger());
    JSch jsch=new JSch();

    try {
      // NOTE: jsch does not support all ciphers.  User may be
      //       prompted to accept host key authenticy even if
      //       the key is in the known_hosts file.
      File knownHosts = new File(FileUtils.getHomeDir()+".ssh"+
                                 FileUtils.getFileSeparator()+"known_hosts");
      if (knownHosts.exists() && knownHosts.canRead())
  	    jsch.setKnownHosts(knownHosts.getAbsolutePath());
      ArrayList<File> privateKeys = new ArrayList<File>();
      if (!getSshKey().isEmpty()) {
        byte[] keyPass = null, key;
        if (!sshKeyPass.getValue().isEmpty())
          keyPass = sshKeyPass.getValue().getBytes();
        jsch.addIdentity("TigerVNC", getSshKey().getBytes(), null, keyPass);
      } else if (!getSshKeyFile().isEmpty()) {
        File f = new File(getSshKeyFile());
        if (!f.exists() || !f.canRead())
          throw new Exception("Cannot access SSH key file "+getSshKeyFile());
        privateKeys.add(f);
      }
      for (Iterator<File> i = privateKeys.iterator(); i.hasNext();) {
        File privateKey = (File)i.next();
        if (privateKey.exists() && privateKey.canRead())
          if (!sshKeyPass.getValue().isEmpty())
  	        jsch.addIdentity(privateKey.getAbsolutePath(),
                             sshKeyPass.getValue());
          else
  	        jsch.addIdentity(privateKey.getAbsolutePath());
      }

      String user = getSshUser();
      String label = new String("SSH Authentication");
      PasswdDialog dlg =
        new PasswdDialog(label, (user == null ? false : true), false);
      dlg.userEntry.setText(user != null ? user : "");
      File ssh_config = new File(sshConfig.getValue());
      if (ssh_config.exists() && ssh_config.canRead()) {
        ConfigRepository repo =
          OpenSSHConfig.parse(ssh_config.getAbsolutePath());
        jsch.setConfigRepository(repo);
      }
      Session session=jsch.getSession(user, gatewayHost, getSshPort());
      session.setUserInfo(dlg);
      // OpenSSHConfig doesn't recognize StrictHostKeyChecking
      if (session.getConfig("StrictHostKeyChecking") == null)
        session.setConfig("StrictHostKeyChecking", "ask");
      session.connect();
      if (gatewayHost.equals(remoteHost))
        session.setPortForwardingL(localPort, new String("localhost"), remotePort);
      else
        session.setPortForwardingL(localPort, remoteHost, remotePort);
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage()); 
    }
  }

  private static void createTunnelExt(String gatewayHost, String remoteHost,
                                      int remotePort, int localPort,
                                      String pattern) throws Exception {
    String cmd = fillCmdPattern(pattern, gatewayHost, remoteHost,
                                remotePort, localPort);
    try {
      Thread t = new Thread(new ExtProcess(cmd, vlog, true));
      t.start();
      // try for up to 5s
      long start = System.currentTimeMillis();
      while (System.currentTimeMillis() - start < 5000) {
        if (isTunnelReady(localPort))
          return;
        else
          Thread.sleep(100);
      }
      throw new Exception("SSH Tunnel not ready");
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage());
    }
  }

  private static boolean isTunnelReady(int localPort) throws Exception {
      // test the local forwarding socket to make
      // sure the tunnel is up before connecting
      SocketAddress sockAddr =
        new InetSocketAddress("localhost", localPort);
      java.net.Socket socket = new java.net.Socket();
      boolean ready = false;
      try {
        socket.connect(sockAddr, 1000);
        ready = socket.isConnected();
        socket.close();
      } catch (IOException e) {
        // expected until tunnel is up
      } catch (java.lang.Exception e) {
        throw new Exception(e.getMessage());
      }
      return ready;
  }

  private static String fillCmdPattern(String pattern, String gatewayHost,
                                       String remoteHost, int remotePort,
                                       int localPort) {
    boolean H_found = false, G_found = false, R_found = false, L_found = false;
    boolean P_found = false;
    String cmd = extSSHClient.getValue() + " ";
    pattern.replaceAll("^\\s+", "");

    String user = getSshUser();
    int sshPort = getSshPort();
    gatewayHost = user + "@" + gatewayHost;

    for (int i = 0; i < pattern.length(); i++) {
      if (pattern.charAt(i) == '%') {
        switch (pattern.charAt(++i)) {
        case 'H':
          cmd += remoteHost;
  	      H_found = true;
          continue;
        case 'G':
          cmd += gatewayHost;
  	      G_found = true;
  	      continue;
        case 'R':
          cmd += remotePort;
  	      R_found = true;
  	      continue;
        case 'L':
          cmd += localPort;
  	      L_found = true;
  	      continue;
        case 'P':
          cmd += sshPort;
  	      P_found = true;
  	      continue;
        }
      }
      cmd += pattern.charAt(i);
    }

    if (pattern.length() > 1024)
      throw new Exception("Tunneling command is too long.");

    if (!H_found || !R_found || !L_found)
      throw new Exception("%H, %R or %L absent in tunneling command template.");

    if (!tunnel.getValue() && !G_found)
      throw new Exception("%G pattern absent in tunneling command template.");

    vlog.info("SSH command line: "+cmd);
    if (VncViewer.os.startsWith("windows"))
      cmd.replaceAll("\\\\", "\\\\\\\\");
    return cmd;
  }

  static LogWriter vlog = new LogWriter("Tunnel");
}
