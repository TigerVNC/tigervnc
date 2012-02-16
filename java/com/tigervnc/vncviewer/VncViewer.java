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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
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
import java.lang.Character;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import javax.swing.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.network.*;

public class VncViewer extends java.applet.Applet implements Runnable
{
  public static final String about1 = "TigerVNC Viewer for Java";
  public static final String about2 = "Copyright (C) 1998-2011 "+
                                      "TigerVNC Team and many others (see README)";
  public static final String about3 = "Visit http://www.tigervnc.org "+
                                      "for information on TigerVNC.";
  public static String version = null;
  public static String build = null;

  public static void main(String[] argv) {
    try {
      String os = System.getProperty("os.name");
      if (os.startsWith("Windows")) {
        String laf = "com.sun.java.swing.plaf.windows.WindowsLookAndFeel";
        UIManager.setLookAndFeel(laf);
      } else {
        UIManager.put("swing.boldMetal", Boolean.FALSE);
        javax.swing.plaf.FontUIResource f = new
          javax.swing.plaf.FontUIResource("SansSerif", Font.PLAIN, 11);
        java.util.Enumeration keys = UIManager.getDefaults().keys();
        while (keys.hasMoreElements()) {
          Object key = keys.nextElement();
          Object value = UIManager.get (key);
          if (value instanceof javax.swing.plaf.FontUIResource)
            UIManager.put(key, f);
        }
      }
      UIManager.put("TitledBorder.titleColor",Color.blue);
    } catch (java.lang.Exception exc) { }
    VncViewer viewer = new VncViewer(argv);
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

  public VncViewer() {
    applet = true;
    firstApplet = true;
  }

  public static void newViewer(VncViewer oldViewer) {
    VncViewer viewer = new VncViewer();
    viewer.applet = oldViewer.applet;
    viewer.firstApplet = false;
    viewer.start();
  }


  public void init() {
    vlog.debug("init called");
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
    if (firstApplet) {
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
    Socket sock = null;

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
        sock = listener.accept();
        if (sock != null)
          break;
        //listener.close();
      }
    }

    try {
      cc = new CConn(this, sock, vncServerName.getValue());
      while (true)
        cc.processMsg();
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

  BoolParameter fastCopyRect
  = new BoolParameter("FastCopyRect",
                          "Use fast CopyRect - turn this off if you get "+
                          "screen corruption when copying from off-screen",
                          true);
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
  BoolParameter customCompressLevel
  = new BoolParameter("CustomCompressLevel",
                          "Use custom compression level. "+
                          "Default if CompressLevel is specified.", false);
  IntParameter compressLevel
  = new IntParameter("CompressLevel",
			                    "Use specified compression level "+
			                    "0 = Low, 6 = High",
			                    1);
  IntParameter keepAliveTimeout
  = new IntParameter("KeepAliveTimeout",
			               "Send dummy framebuffer update requests if no "+
                     "network activity has occurred for more than "+
                     "the specified number of seconds. This is to "+
                     "prevent VPNs or other applications from "+
                     "disconnecting due to inactivity. A value of "+
                     "0 disables this feature.", 0);
  BoolParameter noJpeg
  = new BoolParameter("NoJPEG",
                          "Disable lossy JPEG compression in Tight encoding.", false);
  IntParameter qualityLevel
  = new IntParameter("QualityLevel",
			                    "JPEG quality level. "+
			                    "0 = Low, 9 = High",
			                    8);

  Thread thread;
  boolean applet, firstApplet;
  Image logo;
  static int nViewers;
  static LogWriter vlog = new LogWriter("main");
}
