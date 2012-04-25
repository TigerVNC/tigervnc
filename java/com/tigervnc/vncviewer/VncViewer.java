/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011-2012 Brian P. Hinz
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
// VncViewer - the VNC viewer applet.  It can also be run from the
// command-line, when it behaves as much as possibly like the windows and unix
// viewers.
//
// Unfortunately, because of the way Java classes are loaded on demand, only
// configuration parameters defined in this file can be set from the command
// line or in applet parameters.

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Image;
import java.io.InputStream;
import java.io.IOException;
import java.io.File;
import java.lang.Character;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import java.util.ArrayList;
import java.util.Iterator;
import javax.swing.*;
import javax.swing.plaf.FontUIResource;
import javax.swing.UIManager.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.network.*;

import com.jcraft.jsch.JSch;
import com.jcraft.jsch.Session;

public class VncViewer extends java.applet.Applet implements Runnable
{
  public static final String about1 = "TigerVNC Viewer for Java";
  public static final String about2 = "Copyright (C) 1998-2011 "+
                                      "TigerVNC Team and many others (see README)";
  public static final String about3 = "Visit http://www.tigervnc.org "+
                                      "for information on TigerVNC.";
  public static String version = null;
  public static String build = null;

  public static void setLookAndFeel() {
    try {
      // Use Nimbus LookAndFeel if it's available, otherwise fallback
      // to the native laf, or Metal if no native laf is available.
      String laf = System.getProperty("swing.defaultlaf");
      if (laf == null) {
        LookAndFeelInfo[] installedLafs = UIManager.getInstalledLookAndFeels();
        for (int i = 0; i < installedLafs.length; i++) {
          if (installedLafs[i].getName().equals("Nimbus"))
            laf = installedLafs[i].getClassName();
        }
        if (laf == null)
          UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
      }
      UIManager.setLookAndFeel(laf);
      UIManager.put("TitledBorder.titleColor",Color.blue);
      if (UIManager.getLookAndFeel().getName().equals("Metal")) {
        UIManager.put("swing.boldMetal", Boolean.FALSE);
        FontUIResource f = new FontUIResource("SansSerif", Font.PLAIN, 11);
        java.util.Enumeration keys = UIManager.getDefaults().keys();
        while (keys.hasMoreElements()) {
          Object key = keys.nextElement();
          Object value = UIManager.get(key);
          if (value instanceof javax.swing.plaf.FontUIResource)
            UIManager.put(key, f);
        }
      } else if (UIManager.getLookAndFeel().getName().equals("Nimbus")) {
        FontUIResource f;
        String os = System.getProperty("os.name");
        if (os.startsWith("Windows")) {
          f = new FontUIResource("Verdana", 0, 11);
        } else {
          f = new FontUIResource("DejaVu Sans", 0, 11);
        }
      	UIManager.put("TitledBorder.font", f);
      }
    } catch (java.lang.Exception e) { 
      vlog.info(e.toString());
    }
  }

  public static void main(String[] argv) {
    setLookAndFeel();
    VncViewer viewer = new VncViewer(argv);
    viewer.firstApplet = true;
    viewer.stop = false;
    viewer.start();
  }

  
  public VncViewer(String[] argv) {
    applet = false;
    
    // Override defaults with command-line options
    for (int i = 0; i < argv.length; i++) {
      if (argv[i].equalsIgnoreCase("-log")) {
        if (++i >= argv.length) usage();
        System.err.println("Log setting: "+argv[i]);
        LogWriter.setLogParams(argv[i]);
        continue;
      }

      if (Configuration.setParam(argv[i]))
        continue;

      if (argv[i].charAt(0) == '-') {
        if (i+1 < argv.length) {
          if (Configuration.setParam(argv[i].substring(1), argv[i+1])) {
            i++;
            continue;
          }
        }
        usage();
      }

      if (vncServerName.getValue() != null)
        usage();
      vncServerName.setParam(argv[i]);
    }
  }

  public static void usage() {
    String usage = ("\nusage: vncviewer [options/parameters] "+
                    "[host:displayNum] [options/parameters]\n"+
                    "       vncviewer [options/parameters] -listen [port] "+
                    "[options/parameters]\n"+
                    "\n"+
                    "Options:\n"+
                    "  -log <level>    configure logging level\n"+
                    "\n"+
                    "Parameters can be turned on with -<param> or off with "+
                    "-<param>=0\n"+
                    "Parameters which take a value can be specified as "+
                    "-<param> <value>\n"+
                    "Other valid forms are <param>=<value> -<param>=<value> "+
                    "--<param>=<value>\n"+
                    "Parameter names are case-insensitive.  The parameters "+
                    "are:\n\n"+
                    Configuration.listParams());
    System.err.print(usage);
    System.exit(1);
  }

  /* Tunnelling support. */
  private void interpretViaParam(StringParameter gatewayHost,
    StringParameter remoteHost, IntParameter remotePort, 
    StringParameter vncServerName, IntParameter localPort)
  {
    final int SERVER_PORT_OFFSET = 5900;;
    int pos = vncServerName.getValueStr().indexOf(":");
    if (pos == -1)
      remotePort.setParam(""+SERVER_PORT_OFFSET+"");
    else {
      int portOffset = SERVER_PORT_OFFSET;
      int len;
      pos++;
      len =  vncServerName.getValueStr().substring(pos).length();
      if (vncServerName.getValueStr().substring(pos, pos).equals(":")) {
        /* Two colons is an absolute port number, not an offset. */
        pos++;
        len--;
        portOffset = 0;
      }
      try {
        if (len <= 0 || !vncServerName.getValueStr().substring(pos).matches("[0-9]+"))
          usage();
        portOffset += Integer.parseInt(vncServerName.getValueStr().substring(pos));
        remotePort.setParam(""+portOffset+"");
      } catch (java.lang.NumberFormatException e) {
        usage();
      }
    }
  
    if (vncServerName != null)
      remoteHost.setParam(vncServerName.getValueStr().split(":")[0]);
  
    gatewayHost.setParam(via.getValueStr());
    vncServerName.setParam("localhost::"+localPort.getValue());
  }

  private void
  createTunnel(String gatewayHost, String remoteHost,
          int remotePort, int localPort)
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
      Session session=jsch.getSession(dlg.userEntry.getText(), gatewayHost, 22);
      session.setUserInfo(dlg);
      session.connect();

      session.setPortForwardingL(localPort, remoteHost, remotePort);
    } catch (java.lang.Exception e) {
      System.out.println(e);
    }
  }

  public VncViewer() {
    applet = true;
    firstApplet = true;
  }

  public static void newViewer(VncViewer oldViewer, Socket sock) {
    VncViewer viewer = new VncViewer();
    viewer.applet = oldViewer.applet;
    viewer.firstApplet = false;
    viewer.sock = sock;
    viewer.start();
  }

  public static void newViewer(VncViewer oldViewer) {
    newViewer(oldViewer, null);
  }

  public void init() {
    vlog.debug("init called");
    setLookAndFeel();
    setBackground(Color.white);
    ClassLoader cl = this.getClass().getClassLoader();
    ImageIcon icon = new ImageIcon(cl.getResource("com/tigervnc/vncviewer/tigervnc.png"));
    logo = icon.getImage();
  }

  public void start() {
    vlog.debug("start called");
    if (version == null || build == null) {
      ClassLoader cl = this.getClass().getClassLoader();
      InputStream stream = cl.getResourceAsStream("com/tigervnc/vncviewer/timestamp");
      try {
        Manifest manifest = new Manifest(stream);
        Attributes attributes = manifest.getMainAttributes();
        version = attributes.getValue("Version");
        build = attributes.getValue("Build");
      } catch (java.io.IOException e) { }
    }
    nViewers++;
    if (applet && firstApplet) {
      alwaysShowServerDialog.setParam(true);
      Configuration.readAppletParams(this);
      String host = getCodeBase().getHost();
      if (vncServerName.getValue() == null && vncServerPort.getValue() != 0) {
        int port = vncServerPort.getValue();
        vncServerName.setParam(host + ((port >= 5900 && port <= 5999)
                                       ? (":"+(port-5900))
                                       : ("::"+port)));
      }
    }
    thread = new Thread(this);
    thread.start();
  }

  public void stop() {
    stop = true;
  }

  public void paint(Graphics g) {
    g.drawImage(logo, 0, 0, this);
    int h = logo.getHeight(this)+20;
    g.drawString(about1+" v"+version+" ("+build+")", 0, h);
    h += g.getFontMetrics().getHeight();
    g.drawString(about2, 0, h);
    h += g.getFontMetrics().getHeight();
    g.drawString(about3, 0, h);
  }

  public void run() {
    CConn cc = null;

    /* Tunnelling support. */
    if (via.getValueStr() != null) {
      StringParameter gatewayHost = new StringParameter("", "", "");
      StringParameter remoteHost = new StringParameter("", "", "localhost");
      IntParameter localPort = 
        new IntParameter("", "", TcpSocket.findFreeTcpPort());
      IntParameter remotePort = new IntParameter("", "", 5900);
      if (vncServerName.getValueStr() == null)
        usage();
      interpretViaParam(gatewayHost, remoteHost, remotePort, 
        vncServerName, localPort);
      createTunnel(gatewayHost.getValueStr(), remoteHost.getValueStr(), 
        remotePort.getValue(), localPort.getValue());
    }

    if (listenMode.getValue()) {
      int port = 5500;

      if (vncServerName.getValue() != null && 
          Character.isDigit(vncServerName.getValue().charAt(0)))
        port = Integer.parseInt(vncServerName.getValue());

      TcpListener listener = null;
      try {
        listener = new TcpListener(null, port);
      } catch (java.lang.Exception e) {
        System.out.println(e.toString());
        System.exit(1);
      }

      vlog.info("Listening on port "+port);

      while (true) {
        Socket new_sock = listener.accept();
        if (new_sock != null)
          newViewer(this, new_sock);
      }
    }

    try {
      cc = new CConn(this, sock, vncServerName.getValue());
      while (!stop)
        cc.processMsg();
      if (nViewers > 1) {
        cc = null;
        return;
      }
    } catch (EndOfStream e) {
      vlog.info(e.toString());
    } catch (java.lang.Exception e) {
      if (cc != null) cc.deleteWindow();
      if (cc == null || !cc.shuttingDown) {
        e.printStackTrace();
        JOptionPane.showMessageDialog(null,
          e.toString(),
          "VNC Viewer : Error",
          JOptionPane.ERROR_MESSAGE);
      }
    }
    if (cc != null) cc.deleteWindow();
    nViewers--;
    if (!applet && nViewers == 0) {
      System.exit(0);
    }
  }

  BoolParameter useLocalCursor
  = new BoolParameter("UseLocalCursor",
                          "Render the mouse cursor locally", true);
  BoolParameter sendLocalUsername
  = new BoolParameter("SendLocalUsername",
                          "Send the local username for SecurityTypes "+
                          "such as Plain rather than prompting", true);
  StringParameter passwordFile
  = new StringParameter("PasswordFile",
                          "Password file for VNC authentication", "");
  AliasParameter passwd
  = new AliasParameter("passwd", "Alias for PasswordFile", passwordFile);
  BoolParameter autoSelect
  = new BoolParameter("AutoSelect",
                          "Auto select pixel format and encoding", true);
  BoolParameter fullColour
  = new BoolParameter("FullColour",
                          "Use full colour - otherwise 6-bit colour is used "+
                          "until AutoSelect decides the link is fast enough",
                          true);
  AliasParameter fullColor
  = new AliasParameter("FullColor", "Alias for FullColour", fullColour);
  StringParameter preferredEncoding
  = new StringParameter("PreferredEncoding",
                            "Preferred encoding to use (Tight, ZRLE, hextile or"+
                            " raw) - implies AutoSelect=0", "Tight");
  BoolParameter viewOnly
  = new BoolParameter("ViewOnly", "Don't send any mouse or keyboard "+
                          "events to the server", false);
  BoolParameter shared
  = new BoolParameter("Shared", "Don't disconnect other viewers upon "+
                          "connection - share the desktop instead", false);
  BoolParameter fullScreen
  = new BoolParameter("FullScreen", "Full Screen Mode", false);
  BoolParameter acceptClipboard
  = new BoolParameter("AcceptClipboard",
                          "Accept clipboard changes from the server", true);
  BoolParameter sendClipboard
  = new BoolParameter("SendClipboard",
                          "Send clipboard changes to the server", true);
  StringParameter desktopSize
  = new StringParameter("DesktopSize",
                        "Reconfigure desktop size on the server on "+
                        "connect (if possible)", "");
  BoolParameter listenMode
  = new BoolParameter("listen", "Listen for connections from VNC servers", 
                      false);
  StringParameter scalingFactor
  = new StringParameter("ScalingFactor",
                        "Reduce or enlarge the remote desktop image. "+
                        "The value is interpreted as a scaling factor "+
                        "in percent.  If the parameter is set to "+
                        "\"Auto\", then automatic scaling is "+
                        "performed.  Auto-scaling tries to choose a "+
                        "scaling factor in such a way that the whole "+
                        "remote desktop will fit on the local screen.  "+
                        "If the parameter is set to \"FixedRatio\", "+
                        "then automatic scaling is performed, but the "+
                        "original aspect ratio is preserved.", "100");
  BoolParameter alwaysShowServerDialog
  = new BoolParameter("AlwaysShowServerDialog",
                          "Always show the server dialog even if a server "+
                          "has been specified in an applet parameter or on "+
                          "the command line", false);
  StringParameter vncServerName
  = new StringParameter("Server",
                            "The VNC server <host>[:<dpyNum>] or "+
                            "<host>::<port>", null);
  IntParameter vncServerPort
  = new IntParameter("Port",
                         "The VNC server's port number, assuming it is on "+
                         "the host from which the applet was downloaded", 0);
  BoolParameter acceptBell
  = new BoolParameter("AcceptBell",
                      "Produce a system beep when requested to by the server.", 
                      true);
  StringParameter via
  = new StringParameter("via", "Gateway to tunnel via", null);

  BoolParameter customCompressLevel
  = new BoolParameter("CustomCompressLevel",
                          "Use custom compression level. "+
                          "Default if CompressLevel is specified.", false);
  IntParameter compressLevel
  = new IntParameter("CompressLevel",
			                    "Use specified compression level "+
			                    "0 = Low, 6 = High",
			                    1);
  BoolParameter noJpeg
  = new BoolParameter("NoJPEG",
                          "Disable lossy JPEG compression in Tight encoding.", false);
  IntParameter qualityLevel
  = new IntParameter("QualityLevel",
			                    "JPEG quality level. "+
			                    "0 = Low, 9 = High",
			                    8);

  Thread thread;
  Socket sock;
  boolean applet, firstApplet, stop;
  Image logo;
  static int nViewers;
  static LogWriter vlog = new LogWriter("main");
}
