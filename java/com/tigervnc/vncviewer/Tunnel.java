/*
 *  Copyright (C) 2012-2016 All Rights Reserved.
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

public class Tunnel {

  private final static String DEFAULT_TUNNEL_TEMPLATE
    = "-f -L %L:localhost:%R %H sleep 20";
  private final static String DEFAULT_VIA_TEMPLATE
    = "-f -L %L:%H:%R %G sleep 20";

  public static void createTunnel(CConn cc, int localPort) throws Exception {
    int remotePort;
    String gatewayHost;
    String remoteHost;

    remotePort = cc.getServerPort();
    if (cc.viewer.tunnel.getValue()) {
      gatewayHost = cc.getServerName();
      remoteHost = "localhost";
    } else {
      gatewayHost = getSshHost(cc);
      remoteHost = cc.getServerName();
    }

    String pattern = cc.viewer.extSSHArgs.getValue();
    if (pattern == null) {
      if (cc.viewer.tunnel.getValue())
        pattern = System.getProperty("VNC_TUNNEL_CMD");
      else
        pattern = System.getProperty("VNC_VIA_CMD");
    }

    if (cc.viewer.extSSH.getValue() || 
        (pattern != null && pattern.length() > 0)) {
      createTunnelExt(gatewayHost, remoteHost, remotePort, localPort, pattern, cc);
    } else {
      createTunnelJSch(gatewayHost, remoteHost, remotePort, localPort, cc);
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

  public static String getSshHost(CConn cc) {
    String sshHost = cc.viewer.via.getValue();
    if (sshHost == null)
      return cc.getServerName();
    int end = sshHost.indexOf(":");
    if (end < 0)
      end = sshHost.length();
    sshHost = sshHost.substring(sshHost.indexOf("@")+1, end);
    return sshHost;
  }

  public static String getSshUser(CConn cc) {
    String sshUser = (String)System.getProperties().get("user.name");
    String via = cc.viewer.via.getValue();
    if (via != null && via.indexOf("@") > 0)
      sshUser = via.substring(0, via.indexOf("@"));
    return sshUser;
  }

  public static int getSshPort(CConn cc) {
    String sshPort = "22";
    String via = cc.viewer.via.getValue();
    if (via != null && via.indexOf(":") > 0)
      sshPort = via.substring(via.indexOf(":")+1, via.length());
    return Integer.parseInt(sshPort);
  }

  public static String getSshKeyFile(CConn cc) {
    if (cc.viewer.sshKeyFile.getValue() != null)
      return cc.viewer.sshKeyFile.getValue();
    String[] ids = { "id_dsa", "id_rsa" };
    for (String id : ids) {
      File f = new File(FileUtils.getHomeDir()+".ssh/"+id);
      if (f.exists() && f.canRead())
        return(f.getAbsolutePath());
    }
    return null;
  }

  private static void createTunnelJSch(String gatewayHost, String remoteHost,
                                       int remotePort, int localPort,
                                       CConn cc) throws Exception {
    JSch.setLogger(new MyJSchLogger());
    JSch jsch=new JSch();

    try {
      // NOTE: jsch does not support all ciphers.  User may be
      //       prompted to accept host key authenticy even if
      //       the key is in the known_hosts file.
      File knownHosts = new File(FileUtils.getHomeDir()+".ssh/known_hosts");
      if (knownHosts.exists() && knownHosts.canRead())
  	    jsch.setKnownHosts(knownHosts.getAbsolutePath());
      ArrayList<File> privateKeys = new ArrayList<File>();
      String sshKeyFile = cc.options.sshKeyFile.getText();
      String sshKey = cc.viewer.sshKey.getValue();
      if (sshKey != null) {
        String sshKeyPass = cc.viewer.sshKeyPass.getValue();
        byte[] keyPass = null, key;
        if (sshKeyPass != null)
          keyPass = sshKeyPass.getBytes();
        sshKey = sshKey.replaceAll("\\\\n", "\n");
        key = sshKey.getBytes();
        jsch.addIdentity("TigerVNC", key, null, keyPass);
      } else if (!sshKeyFile.equals("")) {
        File f = new File(sshKeyFile);
        if (!f.exists() || !f.canRead())
          throw new Exception("Cannot access SSH key file "+ sshKeyFile);
        privateKeys.add(f);
      }
      for (Iterator<File> i = privateKeys.iterator(); i.hasNext();) {
        File privateKey = (File)i.next();
        if (privateKey.exists() && privateKey.canRead())
          if (cc.viewer.sshKeyPass.getValue() != null)
  	        jsch.addIdentity(privateKey.getAbsolutePath(),
                             cc.viewer.sshKeyPass.getValue());
          else
  	        jsch.addIdentity(privateKey.getAbsolutePath());
      }
  
      String user = getSshUser(cc);
      String label = new String("SSH Authentication");
      PasswdDialog dlg =
        new PasswdDialog(label, (user == null ? false : true), false);
      dlg.userEntry.setText(user != null ? user : "");
      File ssh_config = new File(cc.viewer.sshConfig.getValue());
      if (ssh_config.exists() && ssh_config.canRead()) {
        ConfigRepository repo =
          OpenSSHConfig.parse(ssh_config.getAbsolutePath());
        jsch.setConfigRepository(repo);
      }
      Session session=jsch.getSession(user, gatewayHost, getSshPort(cc));
      session.setUserInfo(dlg);
      // OpenSSHConfig doesn't recognize StrictHostKeyChecking
      if (session.getConfig("StrictHostKeyChecking") == null)
        session.setConfig("StrictHostKeyChecking", "ask");
      session.connect();
      session.setPortForwardingL(localPort, remoteHost, remotePort);
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage()); 
    }
  }

  private static class MyExtProcess implements Runnable {

    private String cmd = null;
    private Process pid = null;

    private static class MyProcessLogger extends Thread {
      private final BufferedReader err;
  
      public MyProcessLogger(Process p) {
        InputStreamReader reader = 
          new InputStreamReader(p.getErrorStream());
        err = new BufferedReader(reader);
      }
  
      @Override
      public void run() {
        try {
          while (true) {
            String msg = err.readLine();
            if (msg != null)
              vlog.info(msg);
          }
        } catch(java.io.IOException e) {
          vlog.info(e.getMessage());
        } finally {
          try {
            if (err != null)
              err.close();
          } catch (java.io.IOException e ) { }
        }
      }
    }

    private static class MyShutdownHook extends Thread {

      private Process proc = null;

      public MyShutdownHook(Process p) {
        proc = p;
      }
  
      @Override
      public void run() {
        try {
          proc.exitValue();
        } catch (IllegalThreadStateException e) {
          try {
            // wait for CConn to shutdown the socket
            Thread.sleep(500);
          } catch(InterruptedException ie) { }
          proc.destroy();
        }
      }
    }

    public MyExtProcess(String command) {
      cmd = command;  
    }

    public void run() {
      try {
        Runtime runtime = Runtime.getRuntime();
        pid = runtime.exec(cmd);
        runtime.addShutdownHook(new MyShutdownHook(pid));
        new MyProcessLogger(pid).start();
        pid.waitFor();
      } catch(InterruptedException e) {
        vlog.info(e.getMessage());
      } catch(java.io.IOException e) {
        vlog.info(e.getMessage());
      }
    }
  }

  private static void createTunnelExt(String gatewayHost, String remoteHost,
                                      int remotePort, int localPort,
                                      String pattern, CConn cc) throws Exception {
    if (pattern == null || pattern.length() < 1) {
      if (cc.viewer.tunnel.getValue())
        pattern = DEFAULT_TUNNEL_TEMPLATE;
      else
        pattern = DEFAULT_VIA_TEMPLATE;
    }
    String cmd = fillCmdPattern(pattern, gatewayHost, remoteHost,
                                remotePort, localPort, cc);
    try {
      Thread t = new Thread(new MyExtProcess(cmd));
      t.start();
      // wait for the ssh process to start
      Thread.sleep(1000);
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage());
    }
  }

  private static String fillCmdPattern(String pattern, String gatewayHost,
                                       String remoteHost, int remotePort,
                                       int localPort, CConn cc) {
    boolean H_found = false, G_found = false, R_found = false, L_found = false;
    boolean P_found = false;
    String cmd = cc.options.sshClient.getText() + " ";
    pattern.replaceAll("^\\s+", "");

    String user = getSshUser(cc);
    int sshPort = getSshPort(cc);
    gatewayHost = user + "@" + gatewayHost;

    for (int i = 0; i < pattern.length(); i++) {
      if (pattern.charAt(i) == '%') {
        switch (pattern.charAt(++i)) {
        case 'H':
          cmd += (cc.viewer.tunnel.getValue() ? gatewayHost : remoteHost);
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

    if (!cc.viewer.tunnel.getValue() && !G_found)
      throw new Exception("%G pattern absent in tunneling command template.");

    vlog.info("SSH command line: "+cmd);
    if (VncViewer.os.startsWith("windows"))
      cmd.replaceAll("\\\\", "\\\\\\\\");
    return cmd;
  }

  static LogWriter vlog = new LogWriter("Tunnel");
}
