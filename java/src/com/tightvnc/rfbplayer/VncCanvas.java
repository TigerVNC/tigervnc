//
//  Copyright (C) 2001,2002 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001 Constantin Kaplinsky.  All Rights Reserved.
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
  implements KeyListener, MouseListener, MouseMotionListener {

  VncViewer viewer;
  RfbProto rfb;
  ColorModel cm8, cm24;
  Color[] colors;
  int bytesPixel;

  Image memImage;
  Graphics memGraphics;

  Image rawPixelsImage;
  MemoryImageSource pixelsSource;
  byte[] pixels8;
  int[] pixels24;

  // Zlib encoder's data.
  byte[] zlibBuf;
  int zlibBufLen = 0;
  Inflater zlibInflater;

  // Tight encoder's data.
  final static int tightZlibBufferSize = 512;
  Inflater[] tightInflaters;

  // Since JPEG images are loaded asynchronously, we have to remember
  // their position in the framebuffer. Also, this jpegRect object is
  // used for synchronization between the rfbThread and a JVM's thread
  // which decodes and loads JPEG images.
  Rectangle jpegRect;

  // True if we process keyboard and mouse events.
  boolean listenersInstalled;

  //
  // The constructor.
  //

  VncCanvas(VncViewer v) throws IOException {
    viewer = v;
    rfb = viewer.rfb;

    tightInflaters = new Inflater[4];

    cm8 = new DirectColorModel(8, 7, (7 << 3), (3 << 6));
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);

    colors = new Color[256];
    for (int i = 0; i < 256; i++)
      colors[i] = new Color(cm8.getRGB(i));

    setPixelFormat();

    listenersInstalled = false;
    if (!viewer.options.viewOnly)
      enableInput(true);
  }

  //
  // Callback methods to determine geometry of our Component.
  //

  public Dimension getPreferredSize() {
    return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
  }

  public Dimension getMinimumSize() {
    return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
  }

  public Dimension getMaximumSize() {
    return new Dimension(rfb.framebufferWidth, rfb.framebufferHeight);
  }

  //
  // All painting is performed here.
  //

  public void update(Graphics g) {
    paint(g);
  }

  public void paint(Graphics g) {
    synchronized(memImage) {
      g.drawImage(memImage, 0, 0, null);
    }
    if (showSoftCursor) {
      int x0 = cursorX - hotX, y0 = cursorY - hotY;
      Rectangle r = new Rectangle(x0, y0, cursorWidth, cursorHeight);
      if (r.intersects(g.getClipBounds())) {
	g.drawImage(softCursor, x0, y0, null);
      }
    }
  }

  //
  // Override the ImageObserver interface method to handle drawing of
  // JPEG-encoded data.
  //

  public boolean imageUpdate(Image img, int infoflags,
                             int x, int y, int width, int height) {
    if ((infoflags & (ALLBITS | ABORT)) == 0) {
      return true;		// We need more image data.
    } else {
      // If the whole image is available, draw it now.
      if ((infoflags & ALLBITS) != 0) {
	if (jpegRect != null) {
	  synchronized(jpegRect) {
	    memGraphics.drawImage(img, jpegRect.x, jpegRect.y, null);
	    scheduleRepaint(jpegRect.x, jpegRect.y,
			    jpegRect.width, jpegRect.height);
	    jpegRect.notify();
	  }
	}
      }
      return false;		// All image data was processed.
    }
  }

  //
  // Start/stop receiving keyboard and mouse events.
  //

  public synchronized void enableInput(boolean enable) {
    if (enable && !listenersInstalled) {
      listenersInstalled = true;
      addKeyListener(this);
      addMouseListener(this);
      addMouseMotionListener(this);
      viewer.buttonPanel.enableRemoteAccessControls(true);
    } else if (!enable && listenersInstalled) {
      listenersInstalled = false;
      removeKeyListener(this);
      removeMouseListener(this);
      removeMouseMotionListener(this);
      viewer.buttonPanel.enableRemoteAccessControls(false);
    }
  }

  public void setPixelFormat() throws IOException {
    if (viewer.options.eightBitColors) {
      rfb.writeSetPixelFormat(8, 8, false, true, 7, 7, 3, 0, 3, 6);
      bytesPixel = 1;
    } else {
      rfb.writeSetPixelFormat(32, 24, true, true, 255, 255, 255, 16, 8, 0);
      bytesPixel = 4;
    }
    updateFramebufferSize();
  }

  void updateFramebufferSize() {

    // Useful shortcuts.
    int fbWidth = rfb.framebufferWidth;
    int fbHeight = rfb.framebufferHeight;

    // Create new off-screen image either if it does not exist, or if
    // its geometry should be changed. It's not necessary to replace
    // existing image if only pixel format should be changed.
    if (memImage == null) {
      memImage = viewer.createImage(fbWidth, fbHeight);
      memGraphics = memImage.getGraphics();
    } else if (memImage.getWidth(null) != fbWidth ||
	       memImage.getHeight(null) != fbHeight) {
      synchronized(memImage) {
	memImage = viewer.createImage(fbWidth, fbHeight);
	memGraphics = memImage.getGraphics();
      }
    }

    // Images with raw pixels should be re-allocated on every change
    // of geometry or pixel format.
    if (bytesPixel == 1) {
      pixels24 = null;
      pixels8 = new byte[fbWidth * fbHeight];

      pixelsSource =
	new MemoryImageSource(fbWidth, fbHeight, cm8, pixels8, 0, fbWidth);
    } else {
      pixels8 = null;
      pixels24 = new int[fbWidth * fbHeight];

      pixelsSource =
	new MemoryImageSource(fbWidth, fbHeight, cm24, pixels24, 0, fbWidth);
    }
    pixelsSource.setAnimated(true);
    rawPixelsImage = createImage(pixelsSource);

    // Update the size of desktop containers.
    if (viewer.inSeparateFrame) {
      if (viewer.desktopScrollPane != null)
	resizeDesktopFrame();
    } else {
      setSize(fbWidth, fbHeight);
    }
  }

  void resizeDesktopFrame() {
    setSize(rfb.framebufferWidth, rfb.framebufferHeight);

    // FIXME: Find a better way to determine correct size of a
    // ScrollPane.  -- const
    Insets insets = viewer.desktopScrollPane.getInsets();
    viewer.desktopScrollPane.setSize(rfb.framebufferWidth +
				     2 * Math.min(insets.left, insets.right),
				     rfb.framebufferHeight +
				     2 * Math.min(insets.top, insets.bottom));

    viewer.vncFrame.pack();

    // Try to limit the frame size to the screen size.
    Dimension screenSize = viewer.vncFrame.getToolkit().getScreenSize();
    Dimension frameSize = viewer.vncFrame.getSize();
    Dimension newSize = frameSize;
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

  public void processNormalProtocol() throws IOException {

    rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
				      rfb.framebufferHeight, false);

    //
    // main dispatch loop
    //

    while (true) {
      int msgType = rfb.readServerMessageType();

      switch (msgType) {
      case RfbProto.FramebufferUpdate:
	rfb.readFramebufferUpdate();

	for (int i = 0; i < rfb.updateNRects; i++) {
	  rfb.readFramebufferUpdateRectHdr();

	  if (rfb.updateRectEncoding == rfb.EncodingLastRect)
	    break;

	  if (rfb.updateRectEncoding == rfb.EncodingNewFBSize) {
	    rfb.setFramebufferSize(rfb.updateRectW, rfb.updateRectH);
	    updateFramebufferSize();
	    break;
	  }

	  if (rfb.updateRectEncoding == rfb.EncodingXCursor ||
	      rfb.updateRectEncoding == rfb.EncodingRichCursor) {
	    handleCursorShapeUpdate(rfb.updateRectEncoding,
				    rfb.updateRectX, rfb.updateRectY,
				    rfb.updateRectW, rfb.updateRectH);
	    continue;
	  }

	  switch (rfb.updateRectEncoding) {

	  case RfbProto.EncodingRaw:
	  {
	    handleRawRect(rfb.updateRectX, rfb.updateRectY,
			  rfb.updateRectW, rfb.updateRectH);
	    break;
	  }

	  case RfbProto.EncodingCopyRect:
	  {
	    rfb.readCopyRect();

	    int sx = rfb.copyRectSrcX, sy = rfb.copyRectSrcY;
	    int rx = rfb.updateRectX, ry = rfb.updateRectY;
	    int rw = rfb.updateRectW, rh = rfb.updateRectH;

	    memGraphics.copyArea(sx, sy, rw, rh, rx - sx, ry - sy);

	    scheduleRepaint(rx, ry, rw, rh);
	    break;
	  }

	  case RfbProto.EncodingRRE:
	  {
	    int rx = rfb.updateRectX, ry = rfb.updateRectY;
	    int rw = rfb.updateRectW, rh = rfb.updateRectH;
	    int nSubrects = rfb.is.readInt();
	    int x, y, w, h;
	    Color pixel;

	    if (bytesPixel == 1) {
	      memGraphics.setColor(colors[rfb.is.readUnsignedByte()]);
	      memGraphics.fillRect(rx, ry, rw, rh);

	      for (int j = 0; j < nSubrects; j++) {
		pixel = colors[rfb.is.readUnsignedByte()];
		x = rx + rfb.is.readUnsignedShort();
		y = ry + rfb.is.readUnsignedShort();
		w = rfb.is.readUnsignedShort();
		h = rfb.is.readUnsignedShort();

		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);
	      }
	    } else {		// 24-bit color
	      pixel = new Color(0xFF000000 | rfb.is.readInt());
	      memGraphics.setColor(pixel);
	      memGraphics.fillRect(rx, ry, rw, rh);

	      for (int j = 0; j < nSubrects; j++) {
		pixel = new Color(0xFF000000 | rfb.is.readInt());
		x = rx + rfb.is.readUnsignedShort();
		y = ry + rfb.is.readUnsignedShort();
		w = rfb.is.readUnsignedShort();
		h = rfb.is.readUnsignedShort();

		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);
	      }
	    }

	    scheduleRepaint(rx, ry, rw, rh);
	    break;
	  }

	  case RfbProto.EncodingCoRRE:
	  {
	    int rx = rfb.updateRectX, ry = rfb.updateRectY;
	    int rw = rfb.updateRectW, rh = rfb.updateRectH;
	    int nSubrects = rfb.is.readInt();
	    int x, y, w, h;
	    Color pixel;

	    if (bytesPixel == 1) {
	      memGraphics.setColor(colors[rfb.is.readUnsignedByte()]);
	      memGraphics.fillRect(rx, ry, rw, rh);

	      for (int j = 0; j < nSubrects; j++) {
		pixel = colors[rfb.is.readUnsignedByte()];
		x = rx + rfb.is.readUnsignedByte();
		y = ry + rfb.is.readUnsignedByte();
		w = rfb.is.readUnsignedByte();
		h = rfb.is.readUnsignedByte();

		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);
	      }
	    } else {		// 24-bit color
	      pixel = new Color(0xFF000000 | rfb.is.readInt());
	      memGraphics.setColor(pixel);
	      memGraphics.fillRect(rx, ry, rw, rh);

	      for (int j = 0; j < nSubrects; j++) {
		pixel = new Color(0xFF000000 | rfb.is.readInt());
		x = rx + rfb.is.readUnsignedByte();
		y = ry + rfb.is.readUnsignedByte();
		w = rfb.is.readUnsignedByte();
		h = rfb.is.readUnsignedByte();

		memGraphics.setColor(pixel);
		memGraphics.fillRect(x, y, w, h);
	      }
	    }

	    scheduleRepaint(rx, ry, rw, rh);
	    break;
	  }

	  case RfbProto.EncodingHextile:
	  {
	    int rx = rfb.updateRectX, ry = rfb.updateRectY;
	    int rw = rfb.updateRectW, rh = rfb.updateRectH;
	    Color bg = new Color(0), fg = new Color(0);

	    for (int ty = ry; ty < ry + rh; ty += 16) {

	      int th = 16;
	      if (ry + rh - ty < 16)
		th = ry + rh - ty;

	      for (int tx = rx; tx < rx + rw; tx += 16) {

		int tw = 16;
		if (rx + rw - tx < 16)
		  tw = rx + rw - tx;

		int subencoding = rfb.is.readUnsignedByte();

		// Is it a raw-encoded sub-rectangle?
		if ((subencoding & rfb.HextileRaw) != 0) {
		  if (bytesPixel == 1) {
		    for (int j = ty; j < ty + th; j++) {
		      rfb.is.readFully(pixels8, j*rfb.framebufferWidth+tx, tw);
		    }
		  } else {
		    byte[] buf = new byte[tw * 4];
		    int count, offset;
		    for (int j = ty; j < ty + th; j++) {
		      rfb.is.readFully(buf);
		      offset = j * rfb.framebufferWidth + tx;
		      for (count = 0; count < tw; count++) {
			pixels24[offset + count] =
			  (buf[count * 4 + 1] & 0xFF) << 16 |
			  (buf[count * 4 + 2] & 0xFF) << 8 |
			  (buf[count * 4 + 3] & 0xFF);
		      }
		    }
		  }
		  handleUpdatedPixels(tx, ty, tw, th);
		  continue;
		}

		// Read and draw the background if specified.
		if ((subencoding & rfb.HextileBackgroundSpecified) != 0) {
		  if (bytesPixel == 1) {
		    bg = colors[rfb.is.readUnsignedByte()];
		  } else {
		    bg = new Color(0xFF000000 | rfb.is.readInt());
		  }
		}
		memGraphics.setColor(bg);
		memGraphics.fillRect(tx, ty, tw, th);

		// Read the foreground color if specified.
		if ((subencoding & rfb.HextileForegroundSpecified) != 0) {
		  if (bytesPixel == 1) {
		    fg = colors[rfb.is.readUnsignedByte()];
		  } else {
		    fg = new Color(0xFF000000 | rfb.is.readInt());
		  }
		}

		// Done with this tile if there is no sub-rectangles.
		if ((subencoding & rfb.HextileAnySubrects) == 0)
		  continue;

		int nSubrects = rfb.is.readUnsignedByte();

		int b1, b2, sx, sy, sw, sh;
		if (bytesPixel == 1) {

		  // BGR233 (8-bit color) version.
		  if ((subencoding & rfb.HextileSubrectsColoured) != 0) {
		    for (int j = 0; j < nSubrects; j++) {
		      fg = colors[rfb.is.readUnsignedByte()];
		      b1 = rfb.is.readUnsignedByte();
		      b2 = rfb.is.readUnsignedByte();
		      sx = tx + (b1 >> 4);
		      sy = ty + (b1 & 0xf);
		      sw = (b2 >> 4) + 1;
		      sh = (b2 & 0xf) + 1;
		      memGraphics.setColor(fg);
		      memGraphics.fillRect(sx, sy, sw, sh);
		    }
		  } else {
		    memGraphics.setColor(fg);
		    for (int j = 0; j < nSubrects; j++) {
		      b1 = rfb.is.readUnsignedByte();
		      b2 = rfb.is.readUnsignedByte();
		      sx = tx + (b1 >> 4);
		      sy = ty + (b1 & 0xf);
		      sw = (b2 >> 4) + 1;
		      sh = (b2 & 0xf) + 1;
		      memGraphics.fillRect(sx, sy, sw, sh);
		    }
		  }

		} else {

		  // Full-color (24-bit) version.
		  if ((subencoding & rfb.HextileSubrectsColoured) != 0) {
		    for (int j = 0; j < nSubrects; j++) {
		      fg = new Color(0xFF000000 | rfb.is.readInt());
		      b1 = rfb.is.readUnsignedByte();
		      b2 = rfb.is.readUnsignedByte();
		      sx = tx + (b1 >> 4);
		      sy = ty + (b1 & 0xf);
		      sw = (b2 >> 4) + 1;
		      sh = (b2 & 0xf) + 1;
		      memGraphics.setColor(fg);
		      memGraphics.fillRect(sx, sy, sw, sh);
		    }
		  } else {
		    memGraphics.setColor(fg);
		    for (int j = 0; j < nSubrects; j++) {
		      b1 = rfb.is.readUnsignedByte();
		      b2 = rfb.is.readUnsignedByte();
		      sx = tx + (b1 >> 4);
		      sy = ty + (b1 & 0xf);
		      sw = (b2 >> 4) + 1;
		      sh = (b2 & 0xf) + 1;
		      memGraphics.fillRect(sx, sy, sw, sh);
		    }
		  }

		}

	      }
	      // Finished with a row of tiles, now let's show it.
	      scheduleRepaint(rx, ty, rw, th);
	    }
	    break;
	  }

	  case RfbProto.EncodingZlib:
	  {
	    int nBytes = rfb.is.readInt();

            if (zlibBuf == null || zlibBufLen < nBytes) {
              zlibBufLen = nBytes * 2;
              zlibBuf = new byte[zlibBufLen];
            }

            rfb.is.readFully(zlibBuf, 0, nBytes);

            if (zlibInflater == null) {
              zlibInflater = new Inflater();
            }
            zlibInflater.setInput(zlibBuf, 0, nBytes);

            handleZlibRect(rfb.updateRectX, rfb.updateRectY,
			   rfb.updateRectW, rfb.updateRectH);

	    break;
	  }

	  case RfbProto.EncodingTight:
	  {
	    handleTightRect(rfb.updateRectX, rfb.updateRectY,
			    rfb.updateRectW, rfb.updateRectH);

	    break;
	  }

	  default:
	    throw new IOException("Unknown RFB rectangle encoding " +
				  rfb.updateRectEncoding);
	  }

	}

	// Defer framebuffer update request if necessary. But wake up
	// immediately on keyboard or mouse event.
	if (viewer.deferUpdateRequests > 0) {
	  synchronized(rfb) {
	    try {
	      rfb.wait(viewer.deferUpdateRequests);
	    } catch (InterruptedException e) {
	    }
	  }
	}

	// Before requesting framebuffer update, check if the pixel
	// format should be changed. If it should, request full update
	// instead of incremental one.
	if (viewer.options.eightBitColors != (bytesPixel == 1)) {
	  setPixelFormat();
	  rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
					    rfb.framebufferHeight, false);
	} else {
	  rfb.writeFramebufferUpdateRequest(0, 0, rfb.framebufferWidth,
					    rfb.framebufferHeight, true);
	}

	break;

      case RfbProto.SetColourMapEntries:
	throw new IOException("Can't handle SetColourMapEntries message");

      case RfbProto.Bell:
        Toolkit.getDefaultToolkit().beep();
	break;

      case RfbProto.ServerCutText:
	String s = rfb.readServerCutText();
	viewer.clipboard.setCutText(s);
	break;

      default:
	throw new IOException("Unknown RFB message type " + msgType);
      }
    }
  }


  //
  // Handle a raw rectangle.
  //

  void handleRawRect(int x, int y, int w, int h) throws IOException {

    if (bytesPixel == 1) {
      for (int dy = y; dy < y + h; dy++) {
	rfb.is.readFully(pixels8, dy * rfb.framebufferWidth + x, w);
      }
    } else {
      byte[] buf = new byte[w * 4];
      int i, offset;
      for (int dy = y; dy < y + h; dy++) {
	rfb.is.readFully(buf);
	offset = dy * rfb.framebufferWidth + x;
	for (i = 0; i < w; i++) {
	  pixels24[offset + i] =
	    (buf[i * 4 + 1] & 0xFF) << 16 |
	    (buf[i * 4 + 2] & 0xFF) << 8 |
	    (buf[i * 4 + 3] & 0xFF);
	}
      }
    }

    handleUpdatedPixels(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }


  //
  // Handle a Zlib-encoded rectangle.
  //

  void handleZlibRect(int x, int y, int w, int h)
    throws IOException {

    try {
      if (bytesPixel == 1) {
	for (int dy = y; dy < y + h; dy++) {
	  zlibInflater.inflate(pixels8, dy * rfb.framebufferWidth + x, w);
	}
      } else {
	byte[] buf = new byte[w * 4];
	int i, offset;
	for (int dy = y; dy < y + h; dy++) {
	  zlibInflater.inflate(buf);
	  offset = dy * rfb.framebufferWidth + x;
	  for (i = 0; i < w; i++) {
	    pixels24[offset + i] =
	      (buf[i * 4 + 1] & 0xFF) << 16 |
	      (buf[i * 4 + 2] & 0xFF) << 8 |
	      (buf[i * 4 + 3] & 0xFF);
	  }
	}
      }
    }
    catch (DataFormatException dfe) {
      throw new IOException(dfe.toString());
    }

    handleUpdatedPixels(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }


  //
  // Handle a tight rectangle.
  //

  void handleTightRect(int x, int y, int w, int h) throws IOException {

    int comp_ctl = rfb.is.readUnsignedByte();

    // Flush zlib streams if we are told by the server to do so.
    for (int stream_id = 0; stream_id < 4; stream_id++) {
      if ((comp_ctl & 1) != 0 && tightInflaters[stream_id] != null) {
	tightInflaters[stream_id] = null;
      }
      comp_ctl >>= 1;
    }

    // Check correctness of subencoding value.
    if (comp_ctl > rfb.TightMaxSubencoding) {
      throw new IOException("Incorrect tight subencoding: " + comp_ctl);
    }

    // Handle solid-color rectangles.
    if (comp_ctl == rfb.TightFill) {
      if (bytesPixel == 1) {
	memGraphics.setColor(colors[rfb.is.readUnsignedByte()]);
      } else {
	byte[] buf = new byte[3];
	rfb.is.readFully(buf);
	Color bg = new Color(0xFF000000 | (buf[0] & 0xFF) << 16 |
			     (buf[1] & 0xFF) << 8 | (buf[2] & 0xFF));
	memGraphics.setColor(bg);
      }
      memGraphics.fillRect(x, y, w, h);
      scheduleRepaint(x, y, w, h);
      return;
    }

    if (comp_ctl == rfb.TightJpeg) {

      // Read JPEG data.
      byte[] jpegData = new byte[rfb.readCompactLen()];
      rfb.is.readFully(jpegData);

      // Create an Image object from the JPEG data.
      Image jpegImage = Toolkit.getDefaultToolkit().createImage(jpegData);

      // Remember the rectangle where the image should be drawn.
      jpegRect = new Rectangle(x, y, w, h);

      // Let the imageUpdate() method do the actual drawing, here just
      // wait until the image is fully loaded and drawn.
      synchronized(jpegRect) {
	Toolkit.getDefaultToolkit().prepareImage(jpegImage, -1, -1, this);
	try {
	  // Wait no longer than three seconds.
	  jpegRect.wait(3000);
	} catch (InterruptedException e) {
	  throw new IOException("Interrupted while decoding JPEG image");
	}
      }

      // Done, jpegRect is not needed any more.
      jpegRect = null;
      return;

    }

    // Read filter id and parameters.
    int numColors = 0, rowSize = w;
    byte[] palette8 = new byte[2];
    int[] palette24 = new int[256];
    boolean useGradient = false;
    if ((comp_ctl & rfb.TightExplicitFilter) != 0) {
      int filter_id = rfb.is.readUnsignedByte();
      if (filter_id == rfb.TightFilterPalette) {
	numColors = rfb.is.readUnsignedByte() + 1;
        if (bytesPixel == 1) {
	  if (numColors != 2) {
	    throw new IOException("Incorrect tight palette size: " +
				  numColors);
	  }
	  palette8[0] = rfb.is.readByte();
	  palette8[1] = rfb.is.readByte();
	} else {
	  byte[] buf = new byte[numColors * 3];
	  rfb.is.readFully(buf);
	  for (int i = 0; i < numColors; i++) {
	    palette24[i] = ((buf[i * 3] & 0xFF) << 16 |
			    (buf[i * 3 + 1] & 0xFF) << 8 |
			    (buf[i * 3 + 2] & 0xFF));
	  }
	}
	if (numColors == 2)
	  rowSize = (w + 7) / 8;
      } else if (filter_id == rfb.TightFilterGradient) {
	useGradient = true;
      } else if (filter_id != rfb.TightFilterCopy) {
	throw new IOException("Incorrect tight filter id: " + filter_id);
      }
    }
    if (numColors == 0 && bytesPixel == 4)
      rowSize *= 3;

    // Read, optionally uncompress and decode data.
    int dataSize = h * rowSize;
    if (dataSize < rfb.TightMinToCompress) {
      // Data size is small - not compressed with zlib.
      if (numColors != 0) {
	// Indexed colors.
	byte[] indexedData = new byte[dataSize];
	rfb.is.readFully(indexedData);
	if (numColors == 2) {
	  // Two colors.
	  if (bytesPixel == 1) {
	    decodeMonoData(x, y, w, h, indexedData, palette8);
	  } else {
	    decodeMonoData(x, y, w, h, indexedData, palette24);
	  }
	} else {
	  // 3..255 colors (assuming bytesPixel == 4).
	  int i = 0;
	  for (int dy = y; dy < y + h; dy++) {
	    for (int dx = x; dx < x + w; dx++) {
	      pixels24[dy * rfb.framebufferWidth + dx] =
		palette24[indexedData[i++] & 0xFF];
	    }
	  }
	}
      } else if (useGradient) {
	// "Gradient"-processed data
	byte[] buf = new byte[w * h * 3];
	rfb.is.readFully(buf);
	decodeGradientData(x, y, w, h, buf);
      } else {
	// Raw truecolor data.
	if (bytesPixel == 1) {
	  for (int dy = y; dy < y + h; dy++) {
	    rfb.is.readFully(pixels8, dy * rfb.framebufferWidth + x, w);
	  }
	} else {
	  byte[] buf = new byte[w * 3];
	  int i, offset;
	  for (int dy = y; dy < y + h; dy++) {
	    rfb.is.readFully(buf);
	    offset = dy * rfb.framebufferWidth + x;
	    for (i = 0; i < w; i++) {
	      pixels24[offset + i] =
		(buf[i * 3] & 0xFF) << 16 |
		(buf[i * 3 + 1] & 0xFF) << 8 |
		(buf[i * 3 + 2] & 0xFF);
	    }
	  }
	}
      }
    } else {
      // Data was compressed with zlib.
      int zlibDataLen = rfb.readCompactLen();
      byte[] zlibData = new byte[zlibDataLen];
      rfb.is.readFully(zlibData);
      int stream_id = comp_ctl & 0x03;
      if (tightInflaters[stream_id] == null) {
	tightInflaters[stream_id] = new Inflater();
      }
      Inflater myInflater = tightInflaters[stream_id];
      myInflater.setInput(zlibData);
      try {
	if (numColors != 0) {
	  // Indexed colors.
	  byte[] indexedData = new byte[dataSize];
	  myInflater.inflate(indexedData);
	  if (numColors == 2) {
	    // Two colors.
	    if (bytesPixel == 1) {
	      decodeMonoData(x, y, w, h, indexedData, palette8);
	    } else {
	      decodeMonoData(x, y, w, h, indexedData, palette24);
	    }
	  } else {
	    // More than two colors (assuming bytesPixel == 4).
	    int i = 0;
	    for (int dy = y; dy < y + h; dy++) {
	      for (int dx = x; dx < x + w; dx++) {
		pixels24[dy * rfb.framebufferWidth + dx] =
		  palette24[indexedData[i++] & 0xFF];
	      }
	    }
	  }
	} else if (useGradient) {
	  // Compressed "Gradient"-filtered data (assuming bytesPixel == 4).
	  byte[] buf = new byte[w * h * 3];
	  myInflater.inflate(buf);
	  decodeGradientData(x, y, w, h, buf);
	} else {
	  // Compressed truecolor data.
	  if (bytesPixel == 1) {
	    for (int dy = y; dy < y + h; dy++) {
	      myInflater.inflate(pixels8, dy * rfb.framebufferWidth + x, w);
	    }
	  } else {
	    byte[] buf = new byte[w * 3];
	    int i, offset;
	    for (int dy = y; dy < y + h; dy++) {
	      myInflater.inflate(buf);
	      offset = dy * rfb.framebufferWidth + x;
	      for (i = 0; i < w; i++) {
		pixels24[offset + i] =
		  (buf[i * 3] & 0xFF) << 16 |
		  (buf[i * 3 + 1] & 0xFF) << 8 |
		  (buf[i * 3 + 2] & 0xFF);
	      }
	    }
	  }
	}
      }
      catch(DataFormatException dfe) {
	throw new IOException(dfe.toString());
      }
    }

    handleUpdatedPixels(x, y, w, h);
    scheduleRepaint(x, y, w, h);
  }

  //
  // Decode 1bpp-encoded bi-color rectangle (8-bit and 24-bit versions).
  //

  void decodeMonoData(int x, int y, int w, int h, byte[] src, byte[] palette)
    throws IOException {

    int dx, dy, n;
    int i = y * rfb.framebufferWidth + x;
    int rowBytes = (w + 7) / 8;
    byte b;

    for (dy = 0; dy < h; dy++) {
      for (dx = 0; dx < w / 8; dx++) {
	b = src[dy*rowBytes+dx];
	for (n = 7; n >= 0; n--)
	  pixels8[i++] = palette[b >> n & 1];
      }
      for (n = 7; n >= 8 - w % 8; n--) {
	pixels8[i++] = palette[src[dy*rowBytes+dx] >> n & 1];
      }
      i += (rfb.framebufferWidth - w);
    }
  }

  void decodeMonoData(int x, int y, int w, int h, byte[] src, int[] palette)
    throws IOException {

    int dx, dy, n;
    int i = y * rfb.framebufferWidth + x;
    int rowBytes = (w + 7) / 8;
    byte b;

    for (dy = 0; dy < h; dy++) {
      for (dx = 0; dx < w / 8; dx++) {
	b = src[dy*rowBytes+dx];
	for (n = 7; n >= 0; n--)
	  pixels24[i++] = palette[b >> n & 1];
      }
      for (n = 7; n >= 8 - w % 8; n--) {
	pixels24[i++] = palette[src[dy*rowBytes+dx] >> n & 1];
      }
      i += (rfb.framebufferWidth - w);
    }
  }

  //
  // Decode data processed with the "Gradient" filter.
  //

  void decodeGradientData (int x, int y, int w, int h, byte[] buf)
    throws IOException {

    int dx, dy, c;
    byte[] prevRow = new byte[w * 3];
    byte[] thisRow = new byte[w * 3];
    byte[] pix = new byte[3];
    int[] est = new int[3];

    int offset = y * rfb.framebufferWidth + x;

    for (dy = 0; dy < h; dy++) {

      /* First pixel in a row */
      for (c = 0; c < 3; c++) {
	pix[c] = (byte)(prevRow[c] + buf[dy * w * 3 + c]);
	thisRow[c] = pix[c];
      }
      pixels24[offset++] =
	(pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF);

      /* Remaining pixels of a row */
      for (dx = 1; dx < w; dx++) {
	for (c = 0; c < 3; c++) {
	  est[c] = ((prevRow[dx * 3 + c] & 0xFF) + (pix[c] & 0xFF) -
		    (prevRow[(dx-1) * 3 + c] & 0xFF));
	  if (est[c] > 0xFF) {
	    est[c] = 0xFF;
	  } else if (est[c] < 0x00) {
	    est[c] = 0x00;
	  }
	  pix[c] = (byte)(est[c] + buf[(dy * w + dx) * 3 + c]);
	  thisRow[dx * 3 + c] = pix[c];
	}
	pixels24[offset++] =
	  (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF);
      }

      System.arraycopy(thisRow, 0, prevRow, 0, w * 3);
      offset += (rfb.framebufferWidth - w);
    }
  }


  //
  // Display newly updated area of pixels.
  //

  void handleUpdatedPixels(int x, int y, int w, int h) {

    // Draw updated pixels of the off-screen image.
    pixelsSource.newPixels(x, y, w, h);
    memGraphics.setClip(x, y, w, h);
    memGraphics.drawImage(rawPixelsImage, 0, 0, null);
    memGraphics.setClip(0, 0, rfb.framebufferWidth, rfb.framebufferHeight);
  }

  //
  // Tell JVM to repaint specified desktop area.
  //

  void scheduleRepaint(int x, int y, int w, int h) {
    // Request repaint, deferred if necessary.
    repaint(viewer.deferScreenUpdates, x, y, w, h);
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

  public void processLocalKeyEvent(KeyEvent evt) {
    if (rfb != null && rfb.inNormalProtocol) {
      synchronized(rfb) {
	try {
	  rfb.writeKeyEvent(evt);
	} catch (Exception e) {
	  e.printStackTrace();
	}
	rfb.notify();
      }
    }
    // Don't ever pass keyboard events to AWT for default processing. 
    // Otherwise, pressing Tab would switch focus to ButtonPanel etc.
    evt.consume();
  }

  public void processLocalMouseEvent(MouseEvent evt, boolean moved) {
    if (rfb != null && rfb.inNormalProtocol) {
      if (moved) {
	softCursorMove(evt.getX(), evt.getY());
      }
      synchronized(rfb) {
	try {
	  rfb.writePointerEvent(evt);
	} catch (Exception e) {
	  e.printStackTrace();
	}
	rfb.notify();
      }
    }
  }


  //
  // Ignored events.
  //

  public void mouseClicked(MouseEvent evt) {}
  public void mouseEntered(MouseEvent evt) {}
  public void mouseExited(MouseEvent evt) {}


  //////////////////////////////////////////////////////////////////
  //
  // Handle cursor shape updates (XCursor and RichCursor encodings).
  //

  boolean showSoftCursor = false;

  int[] softCursorPixels;
  MemoryImageSource softCursorSource;
  Image softCursor;

  int cursorX = 0, cursorY = 0;
  int cursorWidth, cursorHeight;
  int hotX, hotY;

  //
  // Handle cursor shape update (XCursor and RichCursor encodings).
  //

  synchronized void
    handleCursorShapeUpdate(int encodingType,
			    int xhot, int yhot, int width, int height)
    throws IOException {

    int bytesPerRow = (width + 7) / 8;
    int bytesMaskData = bytesPerRow * height;

    softCursorFree();

    if (width * height == 0)
      return;

    // Ignore cursor shape data if requested by user.

    if (viewer.options.ignoreCursorUpdates) {
      if (encodingType == rfb.EncodingXCursor) {
	rfb.is.skipBytes(6 + bytesMaskData * 2);
      } else {
	// rfb.EncodingRichCursor
	rfb.is.skipBytes(width * height + bytesMaskData);
      }
      return;
    }

    // Decode cursor pixel data.

    softCursorPixels = new int[width * height];

    if (encodingType == rfb.EncodingXCursor) {

      // Read foreground and background colors of the cursor.
      byte[] rgb = new byte[6];
      rfb.is.readFully(rgb);
      int[] colors = { (0xFF000000 | (rgb[3] & 0xFF) << 16 |
			(rgb[4] & 0xFF) << 8 | (rgb[5] & 0xFF)),
		       (0xFF000000 | (rgb[0] & 0xFF) << 16 |
			(rgb[1] & 0xFF) << 8 | (rgb[2] & 0xFF)) };

      // Read pixel and mask data.
      byte[] pixBuf = new byte[bytesMaskData];
      rfb.is.readFully(pixBuf);
      byte[] maskBuf = new byte[bytesMaskData];
      rfb.is.readFully(maskBuf);

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
      rfb.is.readFully(pixBuf);
      byte[] maskBuf = new byte[bytesMaskData];
      rfb.is.readFully(maskBuf);

      // Decode pixel data into softCursorPixels[].
      byte pixByte, maskByte;
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
		  (pixBuf[i * 4 + 1] & 0xFF) << 16 |
		  (pixBuf[i * 4 + 2] & 0xFF) << 8 |
		  (pixBuf[i * 4 + 3] & 0xFF);
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
		(pixBuf[i * 4 + 1] & 0xFF) << 16 |
		(pixBuf[i * 4 + 2] & 0xFF) << 8 |
		(pixBuf[i * 4 + 3] & 0xFF);
	    }
	  } else {
	    result = 0;		// Transparent pixel
	  }
	  softCursorPixels[i++] = result;
	}
      }

    }

    // Draw the cursor on an off-screen image.

    softCursorSource =
      new MemoryImageSource(width, height, softCursorPixels, 0, width);
    softCursor = createImage(softCursorSource);

    // Set remaining data associated with cursor.

    cursorWidth = width;
    cursorHeight = height;
    hotX = xhot;
    hotY = yhot;

    showSoftCursor = true;

    // Show the cursor.

    repaint(viewer.deferCursorUpdates,
	    cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
  }

  //
  // softCursorMove(). Moves soft cursor into a particular location.
  //

  synchronized void softCursorMove(int x, int y) {
    if (showSoftCursor) {
      repaint(viewer.deferCursorUpdates,
	      cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
      repaint(viewer.deferCursorUpdates,
	      x - hotX, y - hotY, cursorWidth, cursorHeight);
    }

    cursorX = x;
    cursorY = y;
  }

  //
  // softCursorFree(). Remove soft cursor, dispose resources.
  //

  synchronized void softCursorFree() {
    if (showSoftCursor) {
      showSoftCursor = false;
      softCursor = null;
      softCursorSource = null;
      softCursorPixels = null;

      repaint(viewer.deferCursorUpdates,
	      cursorX - hotX, cursorY - hotY, cursorWidth, cursorHeight);
    }
  }
}
