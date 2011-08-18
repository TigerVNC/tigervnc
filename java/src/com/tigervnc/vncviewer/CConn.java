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
// CConn
//
// Methods on CConn are called from both the GUI thread and the thread which
// processes incoming RFB messages ("the RFB thread").  This means we need to
// be careful with synchronization here.
//
// Any access to writer() must not only be synchronized, but we must also make
// sure that the connection is in RFBSTATE_NORMAL.  We are guaranteed this for
// any code called after serverInit() has been called.  Since the DesktopWindow
// isn't created until then, any methods called only from DesktopWindow can
// assume that we are in RFBSTATE_NORMAL.

package com.tigervnc.vncviewer;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.event.*;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Event;
import java.awt.Frame;
import java.awt.ScrollPane;

import java.io.*;
import javax.net.ssl.*;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import javax.swing.*;
import javax.swing.filechooser.*;
import javax.swing.ImageIcon;
import java.net.URL;
import java.net.ServerSocket;
import javax.swing.border.*;
import java.util.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;
import com.tigervnc.rfb.Point;
import com.tigervnc.rfb.Rect;

class ViewportFrame extends JFrame 
{
  public ViewportFrame(String name, CConn cc_) {
    cc = cc_;
    setTitle("TigerVNC: "+name);
    setFocusable(false);
    setFocusTraversalKeysEnabled(false);
    addWindowFocusListener(new WindowAdapter() {
      public void windowGainedFocus(WindowEvent e) {
        sp.getViewport().getView().requestFocusInWindow();
      }
    });
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        cc.close();
      }
    });
  }

  public void addChild(DesktopWindow child) {
    sp = new JScrollPane(child);
    child.setBackground(Color.BLACK);
    child.setOpaque(true);
    sp.setBorder(BorderFactory.createEmptyBorder(0,0,0,0));
    getContentPane().add(sp);
  }

  public void setChild(DesktopWindow child) {
    getContentPane().removeAll();
    addChild(child);
  }

  public void setGeometry(int x, int y, int w, int h) {
    pack();
    if (cc.fullScreen) setSize(w, h);
    setLocation(x, y);
    setBackground(Color.BLACK);
  }


  CConn cc;
  Graphics g;
  JScrollPane sp;
  static LogWriter vlog = new LogWriter("ViewportFrame");
}

public class CConn extends CConnection
  implements UserPasswdGetter, UserMsgBox, OptionsDialogCallback
{
  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the RFB thread

  public CConn(VncViewer viewer_, java.net.Socket sock_, 
               String vncServerName, boolean reverse) 
  {
    serverHost = null; serverPort = 0; sock = sock_; viewer = viewer_; 
    currentEncoding = Encodings.encodingTight; lastServerEncoding = -1;
    fullColour = viewer.fullColour.getValue();
    lowColourLevel = 2;
    autoSelect = viewer.autoSelect.getValue();
    shared = viewer.shared.getValue(); formatChange = false;
    encodingChange = false; sameMachine = false;
    fullScreen = viewer.fullScreen.getValue();
    menuKey = Keysyms.F8;
    options = new OptionsDialog(this);
    clipboardDialog = new ClipboardDialog(this);
    firstUpdate = true; pendingUpdate = false;

    setShared(shared);
    upg = this;
    msg = this;

    String encStr = viewer.preferredEncoding.getValue();
    int encNum = Encodings.encodingNum(encStr);
    if (encNum != -1) {
      currentEncoding = encNum;
    }
    cp.supportsDesktopResize = true;
    cp.supportsExtendedDesktopSize = true;
    cp.supportsClientRedirect = true;
    cp.supportsDesktopRename = true;
    cp.supportsLocalCursor = viewer.useLocalCursor.getValue();
    cp.customCompressLevel = viewer.customCompressLevel.getValue();
    cp.compressLevel = viewer.compressLevel.getValue();
    cp.noJpeg = viewer.noJpeg.getValue();
    cp.qualityLevel = viewer.qualityLevel.getValue();
    initMenu();

    if (sock != null) {
      String name = sock.getRemoteSocketAddress()+"::"+sock.getPort();
      vlog.info("Accepted connection from "+name);
    } else {
      if (vncServerName != null) {
        serverHost = Hostname.getHost(vncServerName);
        serverPort = Hostname.getPort(vncServerName);
      } else {
        ServerDialog dlg = new ServerDialog(options, vncServerName, this);
        if (!dlg.showDialog() || dlg.server.getSelectedItem().equals("")) {
          System.exit(1);
        }
        vncServerName = (String)dlg.server.getSelectedItem();
        serverHost = Hostname.getHost(vncServerName);
        serverPort = Hostname.getPort(vncServerName);
      }

      try {
        sock = new java.net.Socket(serverHost, serverPort);
      } catch (java.io.IOException e) { 
        throw new Exception(e.toString());
      }
      vlog.info("connected to host "+serverHost+" port "+serverPort);
    }

    sameMachine = (sock.getLocalSocketAddress() == sock.getRemoteSocketAddress());
    try {
      sock.setTcpNoDelay(true);
      sock.setTrafficClass(0x10);
      setServerName(serverHost);
      jis = new JavaInStream(sock.getInputStream());
      jos = new JavaOutStream(sock.getOutputStream());
    } catch (java.net.SocketException e) {
      throw new Exception(e.toString());
    } catch (java.io.IOException e) { 
      throw new Exception(e.toString());
    }
    setStreams(jis, jos);
    initialiseProtocol();
  }

  public boolean showMsgBox(int flags, String title, String text)
  {
    StringBuffer titleText = new StringBuffer("VNC Viewer: "+title);

    return true;
  }

  // deleteWindow() is called when the user closes the desktop or menu windows.

  void deleteWindow() {
    if (viewport != null)
      viewport.dispose();
    viewport = null;
  } 

  // getUserPasswd() is called by the CSecurity object when it needs us to read
  // a password from the user.

  public final boolean getUserPasswd(StringBuffer user, StringBuffer passwd) {
    String title = ("VNC Authentication ["
                    +csecurity.description() + "]");
    PasswdDialog dlg;    
    if (user == null) {
      dlg = new PasswdDialog(title, (user == null), (passwd == null));
    } else {
      if ((passwd == null) && viewer.sendLocalUsername.getValue()) {
         user.append((String)System.getProperties().get("user.name"));
         return true;
      }
      dlg = new PasswdDialog(title, viewer.sendLocalUsername.getValue(), 
         (passwd == null));
    }
    if (!dlg.showDialog()) return false;
    if (user != null) {
      if (viewer.sendLocalUsername.getValue()) {
         user.append((String)System.getProperties().get("user.name"));
      } else {
         user.append(dlg.userEntry.getText());
      }
    }
    if (passwd != null)
      passwd.append(dlg.passwdEntry.getText());
    return true;
  }

  // CConnection callback methods

  // serverInit() is called when the serverInit message has been received.  At
  // this point we create the desktop window and display it.  We also tell the
  // server the pixel format and encodings to use and request the first update.
  public void serverInit() {
    super.serverInit();

    // If using AutoSelect with old servers, start in FullColor
    // mode. See comment in autoSelectFormatAndEncoding. 
    if (cp.beforeVersion(3, 8) && autoSelect) {
      fullColour = true;
    }

    serverPF = cp.pf();
    desktop = new DesktopWindow(cp.width, cp.height, serverPF, this);
    //desktopEventHandler = desktop.setEventHandler(this);
    //desktop.addEventMask(KeyPressMask | KeyReleaseMask);
    fullColourPF = desktop.getPF();
    if (!serverPF.trueColour)
      fullColour = true;
    recreateViewport();
    formatChange = true; encodingChange = true;
    requestNewUpdate();
  }

  // setDesktopSize() is called when the desktop size changes (including when
  // it is set initially).
  public void setDesktopSize(int w, int h) {
    super.setDesktopSize(w,h);
    resizeFramebuffer();
  }

  // setExtendedDesktopSize() is a more advanced version of setDesktopSize()
  public void setExtendedDesktopSize(int reason, int result, int w, int h,
                                     ScreenSet layout) {
    super.setExtendedDesktopSize(reason, result, w, h, layout);

    if ((reason == screenTypes.reasonClient) &&
        (result != screenTypes.resultSuccess)) {
      vlog.error("SetDesktopSize failed: "+result);
      return;
    }

    resizeFramebuffer();
  }

  // clientRedirect() migrates the client to another host/port
  public void clientRedirect(int port, String host, 
                             String x509subject) {
    try {
      getSocket().close();
      setServerPort(port);
      sock = new java.net.Socket(host, port);
      sock.setTcpNoDelay(true);
      sock.setTrafficClass(0x10);
      setSocket(sock);
      vlog.info("Redirected to "+host+":"+port);
      setStreams(new JavaInStream(sock.getInputStream()),
                 new JavaOutStream(sock.getOutputStream()));
      initialiseProtocol();
    } catch (java.io.IOException e) {
      e.printStackTrace();
    }
  }

  // setName() is called when the desktop name changes
  public void setName(String name) {
    super.setName(name);
  
    if (viewport != null) {
      viewport.setTitle("TigerVNC: "+name);
    }
  }

  // framebufferUpdateStart() is called at the beginning of an update.
  // Here we try to send out a new framebuffer update request so that the
  // next update can be sent out in parallel with us decoding the current
  // one. We cannot do this if we're in the middle of a format change
  // though.
  public void framebufferUpdateStart() {
    if (!formatChange) {
      pendingUpdate = true;
      requestNewUpdate();
    } else 
      pendingUpdate = false;
  }

  // framebufferUpdateEnd() is called at the end of an update.
  // For each rectangle, the FdInStream will have timed the speed
  // of the connection, allowing us to select format and encoding
  // appropriately, and then request another incremental update.
  public void framebufferUpdateEnd() {
    desktop.framebufferUpdateEnd();

    if (firstUpdate) {
      int width, height;
      
      if (cp.supportsSetDesktopSize &&
          viewer.desktopSize.getValue() != null &&
          viewer.desktopSize.getValue().split("x").length == 2) {
        width = Integer.parseInt(viewer.desktopSize.getValue().split("x")[0]);
        height = Integer.parseInt(viewer.desktopSize.getValue().split("x")[1]);
        ScreenSet layout;

        layout = cp.screenLayout;

        if (layout.num_screens() == 0)
          layout.add_screen(new Screen());
        else if (layout.num_screens() != 1) {

          while (true) {
            Iterator iter = layout.screens.iterator(); 
            Screen screen = (Screen)iter.next();
        
            if (!iter.hasNext())
              break;

            layout.remove_screen(screen.id);
          }
        }

        Screen screen0 = (Screen)layout.screens.iterator().next();
        screen0.dimensions.tl.x = 0;
        screen0.dimensions.tl.y = 0;
        screen0.dimensions.br.x = width;
        screen0.dimensions.br.y = height;

        writer().writeSetDesktopSize(width, height, layout);
      }

      firstUpdate = false;
    }

    // A format change prevented us from sending this before the update,
    // so make sure to send it now.
    if (formatChange && !pendingUpdate)
      requestNewUpdate();

    // Compute new settings based on updated bandwidth values
    if (autoSelect)
      autoSelectFormatAndEncoding();
  }

  // The rest of the callbacks are fairly self-explanatory...

  public void setColourMapEntries(int firstColour, int nColours, int[] rgbs) {
    desktop.setColourMapEntries(firstColour, nColours, rgbs);
  }

  public void bell() { 
    if (viewer.acceptBell.getValue())
      desktop.getToolkit().beep(); 
  }

  public void serverCutText(String str, int len) {
    if (viewer.acceptClipboard.getValue())
      clipboardDialog.serverCutText(str, len);
  }

  // We start timing on beginRect and stop timing on endRect, to
  // avoid skewing the bandwidth estimation as a result of the server
  // being slow or the network having high latency
  public void beginRect(Rect r, int encoding) {
    ((JavaInStream)getInStream()).startTiming();
    if (encoding != Encodings.encodingCopyRect) {
      lastServerEncoding = encoding;
    }
  }

  public void endRect(Rect r, int encoding) {
    ((JavaInStream)getInStream()).stopTiming();
  }

  public void fillRect(Rect r, int p) {
    desktop.fillRect(r.tl.x, r.tl.y, r.width(), r.height(), p);
  }
  public void imageRect(Rect r, int[] p) {
    desktop.imageRect(r.tl.x, r.tl.y, r.width(), r.height(), p);
  }
  public void copyRect(Rect r, int sx, int sy) {
    desktop.copyRect(r.tl.x, r.tl.y, r.width(), r.height(), sx, sy);
  }

  public void setCursor(int width, int height, Point hotspot,
                        int[] data, byte[] mask) {
    desktop.setCursor(width, height, hotspot, data, mask);
  }

  private void resizeFramebuffer()
  {
    if ((cp.width == 0) && (cp.height == 0))
      return;
    if (desktop == null)
      return;
    if ((desktop.width() == cp.width) && (desktop.height() == cp.height))
      return;
    
    desktop.resize();
    recreateViewport();
  }

  // recreateViewport() recreates our top-level window.  This seems to be
  // better than attempting to resize the existing window, at least with
  // various X window managers.

  private void recreateViewport()
  {
    if (viewport != null) viewport.dispose();
    viewport = new ViewportFrame(cp.name(), this);
    viewport.setUndecorated(fullScreen);
    desktop.setViewport(viewport);
    ClassLoader loader = this.getClass().getClassLoader();
    URL url = loader.getResource("com/tigervnc/vncviewer/tigervnc.ico");
    ImageIcon icon = null;
    if (url != null) {
      icon = new ImageIcon(url);
      viewport.setIconImage(icon.getImage());
    }
    viewport.addChild(desktop);
    reconfigureViewport();
    viewport.setVisible(true);
    desktop.initGraphics();
    desktop.requestFocusInWindow();
  }

  private void reconfigureViewport()
  {
    //viewport->setMaxSize(cp.width, cp.height);
    if (fullScreen) {
      Dimension dpySize = viewport.getToolkit().getScreenSize();
      viewport.setExtendedState(JFrame.MAXIMIZED_BOTH);
      viewport.setGeometry(0, 0, dpySize.width, dpySize.height);
    } else {
      int w = cp.width;
      int h = cp.height;
      Dimension dpySize = viewport.getToolkit().getScreenSize();
      int wmDecorationWidth = 0;
      int wmDecorationHeight = 24;
      if (w + wmDecorationWidth >= dpySize.width)
        w = dpySize.width - wmDecorationWidth;
      if (h + wmDecorationHeight >= dpySize.height)
        h = dpySize.height - wmDecorationHeight;

      int x = (dpySize.width - w - wmDecorationWidth) / 2;
      int y = (dpySize.height - h - wmDecorationHeight)/2;

      viewport.setExtendedState(JFrame.NORMAL);
      viewport.setGeometry(x, y, w, h);
    }
  }

  // autoSelectFormatAndEncoding() chooses the format and encoding appropriate
  // to the connection speed:
  //
  //   First we wait for at least one second of bandwidth measurement.
  //
  //   Above 16Mbps (i.e. LAN), we choose the second highest JPEG quality,
  //   which should be perceptually lossless.
  //
  //   If the bandwidth is below that, we choose a more lossy JPEG quality.
  //
  //   If the bandwidth drops below 256 Kbps, we switch to palette mode.
  //
  //   Note: The system here is fairly arbitrary and should be replaced
  //         with something more intelligent at the server end.
  //
  private void autoSelectFormatAndEncoding() {
    long kbitsPerSecond = ((JavaInStream)getInStream()).kbitsPerSecond();
    long timeWaited = ((JavaInStream)getInStream()).timeWaited();
    boolean newFullColour = fullColour;
    int newQualityLevel = cp.qualityLevel;

    // Always use Tight
    if (currentEncoding != Encodings.encodingTight) {
      currentEncoding = Encodings.encodingTight;
      encodingChange = true;
    }

    // Check that we have a decent bandwidth measurement
    if ((kbitsPerSecond == 0) || (timeWaited < 10000))
      return;
  
    // Select appropriate quality level
    if (!cp.noJpeg) {
      if (kbitsPerSecond > 16000)
        newQualityLevel = 8;
      else
        newQualityLevel = 6;
  
      if (newQualityLevel != cp.qualityLevel) {
        vlog.info("Throughput "+kbitsPerSecond+
                  " kbit/s - changing to quality "+newQualityLevel);
        cp.qualityLevel = newQualityLevel;
        viewer.qualityLevel.setParam(Integer.toString(newQualityLevel));
        encodingChange = true;
      }
    }

    if (cp.beforeVersion(3, 8)) {
      // Xvnc from TightVNC 1.2.9 sends out FramebufferUpdates with
      // cursors "asynchronously". If this happens in the middle of a
      // pixel format change, the server will encode the cursor with
      // the old format, but the client will try to decode it
      // according to the new format. This will lead to a
      // crash. Therefore, we do not allow automatic format change for
      // old servers.
      return;
    }
    
    // Select best color level
    newFullColour = (kbitsPerSecond > 256);
    if (newFullColour != fullColour) {
      vlog.info("Throughput "+kbitsPerSecond+
                " kbit/s - full color is now "+ 
  	            (newFullColour ? "enabled" : "disabled"));
      fullColour = newFullColour;
      formatChange = true;
    } 
  }

  // requestNewUpdate() requests an update from the server, having set the
  // format and encoding appropriately.
  private void requestNewUpdate()
  {
    if (formatChange) {

      /* Catch incorrect requestNewUpdate calls */
      assert(pendingUpdate == false);

      if (fullColour) {
        desktop.setPF(fullColourPF);
      } else {
        if (lowColourLevel == 0) {
          desktop.setPF(new PixelFormat(8,3,false,true,1,1,1,2,1,0));
        } else if (lowColourLevel == 1) {
          desktop.setPF(new PixelFormat(8,6,false,true,3,3,3,4,2,0));
        } else {
          desktop.setPF(new PixelFormat(8,8,false,true,7,7,3,0,3,6));
        }
      }
      String str = desktop.getPF().print();
      vlog.info("Using pixel format "+str);
      cp.setPF(desktop.getPF());
      synchronized (this) {
        writer().writeSetPixelFormat(cp.pf());
      }
    }
    checkEncodings();
    synchronized (this) {
      writer().writeFramebufferUpdateRequest(new Rect(0,0,cp.width,cp.height),
                                             !formatChange);
    }
    formatChange = false;
  }


  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the GUI thread

  // close() closes the socket, thus waking up the RFB thread.
  public void close() {
    try {
      shuttingDown = true;
      sock.close();
    } catch (java.io.IOException e) {
      e.printStackTrace();
    }
  }

  // Menu callbacks.  These are guaranteed only to be called after serverInit()
  // has been called, since the menu is only accessible from the DesktopWindow

  private void initMenu() {
    menu = new F8Menu(this);
  }

  void showMenu(int x, int y) {
    String os = System.getProperty("os.name");
    if (os.startsWith("Windows"))
      com.sun.java.swing.plaf.windows.WindowsLookAndFeel.setMnemonicHidden(false);
    menu.show(desktop, x, y);
  }

  void showAbout() {
    InputStream stream = cl.getResourceAsStream("com/tigervnc/vncviewer/timestamp");
    String pkgDate = "";
    String pkgTime = "";
    try {
      Manifest manifest = new Manifest(stream);
      Attributes attributes = manifest.getMainAttributes();
      pkgDate = attributes.getValue("Package-Date");
      pkgTime = attributes.getValue("Package-Time");
    } catch (IOException e) { }
    JOptionPane.showMessageDialog((viewport != null ? viewport : null),
      VncViewer.about1+"\n"
      +"Built on "+pkgDate+" at "+pkgTime+"\n"
      +VncViewer.about2+"\n"
      +VncViewer.about3,
      "About TigerVNC Viewer for Java",
      JOptionPane.INFORMATION_MESSAGE,
      logo);
  }

  void showInfo() {
    JOptionPane.showMessageDialog(viewport,
      "Desktop name: "+cp.name()+"\n"
      +"Host: "+serverHost+":"+sock.getPort()+"\n"
      +"Size: "+cp.width+"x"+cp.height+"\n"
      +"Pixel format: "+desktop.getPF().print()+"\n"
      +"(server default "+serverPF.print()+")\n"
      +"Requested encoding: "+Encodings.encodingName(currentEncoding)+"\n"
      +"Last used encoding: "+Encodings.encodingName(lastServerEncoding)+"\n"
      +"Line speed estimate: "+((JavaInStream)getInStream()).kbitsPerSecond()+" kbit/s"+"\n"
      +"Protocol version: "+cp.majorVersion+"."+cp.minorVersion+"\n"
      +"Security method: "+Security.secTypeName(csecurity.getType())
       +" ["+csecurity.description()+"]",
      "VNC connection info",
      JOptionPane.PLAIN_MESSAGE);
  }

  synchronized public void refresh() {
    writer().writeFramebufferUpdateRequest(new Rect(0,0,cp.width,cp.height), false);
    pendingUpdate = true;
  }


  // OptionsDialogCallback.  setOptions() sets the options dialog's checkboxes
  // etc to reflect our flags.  getOptions() sets our flags according to the
  // options dialog's checkboxes.  They are both called from the GUI thread.
  // Some of the flags are also accessed by the RFB thread.  I believe that
  // reading and writing boolean and int values in java is atomic, so there is
  // no need for synchronization.

  public void setOptions() {
    int digit;
    options.autoSelect.setSelected(autoSelect);
    options.fullColour.setSelected(fullColour);
    options.veryLowColour.setSelected(!fullColour && lowColourLevel == 0);
    options.lowColour.setSelected(!fullColour && lowColourLevel == 1);
    options.mediumColour.setSelected(!fullColour && lowColourLevel == 2);
    options.tight.setSelected(currentEncoding == Encodings.encodingTight);
    options.zrle.setSelected(currentEncoding == Encodings.encodingZRLE);
    options.hextile.setSelected(currentEncoding == Encodings.encodingHextile);
    options.raw.setSelected(currentEncoding == Encodings.encodingRaw);

    options.customCompressLevel.setSelected(viewer.customCompressLevel.getValue());
    digit = 0 + viewer.compressLevel.getValue();
    if (digit >= 0 && digit <= 9) {
      options.compressLevel.setSelectedItem(digit);
    } else {
      options.compressLevel.setSelectedItem(Integer.parseInt(viewer.compressLevel.getDefaultStr()));
    }
    options.noJpeg.setSelected(!viewer.noJpeg.getValue());
    digit = 0 + viewer.qualityLevel.getValue();
    options.qualityLevel.setSelectedItem(digit);

    options.viewOnly.setSelected(viewer.viewOnly.getValue());
    options.acceptClipboard.setSelected(viewer.acceptClipboard.getValue());
    options.sendClipboard.setSelected(viewer.sendClipboard.getValue());
    options.menuKey.setSelectedIndex(menuKey-0xFFBE);

    if (state() == RFBSTATE_NORMAL) {
      options.shared.setEnabled(false);
      options.secVeNCrypt.setEnabled(false);
      options.encNone.setEnabled(false);
      options.encTLS.setEnabled(false);
      options.encX509.setEnabled(false);
      options.ca.setEnabled(false);
      options.crl.setEnabled(false);
      options.secIdent.setEnabled(false);
      options.secNone.setEnabled(false);
      options.secVnc.setEnabled(false);
      options.secPlain.setEnabled(false);
      options.sendLocalUsername.setEnabled(false);
    } else {
      options.shared.setSelected(shared);

      /* Process non-VeNCrypt sectypes */
      java.util.List<Integer> secTypes = new ArrayList<Integer>();
      secTypes = security.GetEnabledSecTypes();
      for (Iterator i = secTypes.iterator(); i.hasNext();) {
        switch ((Integer)i.next()) {
        case Security.secTypeVeNCrypt:
          options.secVeNCrypt.setSelected(true);
          break;
        case Security.secTypeNone:
          options.encNone.setSelected(true);
          options.secNone.setSelected(true);
          break;
        case Security.secTypeVncAuth:
          options.encNone.setSelected(true);
          options.secVnc.setSelected(true);
          break;
        }
      }

      /* Process VeNCrypt subtypes */
      if (options.secVeNCrypt.isSelected()) {
        java.util.List<Integer> secTypesExt = new ArrayList<Integer>();
        secTypesExt = security.GetEnabledExtSecTypes();
        for (Iterator iext = secTypesExt.iterator(); iext.hasNext();) {
          switch ((Integer)iext.next()) {
          case Security.secTypePlain:
            options.secPlain.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          case Security.secTypeIdent:
            options.secIdent.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          case Security.secTypeTLSNone:
            options.encTLS.setSelected(true);
            options.secNone.setSelected(true);
            break;
          case Security.secTypeTLSVnc:
            options.encTLS.setSelected(true);
            options.secVnc.setSelected(true);
            break;
          case Security.secTypeTLSPlain:
            options.encTLS.setSelected(true);
            options.secPlain.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          case Security.secTypeTLSIdent:
            options.encTLS.setSelected(true);
            options.secIdent.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          case Security.secTypeX509None:
            options.encX509.setSelected(true);
            options.secNone.setSelected(true);
            break;
          case Security.secTypeX509Vnc:
            options.encX509.setSelected(true);
            options.secVnc.setSelected(true);
            break;
          case Security.secTypeX509Plain:
            options.encX509.setSelected(true);
            options.secPlain.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          case Security.secTypeX509Ident:
            options.encX509.setSelected(true);
            options.secIdent.setSelected(true);
            options.sendLocalUsername.setSelected(true);
            break;
          }
        }
      }
      options.sendLocalUsername.setEnabled(options.secPlain.isSelected()||
        options.secIdent.isSelected());
    }

    options.fullScreen.setSelected(fullScreen);
    options.useLocalCursor.setSelected(viewer.useLocalCursor.getValue());
    options.fastCopyRect.setSelected(viewer.fastCopyRect.getValue());
    options.acceptBell.setSelected(viewer.acceptBell.getValue());
  }

  public void getOptions() {
    autoSelect = options.autoSelect.isSelected();
    if (fullColour != options.fullColour.isSelected())
      formatChange = true;
    fullColour = options.fullColour.isSelected();
    if (!fullColour) {
      int newLowColourLevel = (options.veryLowColour.isSelected() ? 0 :
                               options.lowColour.isSelected() ? 1 : 2);
      if (newLowColourLevel != lowColourLevel) {
        lowColourLevel = newLowColourLevel;
        formatChange = true;
      }
    }
    int newEncoding = (options.zrle.isSelected() ?  Encodings.encodingZRLE :
                       options.hextile.isSelected() ?  Encodings.encodingHextile :
                       options.tight.isSelected() ?  Encodings.encodingTight :
                       Encodings.encodingRaw);
    if (newEncoding != currentEncoding) {
      currentEncoding = newEncoding;
      encodingChange = true;
    }

    viewer.customCompressLevel.setParam(options.customCompressLevel.isSelected());
    if (cp.customCompressLevel != viewer.customCompressLevel.getValue()) {
      cp.customCompressLevel = viewer.customCompressLevel.getValue();
      encodingChange = true;
    }
    if (Integer.parseInt(options.compressLevel.getSelectedItem().toString()) >= 0 && 
        Integer.parseInt(options.compressLevel.getSelectedItem().toString()) <= 9) {
      viewer.compressLevel.setParam(options.compressLevel.getSelectedItem().toString());
    } else {
      viewer.compressLevel.setParam(viewer.compressLevel.getDefaultStr());
    }
    if (cp.compressLevel != viewer.compressLevel.getValue()) {
      cp.compressLevel = viewer.compressLevel.getValue();
      encodingChange = true;
    }
    viewer.noJpeg.setParam(!options.noJpeg.isSelected());
    if (cp.noJpeg != viewer.noJpeg.getValue()) {
      cp.noJpeg = viewer.noJpeg.getValue();
      encodingChange = true;
    }
    viewer.qualityLevel.setParam(options.qualityLevel.getSelectedItem().toString());
    if (cp.qualityLevel != viewer.qualityLevel.getValue()) {
      cp.qualityLevel = viewer.qualityLevel.getValue();
      encodingChange = true;
    }
    viewer.sendLocalUsername.setParam(options.sendLocalUsername.isSelected());

    viewer.viewOnly.setParam(options.viewOnly.isSelected());
    viewer.acceptClipboard.setParam(options.acceptClipboard.isSelected());
    viewer.sendClipboard.setParam(options.sendClipboard.isSelected());
    viewer.fastCopyRect.setParam(options.fastCopyRect.isSelected());
    viewer.acceptBell.setParam(options.acceptBell.isSelected());
    clipboardDialog.setSendingEnabled(viewer.sendClipboard.getValue());
    menuKey = (int)(options.menuKey.getSelectedIndex()+0xFFBE);
    F8Menu.f8.setLabel("Send F"+(menuKey-Keysyms.F1+1));

    shared = options.shared.isSelected();
    setShared(shared);
    viewer.useLocalCursor.setParam(options.useLocalCursor.isSelected());
    if (cp.supportsLocalCursor != viewer.useLocalCursor.getValue()) {
      cp.supportsLocalCursor = viewer.useLocalCursor.getValue();
      encodingChange = true;
      if (desktop != null)
        desktop.resetLocalCursor();
    }
    
    checkEncodings();
  
    if (state() != RFBSTATE_NORMAL) {
      /* Process security types which don't use encryption */
      if (options.encNone.isSelected()) {
        if (options.secNone.isSelected())
          Security.EnableSecType(Security.secTypeNone);
        if (options.secVnc.isSelected())
          Security.EnableSecType(Security.secTypeVncAuth);
        if (options.secPlain.isSelected())
          Security.EnableSecType(Security.secTypePlain);
        if (options.secIdent.isSelected())
          Security.EnableSecType(Security.secTypeIdent);
      } else {
        Security.DisableSecType(Security.secTypeNone);
        Security.DisableSecType(Security.secTypeVncAuth);
        Security.DisableSecType(Security.secTypePlain);
        Security.DisableSecType(Security.secTypeIdent);
      }

      /* Process security types which use TLS encryption */
      if (options.encTLS.isSelected()) {
        if (options.secNone.isSelected())
          Security.EnableSecType(Security.secTypeTLSNone);
        if (options.secVnc.isSelected())
          Security.EnableSecType(Security.secTypeTLSVnc);
        if (options.secPlain.isSelected())
          Security.EnableSecType(Security.secTypeTLSPlain);
        if (options.secIdent.isSelected())
          Security.EnableSecType(Security.secTypeTLSIdent);
      } else {
        Security.DisableSecType(Security.secTypeTLSNone);
        Security.DisableSecType(Security.secTypeTLSVnc);
        Security.DisableSecType(Security.secTypeTLSPlain);
        Security.DisableSecType(Security.secTypeTLSIdent);
      }
  
      /* Process security types which use X509 encryption */
      if (options.encX509.isSelected()) {
        if (options.secNone.isSelected())
          Security.EnableSecType(Security.secTypeX509None);
        if (options.secVnc.isSelected())
          Security.EnableSecType(Security.secTypeX509Vnc);
        if (options.secPlain.isSelected())
          Security.EnableSecType(Security.secTypeX509Plain);
        if (options.secIdent.isSelected())
          Security.EnableSecType(Security.secTypeX509Ident);
      } else {
        Security.DisableSecType(Security.secTypeX509None);
        Security.DisableSecType(Security.secTypeX509Vnc);
        Security.DisableSecType(Security.secTypeX509Plain);
        Security.DisableSecType(Security.secTypeX509Ident);
      }
  
      /* Process *None security types */
      if (options.secNone.isSelected()) {
        if (options.encNone.isSelected())
          Security.EnableSecType(Security.secTypeNone);
        if (options.encTLS.isSelected())
          Security.EnableSecType(Security.secTypeTLSNone);
        if (options.encX509.isSelected())
          Security.EnableSecType(Security.secTypeX509None);
      } else {
        Security.DisableSecType(Security.secTypeNone);
        Security.DisableSecType(Security.secTypeTLSNone);
        Security.DisableSecType(Security.secTypeX509None);
      }
  
      /* Process *Vnc security types */
      if (options.secVnc.isSelected()) {
        if (options.encNone.isSelected())
          Security.EnableSecType(Security.secTypeVncAuth);
        if (options.encTLS.isSelected())
          Security.EnableSecType(Security.secTypeTLSVnc);
        if (options.encX509.isSelected())
          Security.EnableSecType(Security.secTypeX509Vnc);
      } else {
        Security.DisableSecType(Security.secTypeVncAuth);
        Security.DisableSecType(Security.secTypeTLSVnc);
        Security.DisableSecType(Security.secTypeX509Vnc);
      }
  
      /* Process *Plain security types */
      if (options.secPlain.isSelected()) {
        if (options.encNone.isSelected())
          Security.EnableSecType(Security.secTypePlain);
        if (options.encTLS.isSelected())
          Security.EnableSecType(Security.secTypeTLSPlain);
        if (options.encX509.isSelected())
          Security.EnableSecType(Security.secTypeX509Plain);
      } else {
        Security.DisableSecType(Security.secTypePlain);
        Security.DisableSecType(Security.secTypeTLSPlain);
        Security.DisableSecType(Security.secTypeX509Plain);
      }
  
      /* Process *Ident security types */
      if (options.secIdent.isSelected()) {
        if (options.encNone.isSelected())
          Security.EnableSecType(Security.secTypeIdent);
        if (options.encTLS.isSelected())
          Security.EnableSecType(Security.secTypeTLSIdent);
        if (options.encX509.isSelected())
          Security.EnableSecType(Security.secTypeX509Ident);
      } else {
        Security.DisableSecType(Security.secTypeIdent);
        Security.DisableSecType(Security.secTypeTLSIdent);
        Security.DisableSecType(Security.secTypeX509Ident);
      }
  
      CSecurityTLS.x509ca.setParam(options.ca.getText());
      CSecurityTLS.x509crl.setParam(options.crl.getText());
    }
  }

  public void toggleFullScreen() {
    fullScreen = !fullScreen;
    if (!fullScreen) menu.fullScreen.setSelected(false);
    recreateViewport();
  }

  // writeClientCutText() is called from the clipboard dialog
  synchronized public void writeClientCutText(String str, int len) {
    if (state() != RFBSTATE_NORMAL) return;
    writer().writeClientCutText(str,len);
  }

  synchronized public void writeKeyEvent(int keysym, boolean down) {
    if (state() != RFBSTATE_NORMAL) return;
    writer().writeKeyEvent(keysym, down);
  }

  synchronized public void writeKeyEvent(KeyEvent ev) {
    if (ev.getID() != KeyEvent.KEY_PRESSED && !ev.isActionKey())
      return;

    int keysym;

    if (!ev.isActionKey()) {
      vlog.debug("key press "+ev.getKeyChar());
      if (ev.getKeyChar() < 32) {
        // if the ctrl modifier key is down, send the equivalent ASCII since we
        // will send the ctrl modifier anyway

        if ((ev.getModifiers() & KeyEvent.CTRL_MASK) != 0) {
          if ((ev.getModifiers() & KeyEvent.SHIFT_MASK) != 0) {
            keysym = ev.getKeyChar() + 64;
            if (keysym == -1)
              return;
          } else { 
            keysym = ev.getKeyChar() + 96;
            if (keysym == 127) keysym = 95;
          }
        } else {
          switch (ev.getKeyCode()) {
          case KeyEvent.VK_BACK_SPACE: keysym = Keysyms.BackSpace; break;
          case KeyEvent.VK_TAB:        keysym = Keysyms.Tab; break;
          case KeyEvent.VK_ENTER:      keysym = Keysyms.Return; break;
          case KeyEvent.VK_ESCAPE:     keysym = Keysyms.Escape; break;
          default: return;
          }
        }

      } else if (ev.getKeyChar() == 127) {
        keysym = Keysyms.Delete;

      } else {
        keysym = UnicodeToKeysym.translate(ev.getKeyChar());
        if (keysym == -1)
          return;
      }

    } else {
      // KEY_ACTION
      vlog.debug("key action "+ev.getKeyCode());
      switch (ev.getKeyCode()) {
      case KeyEvent.VK_HOME:         keysym = Keysyms.Home; break;
      case KeyEvent.VK_END:          keysym = Keysyms.End; break;
      case KeyEvent.VK_PAGE_UP:      keysym = Keysyms.Page_Up; break;
      case KeyEvent.VK_PAGE_DOWN:    keysym = Keysyms.Page_Down; break;
      case KeyEvent.VK_UP:           keysym = Keysyms.Up; break;
      case KeyEvent.VK_DOWN:         keysym = Keysyms.Down; break;
      case KeyEvent.VK_LEFT:         keysym = Keysyms.Left; break;
      case KeyEvent.VK_RIGHT:        keysym = Keysyms.Right; break;
      case KeyEvent.VK_F1:           keysym = Keysyms.F1; break;
      case KeyEvent.VK_F2:           keysym = Keysyms.F2; break;
      case KeyEvent.VK_F3:           keysym = Keysyms.F3; break;
      case KeyEvent.VK_F4:           keysym = Keysyms.F4; break;
      case KeyEvent.VK_F5:           keysym = Keysyms.F5; break;
      case KeyEvent.VK_F6:           keysym = Keysyms.F6; break;
      case KeyEvent.VK_F7:           keysym = Keysyms.F7; break;
      case KeyEvent.VK_F8:           keysym = Keysyms.F8; break;
      case KeyEvent.VK_F9:           keysym = Keysyms.F9; break;
      case KeyEvent.VK_F10:          keysym = Keysyms.F10; break;
      case KeyEvent.VK_F11:          keysym = Keysyms.F11; break;
      case KeyEvent.VK_F12:          keysym = Keysyms.F12; break;
      case KeyEvent.VK_PRINTSCREEN:  keysym = Keysyms.Print; break;
      case KeyEvent.VK_PAUSE:        keysym = Keysyms.Pause; break;
      case KeyEvent.VK_INSERT:       keysym = Keysyms.Insert; break;
      default: return;
      }
    }

    writeModifiers(ev.getModifiers());
    writeKeyEvent(keysym, true);
    writeKeyEvent(keysym, false);
    writeModifiers(0);
  }


  synchronized public void writePointerEvent(MouseEvent ev) {
    if (state() != RFBSTATE_NORMAL) return;
    int x, y;

    switch (ev.getID()) {
    case MouseEvent.MOUSE_PRESSED:
      buttonMask = 1;
      if ((ev.getModifiers() & KeyEvent.ALT_MASK) != 0) buttonMask = 2;
      if ((ev.getModifiers() & KeyEvent.META_MASK) != 0) buttonMask = 4;
      break;
    case MouseEvent.MOUSE_RELEASED:
      buttonMask = 0;
      break;
    }

    writeModifiers(ev.getModifiers() & ~KeyEvent.ALT_MASK & ~KeyEvent.META_MASK);

    x = ev.getX();
    y = ev.getY();
    if (x < 0) x = 0;
    if (x > cp.width-1) x = cp.width-1;
    if (y < 0) y = 0;
    if (y > cp.height-1) y = cp.height-1;

    writer().writePointerEvent(new Point(x, y), buttonMask);

    if (buttonMask == 0) writeModifiers(0);
  }


  synchronized public void writeWheelEvent(MouseWheelEvent ev) {
    if (state() != RFBSTATE_NORMAL) return;
    int x, y;
    int clicks = ev.getWheelRotation();
    if (clicks < 0) {
      buttonMask = 8;
    } else {
      buttonMask = 16;
    }
    writeModifiers(ev.getModifiers() & ~KeyEvent.ALT_MASK & ~KeyEvent.META_MASK);
    for (int i=0;i<java.lang.Math.abs(clicks);i++) {
      x = ev.getX();
      y = ev.getY();
      writer().writePointerEvent(new Point(x, y), buttonMask);
      buttonMask = 0;
      writer().writePointerEvent(new Point(x, y), buttonMask);
    }
    writeModifiers(0);

  }


  void writeModifiers(int m) {
    if ((m & Event.SHIFT_MASK) != (pressedModifiers & Event.SHIFT_MASK))
      writeKeyEvent(Keysyms.Shift_L, (m & Event.SHIFT_MASK) != 0);
    if ((m & Event.CTRL_MASK) != (pressedModifiers & Event.CTRL_MASK))
      writeKeyEvent(Keysyms.Control_L, (m & Event.CTRL_MASK) != 0);
    if ((m & Event.ALT_MASK) != (pressedModifiers & Event.ALT_MASK))
      writeKeyEvent(Keysyms.Alt_L, (m & Event.ALT_MASK) != 0);
    if ((m & Event.META_MASK) != (pressedModifiers & Event.META_MASK))
      writeKeyEvent(Keysyms.Meta_L, (m & Event.META_MASK) != 0);
    pressedModifiers = m;
  }


  ////////////////////////////////////////////////////////////////////
  // The following methods are called from both RFB and GUI threads

  // checkEncodings() sends a setEncodings message if one is needed.
  synchronized private void checkEncodings() {
    if (encodingChange && state() == RFBSTATE_NORMAL) {
      vlog.info("Using "+Encodings.encodingName(currentEncoding)+" encoding");
      writer().writeSetEncodings(currentEncoding, true);
      encodingChange = false;
    }
  }

  // the following never change so need no synchronization:
  JavaInStream jis;
  JavaOutStream jos;


  // viewer object is only ever accessed by the GUI thread so needs no
  // synchronization (except for one test in DesktopWindow - see comment
  // there).
  VncViewer viewer;

  // access to desktop by different threads is specified in DesktopWindow

  // the following need no synchronization:

  ClassLoader cl = this.getClass().getClassLoader();
  ImageIcon logo = new ImageIcon(cl.getResource("com/tigervnc/vncviewer/tigervnc.png"));
  public static UserPasswdGetter upg;
  public UserMsgBox msg;

  // shuttingDown is set by the GUI thread and only ever tested by the RFB
  // thread after the window has been destroyed.
  boolean shuttingDown;

  // reading and writing int and boolean is atomic in java, so no
  // synchronization of the following flags is needed:
  int currentEncoding, lastServerEncoding;
  
  int lowColourLevel;


  // All menu, options, about and info stuff is done in the GUI thread (apart
  // from when constructed).
  F8Menu menu;
  OptionsDialog options;

  // clipboard sync issues?
  ClipboardDialog clipboardDialog;

  // the following are only ever accessed by the GUI thread:
  int buttonMask;
  int pressedModifiers;

  public String serverHost;
  public int serverPort;
  public int menuKey;
  PixelFormat serverPF;
  ViewportFrame viewport;
  DesktopWindow desktop;
  PixelFormat fullColourPF;
  boolean fullColour;
  boolean autoSelect;
  boolean shared;
  boolean formatChange;
  boolean encodingChange;
  boolean sameMachine;
  boolean fullScreen;
  boolean reverseConnection;
  boolean firstUpdate;
  boolean pendingUpdate;
  
  static LogWriter vlog = new LogWriter("CConn");
}
