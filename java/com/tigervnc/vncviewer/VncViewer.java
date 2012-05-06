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
        java.util.Enumeration keys = UIManager.getDefaults().keys();
        while (keys.hasMoreElements()) {
          Object key = keys.nextElement();
          Object value = UIManager.get(key);
          if (value instanceof FontUIResource) {
            String name = ((FontUIResource)value).getName();
            int style = ((FontUIResource)value).getStyle();
            int size = ((FontUIResource)value).getSize()-1;
            FontUIResource f = new FontUIResource(name, style, size);
            UIManager.put(key, f);
          }
        }
      } else if (UIManager.getLookAndFeel().getName().equals("Nimbus")) {
        Font f = UIManager.getFont("TitledBorder.font");
        String name = f.getName();
        int style = f.getStyle();
        int size = f.getSize()-2;
        FontUIResource r = new FontUIResource(name, style, size);
      	UIManager.put("TitledBorder.font", r);
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

      if (argv[i].equalsIgnoreCase("-tunnel") || argv[i].equalsIgnoreCase("-via")) {
        if (!tunnel.createTunnel(argv.length, argv, i))
          System.exit(1);
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

      if (vncServerName.getValue() == null)
        vncServerName.setParam(argv[i]);
    }

  }

  public static void usage() {
    String usage = ("\nusage: vncviewer [options/parameters] "+
                    "[host:displayNum]\n"+
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
                    "are:");
    System.err.print(usage);
    VoidParameter current = Configuration.head;
    while (current != null) {
      System.err.format("%n%-7s%-64s%n", " ", "\u001B[1m"+"-"+current.getName()+"\u001B[0m");
      String[] desc = current.getDescription().split("(?<=\\G.{60})");
      for (int i = 0; i < desc.length; i++) {
        String line = desc[i];
        if (!line.endsWith(" ") && i < desc.length-1)
          line = line+"-";
        System.err.format("%-14s%-64s%n"," ", line.trim());
      }
      System.err.format("%-14s%-64s%n"," ", "[default="+current.getDefaultStr()+"]");
      current = current.next;
    }
    String propertiesString = ("\n"+
"\u001B[1mSystem Properties\u001B[0m (adapted from the TurboVNC vncviewer man page)\n"+
"\tWhen started with the -via option, vncviewer reads the\n"+
"\t\u001B[1mcom.tigervnc.VNC_VIA_CMD\u001B[0m System property, expands\n"+
"\tpatterns beginning with the \"%\" character, and uses the resulting\n"+
"\tcommand line to establish the secure tunnel to the VNC gateway.\n"+
"\tIf \u001B[1mcom.tigervnc.VNC_VIA_CMD\u001B[0m is not set, this \n"+
"\tcommand line defaults to \"/usr/bin/ssh -f -L %L:%H:%R %G sleep 20\".\n"+
"\n"+
"\tThe following patterns are recognized in the VNC_VIA_CMD property\n"+
"\t(note that all of the patterns %G, %H, %L and %R must be present in \n"+
"\tthe command template):\n"+
"\n"+
"\t\t%%     A literal \"%\";\n"+
"\n"+
"\t\t%G     gateway machine name;\n"+
"\n"+
"\t\t%H     remote VNC machine name, (as known to the gateway);\n"+
"\n"+
"\t\t%L     local TCP port number;\n"+
"\n"+
"\t\t%R     remote TCP port number.\n"+
"\n"+
"\tWhen started with the -tunnel option, vncviewer reads the\n"+
"\t\u001B[1mcom.tigervnc.VNC_TUNNEL_CMD\u001B[0m System property, expands\n"+
"\tpatterns beginning with the \"%\" character, and uses the resulting\n"+
"\tcommand line to establish the secure tunnel to the VNC server.\n"+
"\tIf \u001B[1mcom.tigervnc.VNC_TUNNEL_CMD\u001B[0m is not set, this command \n"+
"\tline defaults to \"/usr/bin/ssh -f -L %L:localhost:%R %H sleep 20\".\n"+
"\n"+
"\tThe following patterns are recognized in the VNC_TUNNEL_CMD property\n"+
"\t(note that all of the patterns %H, %L and %R must be present in \n"+
"\tthe command template):\n"+
"\n"+
"\t\t%%     A literal \"%\";\n"+
"\n"+
"\t\t%H     remote VNC machine name (as known to the client);\n"+
"\n"+
"\t\t%L     local TCP port number;\n"+
"\n"+
"\t\t%R     remote TCP port number.\n"+
"\n");
    System.err.print(propertiesString);
    System.exit(1);
  }

  public VncViewer() {
    applet = true;
    firstApplet = true;
  }

  public static void newViewer(VncViewer oldViewer, Socket sock, boolean close) {
    VncViewer viewer = new VncViewer();
    viewer.applet = oldViewer.applet;
    viewer.firstApplet = (close) ? true : false;
    viewer.sock = sock;
    viewer.start();
    if (close)
      oldViewer.stop();
  }

  public static void newViewer(VncViewer oldViewer, Socket sock) {
    newViewer(oldViewer, sock, false);
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
                      "Render the mouse cursor locally",
                      true);
  BoolParameter sendLocalUsername
  = new BoolParameter("SendLocalUsername",
                      "Send the local username for SecurityTypes "+
                      "such as Plain rather than prompting",
                      true);
  StringParameter passwordFile
  = new StringParameter("PasswordFile",
                        "Password file for VNC authentication",
                        null);
  AliasParameter passwd
  = new AliasParameter("passwd",
                       "Alias for PasswordFile",
                       passwordFile);
  BoolParameter autoSelect
  = new BoolParameter("AutoSelect",
                      "Auto select pixel format and encoding",
                      true);
  BoolParameter fullColour
  = new BoolParameter("FullColour",
                      "Use full colour - otherwise 6-bit colour is "+
                      "used until AutoSelect decides the link is "+
                      "fast enough",
                      true);
  AliasParameter fullColor
  = new AliasParameter("FullColor",
                       "Alias for FullColour",
                       fullColour);
  StringParameter preferredEncoding
  = new StringParameter("PreferredEncoding",
                        "Preferred encoding to use (Tight, ZRLE, "+
                        "hextile or raw) - implies AutoSelect=0",
                        "Tight");
  BoolParameter viewOnly
  = new BoolParameter("ViewOnly",
                      "Don't send any mouse or keyboard events to "+
                      "the server",
                      false);
  BoolParameter shared
  = new BoolParameter("Shared",
                      "Don't disconnect other viewers upon "+
                      "connection - share the desktop instead",
                      false);
  BoolParameter fullScreen
  = new BoolParameter("FullScreen",
                      "Full Screen Mode",
                      false);
  BoolParameter acceptClipboard
  = new BoolParameter("AcceptClipboard",
                      "Accept clipboard changes from the server",
                      true);
  BoolParameter sendClipboard
  = new BoolParameter("SendClipboard",
                      "Send clipboard changes to the server",
                      true);
  StringParameter desktopSize
  = new StringParameter("DesktopSize",
                        "Reconfigure desktop size on the server on "+
                        "connect (if possible)", "");
  BoolParameter listenMode
  = new BoolParameter("listen",
                      "Listen for connections from VNC servers", 
                      false);
  StringParameter scalingFactor
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
  BoolParameter alwaysShowServerDialog
  = new BoolParameter("AlwaysShowServerDialog",
                      "Always show the server dialog even if a server "+
                      "has been specified in an applet parameter or on "+
                      "the command line",
                      false);
  StringParameter vncServerName
  = new StringParameter("Server",
                        "The VNC server <host>[:<dpyNum>] or "+
                        "<host>::<port>",
                        null);
  IntParameter vncServerPort
  = new IntParameter("Port",
                     "The VNC server's port number, assuming it is on "+
                     "the host from which the applet was downloaded",
                     0);
  BoolParameter acceptBell
  = new BoolParameter("AcceptBell",
                      "Produce a system beep when requested to by the server.", 
                      true);
  StringParameter via
  = new StringParameter("via",
    "Automatically create an encrypted TCP tunnel to "+
    "machine gateway, then use that tunnel to connect "+
    "to a VNC server running on host. By default, "+
    "this option invokes SSH local port forwarding and "+
    "assumes that the SSH client binary is located at "+
    "/usr/bin/ssh. Note that when using the -via "+
    "option, the host machine name should be specified "+
    "from the point of view of the gateway machine. "+
    "For example, \"localhost\" denotes the gateway, "+
    "not the machine on which vncviewer was launched. "+
    "See the System Properties section below for "+
    "information on configuring the -via option.",
    null);

  StringParameter tunnelMode
  = new StringParameter("tunnel",
    "Automatically create an encrypted TCP tunnel to "+
    "remote gateway, then use that tunnel to connect "+
    "to the specified VNC server port on the remote "+
    "host. See the System Properties section below "+
    "for information on configuring the -tunnel option.",
    null);

  BoolParameter customCompressLevel
  = new BoolParameter("CustomCompressLevel",
                      "Use custom compression level. "+
                      "Default if CompressLevel is specified.",
                      false);
  IntParameter compressLevel
  = new IntParameter("CompressLevel",
                     "Use specified compression level "+
                     "0 = Low, 6 = High",
                     1);
  BoolParameter noJpeg
  = new BoolParameter("NoJPEG",
                      "Disable lossy JPEG compression in Tight encoding.",
                      false);
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
