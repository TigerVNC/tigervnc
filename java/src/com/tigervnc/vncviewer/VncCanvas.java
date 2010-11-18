//
//  Copyright (C) 2004 Horizon Wimba.  All Rights Reserved.
//  Copyright (C) 2001-2003 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001,2002 Constantin Kaplinsky.  All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

package com.tigervnc.vncviewer;

import com.tigervnc.decoder.CoRREDecoder;
import com.tigervnc.decoder.CopyRectDecoder;
import com.tigervnc.decoder.HextileDecoder;
import com.tigervnc.decoder.RREDecoder;
import com.tigervnc.decoder.RawDecoder;
import com.tigervnc.decoder.TightDecoder;
import com.tigervnc.decoder.ZRLEDecoder;
import com.tigervnc.decoder.ZlibDecoder;
import com.tigervnc.decoder.common.Repaintable;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.lang.*;
import java.util.zip.*;


//
// VncCanvas is a subclass of Canvas which draws a VNC desktop on it.
//

class VncCanvas extends Canvas
  implements KeyListener, MouseListener, MouseMotionListener, Repaintable, Runnable {

  VncViewer viewer;
  RfbProto rfb;
  ColorModel cm8, cm24;
  int bytesPixel;

  int maxWidth = 0, maxHeight = 0;
  int scalingFactor;
  int scaledWidth, scaledHeight;

  Image memImage;
  Graphics memGraphics;

  //
  // Decoders
  //

  RawDecoder rawDecoder;
  RREDecoder rreDecoder;
  CoRREDecoder correDecoder;
  ZlibDecoder zlibDecoder;
  HextileDecoder hextileDecoder;
  ZRLEDecoder zrleDecoder;
  TightDecoder tightDecoder;
  CopyRectDecoder copyRectDecoder;

  // Base decoder decoders array
  RawDecoder []decoders = null;

  // Update statistics.
  long statStartTime;           // time on first framebufferUpdateRequest
  long statNumUpdates;           // counter for FramebufferUpdate messages
  long statNumTotalRects;        // rectangles in FramebufferUpdate messages
  long statNumPixelRects;        // the same, but excluding pseudo-rectangles
  long statNumRectsTight;        // Tight-encoded rectangles (including JPEG)
  long statNumRectsTightJPEG;    // JPEG-compressed Tight-encoded rectangles
  long statNumRectsZRLE;         // ZRLE-encoded rectangles
  long statNumRectsHextile;      // Hextile-encoded rectangles
  long statNumRectsRaw;          // Raw-encoded rectangles
  long statNumRectsCopy;         // CopyRect rectangles
  long statNumBytesEncoded;      // number of bytes in updates, as received
  long statNumBytesDecoded;      // number of bytes, as if Raw encoding was used

  // True if we process keyboard and mouse events.
  boolean inputEnabled;

  // True if was no one auto resize of canvas
  boolean isFirstSizeAutoUpdate = true;

  // Members for limiting sending mouse events to server
  long lastMouseEventSendTime = System.currentTimeMillis();
  long mouseMaxFreq = 20;

  //
  // The constructors.
  //

  public VncCanvas(VncViewer v, int maxWidth_, int maxHeight_)
    throws IOException {

    viewer = v;
    maxWidth = maxWidth_;
    maxHeight = maxHeight_;

    rfb = viewer.rfb;
    scalingFactor = viewer.options.scalingFactor;

    cm8 = new DirectColorModel(8, 7, (7 << 3), (3 << 6));
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);

    //
    // Create decoders
    //

    // Input stream for decoders
    RfbInputStream rfbis = new RfbInputStream(rfb);
    // Create output stream for session recording
    RecordOutputStream ros = new RecordOutputStream(rfb);

    rawDecoder = new RawDecoder(memGraphics, rfbis);
    rreDecoder = new RREDecoder(memGraphics, rfbis);
    correDecoder = new CoRREDecoder(memGraphics, rfbis);
    hextileDecoder = new HextileDecoder(memGraphics, rfbis);
    tightDecoder = new TightDecoder(memGraphics, rfbis);
    zlibDecoder = new ZlibDecoder(memGraphics, rfbis);
    zrleDecoder = new ZRLEDecoder(memGraphics, rfbis);
    copyRectDecoder = new CopyRectDecoder(memGraphics, rfbis);

    //
    // Set data for decoders that needs extra parameters
    //

    hextileDecoder.setRepainableControl(this);
    tightDecoder.setRepainableControl(this);

    //
    // Create array that contains our decoders
    //

    decoders = new RawDecoder[8];
    decoders[0] = rawDecoder;
    decoders[1] = rreDecoder;
    decoders[2] = correDecoder;
    decoders[3] = hextileDecoder;
    decoders[4] = zlibDecoder;
    decoders[5] = tightDecoder;
    decoders[6] = zrleDecoder;
    decoders[7] = copyRectDecoder;

    //
    // Set session recorder for decoders
    //

    for (int i = 0; i < decoders.length; i++) {
      decoders[i].setDataOutputStream(ros);
    }

    setPixelFormat();

    inputEnabled = false;
    if (!viewer.options.viewOnly)
      enableInput(true);

    // Enable mouse and keyboard event listeners.
    addKeyListener(this);
    addMouseListener(this);
    addMouseMotionListener(this);

    // Create thread, that will send mouse movement events
    // to VNC server.
    Thread mouseThread = new Thread(this);
    mouseThread.start();
  }

  public VncCanvas(VncViewer v) throws IOException {
    this(v, 0, 0);
  }

  //
  // Callback methods to determine geometry of our Component.
  //

  public Dimension getPreferredSize() {
    return new Dimension(scaledWidth, scaledHeight);
  }

  public Dimension getMinimumSize() {
    return new Dimension(scaledWidth, scaledHeight);
  }

  public Dimension getMaximumSize() {
    return new Dimension(scaledWidth, scaledHeight);
  }

  //
  // All painting is performed here.
  //

  public void update(Graphics g) {
    paint(g);
  }

  public void paint(Graphics g) {
    synchronized(memImage) {
      if (rfb.framebufferWidth == scaledWidth) {
        g.drawImage(memImage, 0, 0, null);
      } else {
        paintScaledFrameBuffer(g);
      }
    }
    if (showSoftCursor) {
      int x0 = cursorX - hotX, y0 = cursorY - hotY;
      Rectangle r = new Rectangle(x0, y0, cursorWidth, cursorHeight);
      if (r.intersects(g.getClipBounds())) {
	g.drawImage(softCursor, x0, y0, null);
      }
    }
  }

  public void paintScaledFrameBuffer(Graphics g) {
    g.drawImage(memImage, 0, 0, scaledWidth, scaledHeight, null);
  }

  //
  // Start/stop receiving mouse events. Keyboard events are received
  // even in view-only mode, because we want to map the 'r' key to the
  // screen refreshing function.
  //

  public synchronized void enableInput(boolean enable) {
    if (enable && !inputEnabled) {
      inputEnabled = true;
      if (viewer.showControls) {
	viewer.buttonPanel.enableRemoteAccessControls(true);
      }
      createSoftCursor();	// scaled cursor
    } else if (!enable && inputEnabled) {
      inputEnabled = false;
      if (viewer.showControls) {
	viewer.buttonPanel.enableRemoteAccessControls(false);
      }
      createSoftCursor();	// non-scaled cursor
    }
  }

  public void setPixelFormat() throws IOException {
    if (viewer.options.eightBitColors) {
      rfb.writeSetPixelFormat(8, 8, false, true, 7, 7, 3, 0, 3, 6);
      bytesPixel = 1;
    } else {
      rfb.writeSetPixelFormat(32, 24, false, true, 255, 255, 255, 16, 8, 0);
      bytesPixel = 4;
    }
    updateFramebufferSize();
  }

  void setScalingFactor(int sf) {
    scalingFactor = sf;
    updateFramebufferSize();
    invalidate();
  }

  void updateFramebufferSize() {

    // Useful shortcuts.
    int fbWidth = rfb.framebufferWidth;
    int fbHeight = rfb.framebufferHeight;

    // FIXME: This part of code must be in VncViewer i think
    if (viewer.options.autoScale) {
      if (viewer.inAnApplet) {
        maxWidth = viewer.getWidth();
        maxHeight = viewer.getHeight();
      } else {
        if (viewer.vncFrame != null) {
          if (isFirstSizeAutoUpdate) {
            isFirstSizeAutoUpdate = false;
            Dimension screenSize = viewer.vncFrame.getToolkit().getScreenSize();
            maxWidth = (int)screenSize.getWidth() - 100;
            maxHeight = (int)screenSize.getHeight() - 100;
            viewer.vncFrame.setSize(maxWidth, maxHeight);
          } else {
            viewer.desktopScrollPane.doLayout();
            maxWidth = viewer.desktopScrollPane.getWidth();
            maxHeight = viewer.desktopScrollPane.getHeight();
          }
        } else {
          maxWidth = fbWidth;
          maxHeight = fbHeight;
        }
      }
      int f1 = maxWidth * 100 / fbWidth;
      int f2 = maxHeight * 100 / fbHeight;
      scalingFactor = Math.min(f1, f2);
      if (scalingFactor > 100)
	scalingFactor = 100;
      System.out.println("Scaling desktop at " + scalingFactor + "%");
    }

    // Update scaled framebuffer geometry.
    scaledWidth = (fbWidth * scalingFactor + 50) / 100;
    scaledHeight = (fbHeight * scalingFactor + 50) / 100;

    // Create new off-screen image either if it does not exist, or if
    // its geometry should be changed. It's not necessary to replace
    // existing image if only pixel format should be changed.
    if (memImage == null) {
      memImage = viewer.vncContainer.createImage(fbWidth, fbHeight);
      memGraphics = memImage.getGraphics();
    } else if (memImage.getWidth(null) != fbWidth ||
	       memImage.getHeight(null) != fbHeight) {
      synchronized(memImage) {
	memImage = viewer.vncContainer.createImage(fbWidth, fbHeight);
	memGraphics = memImage.getGraphics();
      }
    }

    //
    // Update decoders
    //

    //
    // FIXME: Why decoders can be null here?
    //

    if (decoders != null) {
      for (int i = 0; i < decoders.length; i++) {
        //
        // Set changes to every decoder that we can use
        //

        decoders[i].setBPP(bytesPixel);
        decoders[i].setFrameBufferSize(fbWidth, fbHeight);
        decoders[i].setGraphics(memGraphics);

        //
        // Update decoder
        //

        decoders[i].update();
      }
    }

    // FIXME: This part of code must be in VncViewer i think
    // Update the size of desktop containers.
    if (viewer.inSeparateFrame) {
      if (viewer.desktopScrollPane != null) {
        if (!viewer.options.autoScale) {
          resizeDesktopFrame();
        } else {
          setSize(scaledWidth, scaledHeight);
          viewer.desktopScrollPane.setSize(maxWidth + 200,
                                           maxHeight + 200);
        }
      }
    } else {
      setSize(scaledWidth, scaledHeight);
    }
    viewer.moveFocusToDesktop();
  }

  void resizeDesktopFrame() {
    setSize(scaledWidth, scaledHeight);

    // FIXME: Find a better way to determine correct size of a
    // ScrollPane.  -- const
    Insets insets = viewer.desktopScrollPane.getInsets();
    viewer.desktopScrollPane.setSize(scaledWidth +
				     2 * Math.min(insets.left, insets.right),
				     scaledHeight +
				     2 * Math.min(insets.top, insets.bottom));

    viewer.vncFrame.pack();

    // Try to limit the frame size to the screen size.

    Dimension screenSize = viewer.vncFrame.getToolkit().getScreenSize();
    Dimension frameSize = viewer.vncFrame.getSize();
    Dimension newSize = frameSize;

    // Reduce Screen Size by 30 pixels in each direction;
    // This is a (poor) attempt to account for
    //     1) Menu bar on Macintosh (should really also account for
    //        Dock on OSX).  Usually 22px on top of screen.
    //     2) Taxkbar on Windows (usually about 28 px on bottom)
    //     3) Other obstructions.

    screenSize.height -= 30;
    screenSize.width  -= 30;

    boolean needToResizeFrame = false;
    if (frameSize.height > screenSize.height) {
      newSize.height = screenSize.height;
      needToResizeFrame = true;
    }
    if (frameSize.width > screenSize.width) {
      newSize.width = screenSize.width;
      needToResizeFrame = true;
    }
    if (needToResizeFrame) {
      viewer.vncFrame.setSize(newSize);
    }

    viewer.desktopScrollPane.doLayout();
  }

  //
  // processNormalProtocol() - executed by the rfbThread to deal with the
  // RFB socket.
  //

  public void processNormalProtocol() throws Exception {

    // Start/stop session recording if necessary.
    viewer.checkRecordingStatus();

    rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
				      rfb.framebufferHeight, false);

    resetStats();
    boolean statsRestarted = false;

    //
    // main dispatch loop
    //

    while (true) {

      // Read message type from the server.
      int msgType = rfb.readServerMessageType();

      // Process the message depending on its type.
      switch (msgType) {
      case RfbProto.FramebufferUpdate:

        if (statNumUpdates == viewer.debugStatsExcludeUpdates &&
            !statsRestarted) {
          resetStats();
          statsRestarted = true;
        } else if (statNumUpdates == viewer.debugStatsMeasureUpdates &&
                   statsRestarted) {
          viewer.disconnect();
        }

	rfb.readFramebufferUpdate();
	statNumUpdates++;

	boolean cursorPosReceived = false;

	for (int i = 0; i < rfb.updateNRects; i++) {

	  rfb.readFramebufferUpdateRectHdr();
	  statNumTotalRects++;
	  int rx = rfb.updateRectX, ry = rfb.updateRectY;
	  int rw = rfb.updateRectW, rh = rfb.updateRectH;

	  if (rfb.updateRectEncoding == rfb.EncodingLastRect)
	    break;

	  if (rfb.updateRectEncoding == rfb.EncodingNewFBSize) {
	    rfb.setFramebufferSize(rw, rh);
	    updateFramebufferSize();
	    break;
	  }

	  if (rfb.updateRectEncoding == rfb.EncodingXCursor ||
	      rfb.updateRectEncoding == rfb.EncodingRichCursor) {
	    handleCursorShapeUpdate(rfb.updateRectEncoding, rx, ry, rw, rh);
	    continue;
	  }

	  if (rfb.updateRectEncoding == rfb.EncodingPointerPos) {
	    softCursorMove(rx, ry);
	    cursorPosReceived = true;
	    continue;
	  }

          long numBytesReadBefore = rfb.getNumBytesRead();

          rfb.startTiming();

	  switch (rfb.updateRectEncoding) {
	  case RfbProto.EncodingRaw:
	    statNumRectsRaw++;
	    handleRawRect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingCopyRect:
	    statNumRectsCopy++;
	    handleCopyRect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingRRE:
	    handleRRERect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingCoRRE:
	    handleCoRRERect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingHextile:
	    statNumRectsHextile++;
	    handleHextileRect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingZRLE:
	    statNumRectsZRLE++;
	    handleZRLERect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingZlib:
            handleZlibRect(rx, ry, rw, rh);
	    break;
	  case RfbProto.EncodingTight:
            if (tightDecoder != null) {
	      statNumRectsTightJPEG = tightDecoder.getNumJPEGRects();
              //statNumRectsTight = tightDecoder.getNumTightRects();
            }
            statNumRectsTight++;
	    handleTightRect(rx, ry, rw, rh);
	    break;
	  default:
	    throw new Exception("Unknown RFB rectangle encoding " +
				rfb.updateRectEncoding);
	  }

          rfb.stopTiming();

          statNumPixelRects++;
          statNumBytesDecoded += rw * rh * bytesPixel;
          statNumBytesEncoded +=
            (int)(rfb.getNumBytesRead() - numBytesReadBefore);
	}

	boolean fullUpdateNeeded = false;

	// Start/stop session recording if necessary. Request full
	// update if a new session file was opened.
	if (viewer.checkRecordingStatus())
	  fullUpdateNeeded = true;

	// Defer framebuffer update request if necessary. But wake up
	// immediately on keyboard or mouse event. Also, don't sleep
	// if there is some data to receive, or if the last update
	// included a PointerPos message.
	if (viewer.deferUpdateRequests > 0 &&
	    rfb.available() == 0 && !cursorPosReceived) {
	  synchronized(rfb) {
	    try {
	      rfb.wait(viewer.deferUpdateRequests);
	    } catch (InterruptedException e) {
	    }
	  }
	}

        viewer.autoSelectEncodings();

	// Before requesting framebuffer update, check if the pixel
	// format should be changed.
	if (viewer.options.eightBitColors != (bytesPixel == 1)) {
          // Pixel format should be changed.
          setPixelFormat();
          fullUpdateNeeded = true;
	}

        // Finally, request framebuffer update if needed.
        if (fullUpdateNeeded) {
          rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
                                            rfb.framebufferHeight, false);
        } else {
          rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
                                            rfb.framebufferHeight, true);
        }

	break;

      case RfbProto.SetColourMapEntries:
	throw new Exception("Can't handle SetColourMapEntries message");

      case RfbProto.Bell:
        Toolkit.getDefaultToolkit().beep();
	break;

      case RfbProto.ServerCutText:
	String s = rfb.readServerCutText();
	viewer.clipboard.setCutText(s);
	break;

      default:
	throw new Exception("Unknown RFB message type " + msgType);
      }
    }
  }

  //
  // Handle a raw rectangle. The second form with paint==false is used
  // by the Hextile decoder for raw-encoded tiles.
  //

  void handleRawRect(int x, int y, int w, int h) throws IOException, Exception {
    handleRawRect(x, y, w, h, true);
  }

  void handleRawRect(int x, int y, int w, int h, boolean paint)
    throws IOException , Exception{
    rawDecoder.handleRect(x, y, w, h);
    if (paint)
      scheduleRepaint(x, y, w, h);
  }

  //
  // Handle a CopyRect rectangle.
  //

  void handleCopyRect(int x, int y, int w, int h) throws IOException {
    copyRectDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Handle an RRE-encoded rectangle.
  //

  void handleRRERect(int x, int y, int w, int h) throws IOException {
    rreDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Handle a CoRRE-encoded rectangle.
  //

  void handleCoRRERect(int x, int y, int w, int h) throws IOException {
    correDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Handle a Hextile-encoded rectangle.
  //

  void handleHextileRect(int x, int y, int w, int h) throws IOException,
                                                            Exception {
    hextileDecoder.handleRect(x, y, w, h);
  }

  //
  // Handle a ZRLE-encoded rectangle.
  //
  // FIXME: Currently, session recording is not fully supported for ZRLE.
  //

  void handleZRLERect(int x, int y, int w, int h) throws Exception {
    zrleDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Handle a Zlib-encoded rectangle.
  //

  void handleZlibRect(int x, int y, int w, int h) throws Exception {
    zlibDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Handle a Tight-encoded rectangle.
  //

  void handleTightRect(int x, int y, int w, int h) throws Exception {
    tightDecoder.handleRect(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Tell JVM to repaint specified desktop area.
  //

  public void scheduleRepaint(int x, int y, int w, int h) {
    // Request repaint, deferred if necessary.
    if (rfb.framebufferWidth == scaledWidth) {
      repaint(viewer.deferScreenUpdates, x, y, w, h);
    } else {
      int sx = x * scalingFactor / 100;
      int sy = y * scalingFactor / 100;
      int sw = ((x + w) * scalingFactor + 49) / 100 - sx + 1;
      int sh = ((y + h) * scalingFactor + 49) / 100 - sy + 1;
      repaint(viewer.deferScreenUpdates, sx, sy, sw, sh);
    }
  }

  //
  // Handle events.
  //

  public void keyPressed(KeyEvent evt) {
    processLocalKeyEvent(evt);
  }
  public void keyReleased(KeyEvent evt) {
    processLocalKeyEvent(evt);
  }
  public void keyTyped(KeyEvent evt) {
    evt.consume();
  }

  public void mousePressed(MouseEvent evt) {
    processLocalMouseEvent(evt, false);
  }
  public void mouseReleased(MouseEvent evt) {
    processLocalMouseEvent(evt, false);
  }
  public void mouseMoved(MouseEvent evt) {
    processLocalMouseEvent(evt, true);
  }
  public void mouseDragged(MouseEvent evt) {
    processLocalMouseEvent(evt, true);
  }

  private synchronized void trySendPointerEvent() {
    if ((needToSendMouseEvent) && (mouseEvent!=null)) {
      sendMouseEvent(mouseEvent, false);
      needToSendMouseEvent = false;
      lastMouseEventSendTime = System.currentTimeMillis();
    }
  }

  public void run() {
    while (true) {
      // Send mouse movement if we have it
      trySendPointerEvent();
      // Sleep for some time
      try {
        Thread.sleep(1000 / mouseMaxFreq);
      } catch (InterruptedException ex) {
      }
    }
  }

  //
  // Ignored events.
  //

  public void mouseClicked(MouseEvent evt) {}
  public void mouseEntered(MouseEvent evt) {}
  public void mouseExited(MouseEvent evt) {}

  //
  // Actual event processing.
  //

  private void processLocalKeyEvent(KeyEvent evt) {
    if (viewer.rfb != null && rfb.inNormalProtocol) {
      if (!inputEnabled) {
	if ((evt.getKeyChar() == 'r' || evt.getKeyChar() == 'R') &&
	    evt.getID() == KeyEvent.KEY_PRESSED ) {
	  // Request screen update.
	  try {
	    rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
					      rfb.framebufferHeight, false);
	  } catch (IOException e) {
	    e.printStackTrace();
	  }
	}
      } else {
	// Input enabled.
	synchronized(rfb) {
	  try {
	    rfb.writeKeyEvent(evt);
	  } catch (Exception e) {
	    e.printStackTrace();
	  }
	  rfb.notify();
	}
      }
    }
    // Don't ever pass keyboard events to AWT for default processing.
    // Otherwise, pressing Tab would switch focus to ButtonPanel etc.
    evt.consume();
  }

  private void processLocalMouseEvent(MouseEvent evt, boolean moved) {
    if (viewer.rfb != null && rfb.inNormalProtocol) {
      if (inputEnabled) {
        // If mouse not moved, but it's click event then
        // send it to server immideanlty.
        // Else, it's mouse movement - we can send it in
        // our thread later.
        if (!moved) {
          sendMouseEvent(evt, moved);
        } else {
          mouseEvent = evt;
          needToSendMouseEvent = true;
        }
      }
    }
  }

  private void sendMouseEvent(MouseEvent evt, boolean moved) {
    if (moved) {
      softCursorMove(evt.getX(), evt.getY());
    }
    if (rfb.framebufferWidth != scaledWidth) {
      int sx = (evt.getX() * 100 + scalingFactor/2) / scalingFactor;
      int sy = (evt.getY() * 100 + scalingFactor/2) / scalingFactor;
      evt.translatePoint(sx - evt.getX(), sy - evt.getY());
    }
    synchronized(rfb) {
      try {
        rfb.writePointerEvent(evt);
      } catch (Exception e) {
        e.printStackTrace();
      }
      rfb.notify();
      lastMouseEventSendTime = System.currentTimeMillis();
    }
  }

  //
  // Reset update statistics.
  //

  void resetStats() {
    statStartTime = System.currentTimeMillis();
    statNumUpdates = 0;
    statNumTotalRects = 0;
    statNumPixelRects = 0;
    statNumRectsTight = 0;
    statNumRectsTightJPEG = 0;
    statNumRectsZRLE = 0;
    statNumRectsHextile = 0;
    statNumRectsRaw = 0;
    statNumRectsCopy = 0;
    statNumBytesEncoded = 0;
    statNumBytesDecoded = 0;
    if (tightDecoder != null) {
      tightDecoder.setNumJPEGRects(0);
      tightDecoder.setNumTightRects(0);
    }
  }

  //////////////////////////////////////////////////////////////////
  //
  // Handle cursor shape updates (XCursor and RichCursor encodings).
  //

  boolean showSoftCursor = false;

  MemoryImageSource softCursorSource;
  Image softCursor;
  MouseEvent mouseEvent = null;
  boolean needToSendMouseEvent = false;
  int cursorX = 0, cursorY = 0;
  int cursorWidth, cursorHeight;
  int origCursorWidth, origCursorHeight;
  int hotX, hotY;
  int origHotX, origHotY;

  //
  // Handle cursor shape update (XCursor and RichCursor encodings).
  //

  synchronized void
    handleCursorShapeUpdate(int encodingType,
			    int xhot, int yhot, int width, int height)
    throws IOException {

    softCursorFree();

    if (width * height == 0)
      return;

    // Ignore cursor shape data if requested by user.
    if (viewer.options.ignoreCursorUpdates) {
      int bytesPerRow = (width + 7) / 8;
      int bytesMaskData = bytesPerRow * height;

      if (encodingType == rfb.EncodingXCursor) {
	rfb.skipBytes(6 + bytesMaskData * 2);
      } else {
	// rfb.EncodingRichCursor
	rfb.skipBytes(width * height + bytesMaskData);
      }
      return;
    }

    // Decode cursor pixel data.
    softCursorSource = decodeCursorShape(encodingType, width, height);

    // Set original (non-scaled) cursor dimensions.
    origCursorWidth = width;
    origCursorHeight = height;
    origHotX = xhot;
    origHotY = yhot;

    // Create off-screen cursor image.
    createSoftCursor();

    // Show the cursor.
    showSoftCursor = true;
    repaint(viewer.deferCursorUpdates,
	    cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
  }

  //
  // decodeCursorShape(). Decode cursor pixel data and return
  // corresponding MemoryImageSource instance.
  //

  synchronized MemoryImageSource
    decodeCursorShape(int encodingType, int width, int height)
    throws IOException {

    int bytesPerRow = (width + 7) / 8;
    int bytesMaskData = bytesPerRow * height;

    int[] softCursorPixels = new int[width * height];

    if (encodingType == rfb.EncodingXCursor) {

      // Read foreground and background colors of the cursor.
      byte[] rgb = new byte[6];
      rfb.readFully(rgb);
      int[] colors = { (0xFF000000 | (rgb[3] & 0xFF) << 16 |
			(rgb[4] & 0xFF) << 8 | (rgb[5] & 0xFF)),
		       (0xFF000000 | (rgb[0] & 0xFF) << 16 |
			(rgb[1] & 0xFF) << 8 | (rgb[2] & 0xFF)) };

      // Read pixel and mask data.
      byte[] pixBuf = new byte[bytesMaskData];
      rfb.readFully(pixBuf);
      byte[] maskBuf = new byte[bytesMaskData];
      rfb.readFully(maskBuf);

      // Decode pixel data into softCursorPixels[].
      byte pixByte, maskByte;
      int x, y, n, result;
      int i = 0;
      for (y = 0; y < height; y++) {
	for (x = 0; x < width / 8; x++) {
	  pixByte = pixBuf[y * bytesPerRow + x];
	  maskByte = maskBuf[y * bytesPerRow + x];
	  for (n = 7; n >= 0; n--) {
	    if ((maskByte >> n & 1) != 0) {
	      result = colors[pixByte >> n & 1];
	    } else {
	      result = 0;	// Transparent pixel
	    }
	    softCursorPixels[i++] = result;
	  }
	}
	for (n = 7; n >= 8 - width % 8; n--) {
	  if ((maskBuf[y * bytesPerRow + x] >> n & 1) != 0) {
	    result = colors[pixBuf[y * bytesPerRow + x] >> n & 1];
	  } else {
	    result = 0;		// Transparent pixel
	  }
	  softCursorPixels[i++] = result;
	}
      }

    } else {
      // encodingType == rfb.EncodingRichCursor

      // Read pixel and mask data.
      byte[] pixBuf = new byte[width * height * bytesPixel];
      rfb.readFully(pixBuf);
      byte[] maskBuf = new byte[bytesMaskData];
      rfb.readFully(maskBuf);

      // Decode pixel data into softCursorPixels[].
      byte maskByte;
      int x, y, n, result;
      int i = 0;
      for (y = 0; y < height; y++) {
	for (x = 0; x < width / 8; x++) {
	  maskByte = maskBuf[y * bytesPerRow + x];
	  for (n = 7; n >= 0; n--) {
	    if ((maskByte >> n & 1) != 0) {
	      if (bytesPixel == 1) {
		result = cm8.getRGB(pixBuf[i]);
	      } else {
		result = 0xFF000000 |
		  (pixBuf[i * 4 + 2] & 0xFF) << 16 |
		  (pixBuf[i * 4 + 1] & 0xFF) << 8 |
		  (pixBuf[i * 4] & 0xFF);
	      }
	    } else {
	      result = 0;	// Transparent pixel
	    }
	    softCursorPixels[i++] = result;
	  }
	}
	for (n = 7; n >= 8 - width % 8; n--) {
	  if ((maskBuf[y * bytesPerRow + x] >> n & 1) != 0) {
	    if (bytesPixel == 1) {
	      result = cm8.getRGB(pixBuf[i]);
	    } else {
	      result = 0xFF000000 |
		(pixBuf[i * 4 + 2] & 0xFF) << 16 |
		(pixBuf[i * 4 + 1] & 0xFF) << 8 |
		(pixBuf[i * 4] & 0xFF);
	    }
	  } else {
	    result = 0;		// Transparent pixel
	  }
	  softCursorPixels[i++] = result;
	}
      }

    }

    return new MemoryImageSource(width, height, softCursorPixels, 0, width);
  }

  //
  // createSoftCursor(). Assign softCursor new Image (scaled if necessary).
  // Uses softCursorSource as a source for new cursor image.
  //

  synchronized void
    createSoftCursor() {

    if (softCursorSource == null)
      return;

    int scaleCursor = viewer.options.scaleCursor;
    if (scaleCursor == 0 || !inputEnabled)
      scaleCursor = 100;

    // Save original cursor coordinates.
    int x = cursorX - hotX;
    int y = cursorY - hotY;
    int w = cursorWidth;
    int h = cursorHeight;

    cursorWidth = (origCursorWidth * scaleCursor + 50) / 100;
    cursorHeight = (origCursorHeight * scaleCursor + 50) / 100;
    hotX = (origHotX * scaleCursor + 50) / 100;
    hotY = (origHotY * scaleCursor + 50) / 100;
    softCursor = Toolkit.getDefaultToolkit().createImage(softCursorSource);

    if (scaleCursor != 100) {
      softCursor = softCursor.getScaledInstance(cursorWidth, cursorHeight,
						Image.SCALE_SMOOTH);
    }

    if (showSoftCursor) {
      // Compute screen area to update.
      x = Math.min(x, cursorX - hotX);
      y = Math.min(y, cursorY - hotY);
      w = Math.max(w, cursorWidth);
      h = Math.max(h, cursorHeight);

      repaint(viewer.deferCursorUpdates, x, y, w, h);
    }
  }

  //
  // softCursorMove(). Moves soft cursor into a particular location.
  //

  synchronized void softCursorMove(int x, int y) {
    int oldX = cursorX;
    int oldY = cursorY;
    cursorX = x;
    cursorY = y;
    if (showSoftCursor) {
      repaint(viewer.deferCursorUpdates,
	      oldX - hotX, oldY - hotY, cursorWidth, cursorHeight);
      repaint(viewer.deferCursorUpdates,
	      cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
    }
  }

  //
  // softCursorFree(). Remove soft cursor, dispose resources.
  //

  synchronized void softCursorFree() {
    if (showSoftCursor) {
      showSoftCursor = false;
      softCursor = null;
      softCursorSource = null;

      repaint(viewer.deferCursorUpdates,
	      cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
    }
  }
}
