/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2013 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright (C) 2011-2013 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2011-2017 Brian P. Hinz
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

import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.event.*;
import java.awt.Toolkit;

import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import javax.swing.*;
import javax.swing.ImageIcon;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.util.*;
import java.util.prefs.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Point;
import com.tigervnc.rfb.Exception;
import com.tigervnc.network.Socket;
import com.tigervnc.network.TcpSocket;

import static com.tigervnc.vncviewer.Parameters.*;

public class CConn extends CConnection implements 
  FdInStreamBlockCallback, ActionListener {

  // 8 colours (1 bit per component)
  static final PixelFormat verylowColorPF =
    new PixelFormat(8, 3, false, true, 1, 1, 1, 2, 1, 0);

  // 64 colours (2 bits per component)
  static final PixelFormat lowColorPF =
    new PixelFormat(8, 6, false, true, 3, 3, 3, 4, 2, 0);

  // 256 colours (2-3 bits per component)
  static final PixelFormat mediumColorPF =
    new PixelFormat(8, 8, false, true, 7, 7, 3, 5, 2, 0);

  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the RFB thread

  public CConn(String vncServerName, Socket socket)
  {
    serverHost = null; serverPort = 0; desktop = null;
    pendingPFChange = false;
    currentEncoding = Encodings.encodingTight; lastServerEncoding = -1;
    formatChange = false; encodingChange = false;
    firstUpdate = true; pendingUpdate = false; continuousUpdates = false;
    forceNonincremental = true; supportsSyncFence = false;

    setShared(shared.getValue());
    sock = socket;

    int encNum = Encodings.encodingNum(preferredEncoding.getValue());
    if (encNum != -1)
      currentEncoding = encNum;

    cp.supportsLocalCursor = true;
    if (VncViewer.os.contains("windows"))
      // JRE on Windows does not support alpha cursor
      cp.supportsLocalCursorWithAlpha = false;
    else
      cp.supportsLocalCursorWithAlpha = true;

    cp.supportsDesktopResize = true;
    cp.supportsExtendedDesktopSize = true;
    cp.supportsDesktopRename = true;

    cp.supportsSetDesktopSize = false;
    cp.supportsClientRedirect = true;

    if (customCompressLevel.getValue())
      cp.compressLevel = compressLevel.getValue();
    else
      cp.compressLevel = -1;

    if (!noJpeg.getValue())
      cp.qualityLevel = qualityLevel.getValue();
    else
      cp.qualityLevel = -1;

    if (sock == null) {
      setServerName(Hostname.getHost(vncServerName));
      setServerPort(Hostname.getPort(vncServerName));
      try {
        if (tunnel.getValue() || !via.getValue().isEmpty()) {
          int localPort = TcpSocket.findFreeTcpPort();
          if (localPort == 0)
            throw new Exception("Could not obtain free TCP port");
          Tunnel.createTunnel(this, localPort);
          sock = new TcpSocket("localhost", localPort);
        } else {
          sock = new TcpSocket(getServerName(), getServerPort());
        }
      } catch (java.lang.Exception e) {
        throw new Exception(e.getMessage());
      }
      vlog.info("connected to host "+getServerName()+" port "+getServerPort());
    } else {
      String name = sock.getPeerEndpoint();
      if (listenMode.getValue())
        vlog.info("Accepted connection from " + name);
      else
        vlog.info("connected to host "+Hostname.getHost(name)+" port "+Hostname.getPort(name));
    }

    // See callback below
    sock.inStream().setBlockCallback(this);

    setStreams(sock.inStream(), sock.outStream());

    initialiseProtocol();

    OptionsDialog.addCallback("handleOptions", this);
  }

  public void refreshFramebuffer()
  {
    forceNonincremental = true;

    // Without fences, we cannot safely trigger an update request directly
    // but must wait for the next update to arrive.
    if (supportsSyncFence)
      requestNewUpdate();
  }

  public String connectionInfo() {
    String info = new String("Desktop name: %s%n"+
                             "Host: %s:%d%n"+
                             "Size: %dx%d%n"+
                             "Pixel format: %s%n"+
                             "  (server default: %s)%n"+
                             "Requested encoding: %s%n"+
                             "Last used encoding: %s%n"+
                             "Line speed estimate: %d kbit/s%n"+
                             "Protocol version: %d.%d%n"+
                             "Security method: %s [%s]%n");
    String infoText =
      String.format(info, cp.name(),
                    sock.getPeerName(), sock.getPeerPort(),
                    cp.width, cp.height,
                    cp.pf().print(),
                    serverPF.print(),
                    Encodings.encodingName(currentEncoding),
                    Encodings.encodingName(lastServerEncoding),
                    sock.inStream().kbitsPerSecond(),
                    cp.majorVersion, cp.minorVersion,
                    Security.secTypeName(csecurity.getType()),
                    csecurity.description());

    return infoText;
  }

  // The RFB core is not properly asynchronous, so it calls this callback
  // whenever it needs to block to wait for more data. Since FLTK is
  // monitoring the socket, we just make sure FLTK gets to run.

  public void blockCallback() {
    try {
      synchronized(this) {
        wait(1);
      }
    } catch (java.lang.InterruptedException e) {
      throw new Exception(e.getMessage());
    }
  }

  ////////////////////// CConnection callback methods //////////////////////

  // serverInit() is called when the serverInit message has been received.  At
  // this point we create the desktop window and display it.  We also tell the
  // server the pixel format and encodings to use and request the first update.
  public void serverInit()
  {
    super.serverInit();

    // If using AutoSelect with old servers, start in FullColor
    // mode. See comment in autoSelectFormatAndEncoding.
    if (cp.beforeVersion(3, 8) && autoSelect.getValue())
      fullColor.setParam(true);

    serverPF = cp.pf();

    desktop = new DesktopWindow(cp.width, cp.height, cp.name(), serverPF, this);
    fullColorPF = desktop.getPreferredPF();

    // Force a switch to the format and encoding we'd like
    formatChange = true; encodingChange = true;

    // And kick off the update cycle
    requestNewUpdate();

    // This initial update request is a bit of a corner case, so we need
    // to help out setting the correct format here.
    assert(pendingPFChange);
    cp.setPF(pendingPF);
    pendingPFChange = false;
  }

  // setDesktopSize() is called when the desktop size changes (including when
  // it is set initially).
  public void setDesktopSize(int w, int h)
  {
    super.setDesktopSize(w, h);
    resizeFramebuffer();
  }

  // setExtendedDesktopSize() is a more advanced version of setDesktopSize()
  public void setExtendedDesktopSize(int reason, int result, int w, int h,
                                     ScreenSet layout)
  {
    super.setExtendedDesktopSize(reason, result, w, h, layout);

    if ((reason == screenTypes.reasonClient) &&
        (result != screenTypes.resultSuccess)) {
      vlog.error("SetDesktopSize failed: " + result);
      return;
    }

    resizeFramebuffer();
  }

  // clientRedirect() migrates the client to another host/port
  public void clientRedirect(int port, String host, String x509subject)
  {
    try {
      sock.close();
      sock = new TcpSocket(host, port);
      vlog.info("Redirected to "+host+":"+port);
      setServerName(host);
      setServerPort(port);
      sock.inStream().setBlockCallback(this);
      setStreams(sock.inStream(), sock.outStream());
      if (desktop != null)
        desktop.dispose();
      initialiseProtocol();
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage());
    }
  }

  // setName() is called when the desktop name changes
  public void setName(String name)
  {
    super.setName(name);
    if (desktop != null)
      desktop.setName(name);
  }

  // framebufferUpdateStart() is called at the beginning of an update.
  // Here we try to send out a new framebuffer update request so that the
  // next update can be sent out in parallel with us decoding the current
  // one.
  public void framebufferUpdateStart()
  {
    ModifiablePixelBuffer pb;
    PlatformPixelBuffer ppb;

    super.framebufferUpdateStart();

    // Note: This might not be true if sync fences are supported
    pendingUpdate = false;

    requestNewUpdate();

    // We might still be rendering the previous update
    pb = getFramebuffer();
    assert(pb != null);
    ppb = (PlatformPixelBuffer)pb;
    assert(ppb != null);

    //FIXME
  }

  // framebufferUpdateEnd() is called at the end of an update.
  // For each rectangle, the FdInStream will have timed the speed
  // of the connection, allowing us to select format and encoding
  // appropriately, and then request another incremental update.
  public void framebufferUpdateEnd()
  {
    super.framebufferUpdateEnd();

    desktop.updateWindow();

    if (firstUpdate) {
      // We need fences to make extra update requests and continuous
      // updates "safe". See fence() for the next step.
      if (cp.supportsFence)
        writer().writeFence(fenceTypes.fenceFlagRequest | fenceTypes.fenceFlagSyncNext, 0, null);

      firstUpdate = false;
    }

    // A format change has been scheduled and we are now past the update
    // with the old format. Time to active the new one.
    if (pendingPFChange) {
      cp.setPF(pendingPF);
      pendingPFChange = false;
    }

    // Compute new settings based on updated bandwidth values
    if (autoSelect.getValue())
      autoSelectFormatAndEncoding();
  }

  // The rest of the callbacks are fairly self-explanatory...

  public void setColourMapEntries(int firstColor, int nColors, int[] rgbs)
  {
    vlog.error("Invalid SetColourMapEntries from server!");
  }

  public void bell()
  {
    if (acceptBell.getValue())
      desktop.getToolkit().beep();
  }

  public void serverCutText(String str, int len)
  {
    StringSelection buffer;

    if (!acceptClipboard.getValue())
      return;

    ClipboardDialog.serverCutText(str);
  }

  public void dataRect(Rect r, int encoding)
  {
    sock.inStream().startTiming();

    if (encoding != Encodings.encodingCopyRect)
      lastServerEncoding = encoding;

    super.dataRect(r, encoding);

    sock.inStream().stopTiming();
  }

  public void setCursor(int width, int height, Point hotspot,
                        byte[] data)
  {
    desktop.setCursor(width, height, hotspot, data);
  }

  public void fence(int flags, int len, byte[] data)
  {
    // can't call super.super.fence(flags, len, data);
    cp.supportsFence = true;

    if ((flags & fenceTypes.fenceFlagRequest) != 0) {
      // We handle everything synchronously so we trivially honor these modes
      flags = flags & (fenceTypes.fenceFlagBlockBefore | fenceTypes.fenceFlagBlockAfter);

      writer().writeFence(flags, len, data);
      return;
    }

    if (len == 0) {
      // Initial probe
      if ((flags & fenceTypes.fenceFlagSyncNext) != 0) {
        supportsSyncFence = true;

        if (cp.supportsContinuousUpdates) {
          vlog.info("Enabling continuous updates");
          continuousUpdates = true;
          writer().writeEnableContinuousUpdates(true, 0, 0, cp.width, cp.height);
        }
      }
    } else {
      // Pixel format change
      MemInStream memStream = new MemInStream(data, 0, len);
      PixelFormat pf = new PixelFormat();

      pf.read(memStream);

      cp.setPF(pf);
    }
  }

  ////////////////////// Internal methods //////////////////////
  private void resizeFramebuffer()
  {
    if (desktop == null)
      return;

    if (continuousUpdates)
      writer().writeEnableContinuousUpdates(true, 0, 0, cp.width, cp.height);

    desktop.resizeFramebuffer(cp.width, cp.height);
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
  private void autoSelectFormatAndEncoding()
  {
    long kbitsPerSecond = sock.inStream().kbitsPerSecond();
    long timeWaited = sock.inStream().timeWaited();
    boolean newFullColor = fullColor.getValue();
    int newQualityLevel = qualityLevel.getValue();

    // Always use Tight
    if (currentEncoding != Encodings.encodingTight) {
      currentEncoding = Encodings.encodingTight;
      encodingChange = true;
    }

    // Check that we have a decent bandwidth measurement
    if ((kbitsPerSecond == 0) || (timeWaited < 100))
      return;

    // Select appropriate quality level
    if (!noJpeg.getValue()) {
      if (kbitsPerSecond > 16000)
        newQualityLevel = 8;
      else
        newQualityLevel = 6;

      if (newQualityLevel != qualityLevel.getValue()) {
        vlog.info("Throughput "+kbitsPerSecond+
                  " kbit/s - changing to quality "+newQualityLevel);
        cp.qualityLevel = newQualityLevel;
        qualityLevel.setParam(newQualityLevel);
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
    newFullColor = (kbitsPerSecond > 256);
    if (newFullColor != fullColor.getValue()) {
      vlog.info("Throughput "+kbitsPerSecond+
                " kbit/s - full color is now "+
                (newFullColor ? "enabled" : "disabled"));
      fullColor.setParam(newFullColor);
      formatChange = true;
    }
  }

  // checkEncodings() sends a setEncodings message if one is needed.
  private void checkEncodings()
  {
    if (encodingChange && (writer() != null)) {
      vlog.info("Using " + Encodings.encodingName(currentEncoding) +
        " encoding");
      writer().writeSetEncodings(currentEncoding, true);
      encodingChange = false;
    }
  }

  // requestNewUpdate() requests an update from the server, having set the
  // format and encoding appropriately.
  private void requestNewUpdate()
  {
    if (formatChange) {
      PixelFormat pf;

      /* Catch incorrect requestNewUpdate calls */
      assert(!pendingUpdate || supportsSyncFence);

      if (fullColor.getValue()) {
        pf = fullColorPF;
      } else {
        if (lowColorLevel.getValue() == 0) {
          pf = verylowColorPF;
        } else if (lowColorLevel.getValue() == 1) {
          pf = lowColorPF;
        } else {
          pf = mediumColorPF;
        }
      }

      if (supportsSyncFence) {
        // We let the fence carry the pixel format and switch once we
        // get the response back. That way we will be synchronised with
        // when the server switches.
        MemOutStream memStream = new MemOutStream();

        pf.write(memStream);

        writer().writeFence(fenceTypes.fenceFlagRequest | fenceTypes.fenceFlagSyncNext,
                            memStream.length(), (byte[])memStream.data());
      } else {
        // New requests are sent out at the start of processing the last
        // one, so we cannot switch our internal format right now (doing so
        // would mean misdecoding the current update).
        pendingPFChange = true;
        pendingPF = pf;
      }

      String str = pf.print();
      vlog.info("Using pixel format " + str);
      writer().writeSetPixelFormat(pf);

      formatChange = false;
    }

    checkEncodings();

    if (forceNonincremental || !continuousUpdates) {
      pendingUpdate = true;
      writer().writeFramebufferUpdateRequest(new Rect(0, 0, cp.width, cp.height),
                                                 !forceNonincremental);
    }

    forceNonincremental = false;
  }

  public void handleOptions()
  {

    // Checking all the details of the current set of encodings is just
    // a pain. Assume something has changed, as resending the encoding
    // list is cheap. Avoid overriding what the auto logic has selected
    // though.
    if (!autoSelect.getValue()) {
      int encNum = Encodings.encodingNum(preferredEncoding.getValue());

      if (encNum != -1)
        this.currentEncoding = encNum;
    }

    this.cp.supportsLocalCursor = true;

    if (customCompressLevel.getValue())
      this.cp.compressLevel = compressLevel.getValue();
    else
      this.cp.compressLevel = -1;

    if (!noJpeg.getValue() && !autoSelect.getValue())
      this.cp.qualityLevel = qualityLevel.getValue();
    else
      this.cp.qualityLevel = -1;

    this.encodingChange = true;

    // Format changes refreshes the entire screen though and are therefore
    // very costly. It's probably worth the effort to see if it is necessary
    // here.
    PixelFormat pf;

    if (fullColor.getValue()) {
      pf = fullColorPF;
    } else {
      if (lowColorLevel.getValue() == 0)
        pf = verylowColorPF;
      else if (lowColorLevel.getValue() == 1)
        pf = lowColorPF;
      else
        pf = mediumColorPF;
    }

    if (!pf.equal(this.cp.pf())) {
      this.formatChange = true;

      // Without fences, we cannot safely trigger an update request directly
      // but must wait for the next update to arrive.
      if (this.supportsSyncFence)
        this.requestNewUpdate();
    }

  }

  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the GUI thread

  // close() shuts down the socket, thus waking up the RFB thread.
  public void close() {
    if (closeListener != null) {
      embed.setParam(true);
      JFrame f =
        (JFrame)SwingUtilities.getAncestorOfClass(JFrame.class, desktop);
      if (f != null)
        f.dispatchEvent(new WindowEvent(f, WindowEvent.WINDOW_CLOSING));
    }
    shuttingDown = true;
    try {
      if (sock != null)
        sock.shutdown();
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage());
    }
  }

  // writeClientCutText() is called from the clipboard dialog
  public void writeClientCutText(String str, int len) {
    if (state() != RFBSTATE_NORMAL || shuttingDown)
      return;
    writer().writeClientCutText(str, len);
  }

  // this is a special ActionListener passed in by the
  // Java Plug-in software to control applet's close behavior
  public void setCloseListener(ActionListener cl) {
    closeListener = cl;
  }

  public void actionPerformed(ActionEvent e) {}

  public Socket getSocket() {
    return sock;
  }

  ////////////////////////////////////////////////////////////////////
  // The following methods are called from both RFB and GUI threads

  // the following never change so need no synchronization:

  // access to desktop by different threads is specified in DesktopWindow

  // the following need no synchronization:

  // shuttingDown is set by the GUI thread and only ever tested by the RFB
  // thread after the window has been destroyed.
  boolean shuttingDown = false;

  // reading and writing int and boolean is atomic in java, so no
  // synchronization of the following flags is needed:


  // All menu, options, about and info stuff is done in the GUI thread (apart
  // from when constructed).

  // the following are only ever accessed by the GUI thread:
  private String serverHost;
  private int serverPort;
  private Socket sock;

  protected DesktopWindow desktop;

  private PixelFormat serverPF;
  private PixelFormat fullColorPF;

  private boolean pendingPFChange;
  private PixelFormat pendingPF;

  private int currentEncoding, lastServerEncoding;

  private boolean formatChange;
  private boolean encodingChange;

  private boolean firstUpdate;
  private boolean pendingUpdate;
  private boolean continuousUpdates;

  private boolean forceNonincremental;

  private boolean supportsSyncFence;

  public ActionListener closeListener = null;

  static LogWriter vlog = new LogWriter("CConn");
}
