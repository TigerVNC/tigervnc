/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2011-2013 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011-2019 Brian P. Hinz
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
// VncViewer - the VNC viewer.  It behaves as much as possible like the native
// viewers.
//
// Unfortunately, because of the way Java classes are loaded on demand, only
// configuration parameters defined in this file can be set from the command
// line.

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.event.*;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Image;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.File;
import java.lang.Character;
import java.lang.reflect.*;
import java.net.URL;
import java.nio.CharBuffer;
import java.util.*;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.plaf.FontUIResource;
import javax.swing.SwingUtilities;
import javax.swing.UIManager.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.network.*;

import static com.tigervnc.vncviewer.Parameters.*;

public class VncViewer implements Runnable {

  public static final String aboutText =
    new String("TigerVNC Java Viewer v%s (%s)%n"+
               "Built on %s at %s%n"+
               "Copyright (C) 1999-2022 TigerVNC Team and many others (see README.rst)%n"+
               "See https://www.tigervnc.org for information on TigerVNC.");

  public static String version = null;
  public static String build = null;
  public static String buildDate = null;
  public static String buildTime = null;
  static ImageIcon frameIconSrc =
    new ImageIcon(VncViewer.class.getResource("tigervnc.ico"));
  public static final Image frameIcon = frameIconSrc.getImage();
  public static final ImageIcon logoIcon =
    new ImageIcon(VncViewer.class.getResource("tigervnc.png"));
  public static final Image logoImage = logoIcon.getImage();
  public static final InputStream timestamp =
    VncViewer.class.getResourceAsStream("timestamp");
  public static final String os = 
    System.getProperty("os.name").toLowerCase(Locale.ENGLISH);

  private String defaultServerName;
  int VNCSERVERNAMELEN = 256;
  CharBuffer vncServerName = CharBuffer.allocate(VNCSERVERNAMELEN);

  public static void setLookAndFeel() {
    try {
      if (os.startsWith("mac os x")) {
        String appClassName = new String("com.apple.eawt.Application");
        String appMethodName = new String("getApplication");
        String setIconMethodName = new String("setDockIconImage");
        // JEP-272. Platform-specific Desktop Features are strongly encapsulated
        // in JRE 9 & above, but the API features needed aren't in JRE 8.
        if (Float.parseFloat(System.getProperty("java.specification.version")) > 1.8) {
          appClassName = new String("java.awt.Taskbar");
          appMethodName = new String("getTaskbar");
          setIconMethodName = new String("setIconImage");
        }
        Class appClass = Class.forName(appClassName);
        Method getApplication = 
          appClass.getMethod(appMethodName, (Class[])null);
        Object app = getApplication.invoke(appClass);
        Class paramTypes[] = new Class[1];
        paramTypes[0] = Image.class;
        Method setDockIconImage =
          appClass.getMethod(setIconMethodName, paramTypes);
        setDockIconImage.invoke(app, VncViewer.logoImage);
      }
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
      if (UIManager.getLookAndFeel().getName().equals("Metal")) {
        UIManager.put("swing.boldMetal", Boolean.FALSE);
        Enumeration<Object> keys = UIManager.getDefaults().keys();
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
    viewer.start();
  }

  public VncViewer(String[] argv) {

    SecurityClient.setDefaults();

    // Write about text to console, still using normal locale codeset
    getTimestamp();
    System.err.format("%n");
    System.err.format(aboutText, version, build, buildDate, buildTime);
    System.err.format("%n");

    Configuration.enableViewerParams();

    /* Load the default parameter settings */
    try {
      defaultServerName = loadViewerParameters(null);
    } catch (com.tigervnc.rfb.Exception e) {
      defaultServerName = "";
      vlog.info(e.getMessage());
    }

    // Override defaults with command-line options
    int i = 0;
    for (; i < argv.length; i++) {
      if (argv[i].length() == 0)
        continue;

      if (argv[i].equalsIgnoreCase("-config")) {
        if (++i >= argv.length)
          usage();
        defaultServerName = loadViewerParameters(argv[i]);
        continue;
      }

      if (argv[i].equalsIgnoreCase("-log")) {
        if (++i >= argv.length) usage();
        System.err.println("Log setting: "+argv[i]);
        LogWriter.setLogParams(argv[i]);
        continue;
      }

      if (argv[i].charAt(0) == '-') {
        if (i+1 < argv.length) {
          if (Configuration.setParam(argv[i].substring(1), argv[i+1])) {
            i++;
            continue;
          }
        }
        if (Configuration.setParam(argv[i]))
          continue;

        usage();
      }

      vncServerName.put(argv[i].toCharArray()).flip();
    }

    // Check if the server name in reality is a configuration file
    potentiallyLoadConfigurationFile(vncServerName);
  }

  public static void usage() {
    String usage = ("\nusage: vncviewer [options/parameters] "+
                    "[host:displayNum]\n"+
                    "       vncviewer [options/parameters] -listen [port] "+
                    "[options/parameters]\n"+
                    "       vncviewer [options/parameters] [.tigervnc file]\n"+
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
                    "are:\n"+
                    "\n");
    System.err.print(usage);

    Configuration.listParams(79, 14);
    String propertiesString = ("\n"+
"System Properties (adapted from the TurboVNC vncviewer man page)\n"+
"  When started with the -via option, vncviewer reads the VNC_VIA_CMD\n"+
"  System property, expands patterns beginning with the \"%\" character,\n"+
"  and uses the resulting command line to establish the secure tunnel\n"+
"  to the VNC gateway.  If VNC_VIA_CMD is not set, this command line\n"+
"  defaults to \"/usr/bin/ssh -f -L %L:%H:%R %G sleep 20\".\n"+
"\n"+
"  The following patterns are recognized in the VNC_VIA_CMD property\n"+
"  (note that all of the patterns %G, %H, %L and %R must be present in \n"+
"  the command template):\n"+
"\n"+
"  \t%%     A literal \"%\";\n"+
"\n"+
"  \t%G     gateway machine name;\n"+
"\n"+
"  \t%H     remote VNC machine name, (as known to the gateway);\n"+
"\n"+
"  \t%L     local TCP port number;\n"+
"\n"+
"  \t%R     remote TCP port number.\n"+
"\n"+
"  When started with the -tunnel option, vncviewer reads the VNC_TUNNEL_CMD\n"+
"  System property, expands patterns beginning with the \"%\" character, and\n"+
"  uses the resulting command line to establish the secure tunnel to the\n"+
"  VNC server.  If VNC_TUNNEL_CMD is not set, this command line defaults\n"+
"  to \"/usr/bin/ssh -f -L %L:localhost:%R %H sleep 20\".\n"+
"\n"+
"  The following patterns are recognized in the VNC_TUNNEL_CMD property\n"+
"  (note that all of the patterns %H, %L and %R must be present in \n"+
"  the command template):\n"+
"\n"+
"  \t%%     A literal \"%\";\n"+
"\n"+
"  \t%H     remote VNC machine name (as known to the client);\n"+
"\n"+
"  \t%L     local TCP port number;\n"+
"\n"+
"  \t%R     remote TCP port number.\n"+
"\n");
    System.err.print(propertiesString);
    // Technically, we shouldn't use System.exit here but if there is a parameter
    // error then the problem is in the index/html file anyway.
    System.exit(1);
  }

  public static void potentiallyLoadConfigurationFile(CharBuffer vncServerName) {
    String serverName = vncServerName.toString();
    boolean hasPathSeparator = (serverName.indexOf('/') != -1 ||
                                (serverName.indexOf('\\')) != -1);

    if (hasPathSeparator) {
      try {
        serverName = loadViewerParameters(vncServerName.toString());
        if (serverName == "") {
          vlog.info("Unable to load the server name from given file");
          System.exit(1);
        }
        vncServerName.clear();
        vncServerName.put(serverName).flip();
      } catch (com.tigervnc.rfb.Exception e) {
        vlog.info(e.getMessage());
        System.exit(1);
      }
    }
  }

  public static void newViewer() {
    String cmd = "java -jar ";
    try {
      URL url =
        VncViewer.class.getProtectionDomain().getCodeSource().getLocation();
      File f = new File(url.toURI());
      if (!f.exists() || !f.canRead()) {
        String msg = new String("The jar file "+f.getAbsolutePath()+
                                " does not exist or cannot be read.");
        JOptionPane.showMessageDialog(null, msg, "ERROR",
                                      JOptionPane.ERROR_MESSAGE);
        return;
      }
      cmd = cmd.concat(f.getAbsolutePath());
      Thread t = new Thread(new ExtProcess(cmd, vlog));
      t.start();
    } catch (java.net.URISyntaxException e) {
      vlog.info(e.getMessage());
    } catch (java.lang.Exception e) {
      vlog.info(e.getMessage());
    }
  }

  private static void getTimestamp() {
    if (version == null || build == null) {
      try {
        Manifest manifest = new Manifest(timestamp);
        Attributes attributes = manifest.getMainAttributes();
        version = attributes.getValue("Version");
        build = attributes.getValue("Build");
        buildDate = attributes.getValue("Package-Date");
        buildTime = attributes.getValue("Package-Time");
      } catch (java.lang.Exception e) { }
    }
  }

  public static void about_vncviewer(Container parent) {
    String pkgDate = "";
    String pkgTime = "";
    try {
      Manifest manifest = new Manifest(VncViewer.timestamp);
      Attributes attributes = manifest.getMainAttributes();
      pkgDate = attributes.getValue("Package-Date");
      pkgTime = attributes.getValue("Package-Time");
    } catch (java.lang.Exception e) { }

    Window fullScreenWindow = DesktopWindow.getFullScreenWindow();
    if (fullScreenWindow != null)
      DesktopWindow.setFullScreenWindow(null);
    String msg =
      String.format(VncViewer.aboutText, VncViewer.version, VncViewer.build,
                    VncViewer.buildDate, VncViewer.buildTime);
    Object[] options = {"Close  \u21B5"};
    JOptionPane op =
      new JOptionPane(msg, JOptionPane.INFORMATION_MESSAGE,
                      JOptionPane.DEFAULT_OPTION, VncViewer.logoIcon, options);
    JDialog dlg = op.createDialog(parent, "About TigerVNC Viewer for Java");
    dlg.setIconImage(VncViewer.frameIcon);
    dlg.setAlwaysOnTop(true);
    dlg.setVisible(true);
    if (fullScreenWindow != null)
      DesktopWindow.setFullScreenWindow(fullScreenWindow);
  }

  public void start() {
    (new Thread(this, "VncViewer Thread")).start();
  }

  public void exit(int n) {
    System.exit(n);
  }

  void reportException(java.lang.Exception e) {
    String title, msg = e.getMessage();
    int msgType = JOptionPane.ERROR_MESSAGE;
    title = "TigerVNC Viewer : Error";
    e.printStackTrace();
    JOptionPane.showMessageDialog(null, msg, title, msgType);
  }

  public void run() {
    cc = null;
    UserDialog dlg = new UserDialog();
    CSecurity.upg = dlg;
    CSecurityTLS.msg = dlg;
    Socket sock = null;

    /* Specifying -via and -listen together is nonsense */
    if (listenMode.getValue() && !via.getValueStr().isEmpty()) {
      vlog.error("Parameters -listen and -via are incompatible");
      String msg =
        new String("Parameters -listen and -via are incompatible");
      JOptionPane.showMessageDialog(null, msg, "ERROR",
                                    JOptionPane.ERROR_MESSAGE);
      exit(1);
    }

    if (listenMode.getValue()) {
      int port = 5500;

      if (vncServerName.charAt(0) != 0 &&
          Character.isDigit(vncServerName.charAt(0)))
        port = Integer.parseInt(vncServerName.toString());

      TcpListener listener = null;
      try {
        listener = new TcpListener(null, port);
      } catch (java.lang.Exception e) {
        reportException(e);
        exit(1);
      }

      vlog.info("Listening on port "+port);

      while (sock == null)
        sock = listener.accept();
    } else {
      if (vncServerName.charAt(0) == 0) {
        try {
          SwingUtilities.invokeAndWait(
            new ServerDialog(defaultServerName, vncServerName));
        } catch (InvocationTargetException e) {
          reportException(e);
        } catch (InterruptedException e) {
          reportException(e);
        }
        if (vncServerName.charAt(0) == 0)
          exit(0);
      }
    }

    try {
      cc = new CConn(vncServerName.toString(), sock);
      while (!cc.shuttingDown)
        cc.processMsg();
      exit(0);
    } catch (java.lang.Exception e) {
      if (cc == null || !cc.shuttingDown) {
        reportException(e);
        if (cc != null)
          cc.close();
      }
      exit(1);
    }
  }

  public static CConn cc;
  public static StringParameter config
  = new StringParameter("Config",
  "Specifies a configuration file to load.", null);

  static LogWriter vlog = new LogWriter("VncViewer");
}
