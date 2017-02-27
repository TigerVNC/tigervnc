/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2009 Paul Donohue.  All Rights Reserved.
 * Copyright (C) 2010, 2012-2013 D. R. Commander.  All Rights Reserved.
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
// DesktopWindow is an AWT Canvas representing a VNC desktop.
//
// Methods on DesktopWindow are called from both the GUI thread and the thread
// which processes incoming RFB messages ("the RFB thread").  This means we
// need to be careful with synchronization here.
//

package com.tigervnc.vncviewer;
import java.awt.*;
import java.awt.Color;
import java.awt.color.ColorSpace;
import java.awt.event.*;
import java.awt.geom.AffineTransform;
import java.awt.image.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.Clipboard;
import java.io.BufferedReader;
import java.nio.*;
import javax.swing.*;

import javax.imageio.*;
import java.io.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Cursor;
import com.tigervnc.rfb.Point;

import static com.tigervnc.vncviewer.Parameters.*;

class Viewport extends JPanel implements MouseListener,
  MouseMotionListener, MouseWheelListener, KeyListener {

  static LogWriter vlog = new LogWriter("Viewport");

  public Viewport(int w, int h, PixelFormat serverPF, CConn cc_)
  {
    cc = cc_;
    setScaledSize(cc.cp.width, cc.cp.height);
    frameBuffer = createFramebuffer(serverPF, w, h);
    assert(frameBuffer != null);
    setBackground(Color.BLACK);

    cc.setFramebuffer(frameBuffer);
    OptionsDialog.addCallback("handleOptions", this);

    addMouseListener(this);
    addMouseWheelListener(this);
    addMouseMotionListener(this);
    addKeyListener(this);
    addFocusListener(new FocusAdapter() {
      public void focusGained(FocusEvent e) {
        ClipboardDialog.clientCutText();
      }
      public void focusLost(FocusEvent e) {
        cc.releaseDownKeys();
      }
    });
    setFocusTraversalKeysEnabled(false);
    setFocusable(true);

    // Send a fake pointer event so that the server will stop rendering
    // a server-side cursor. Ideally we'd like to send the actual pointer
    // position, but we can't really tell when the window manager is done
    // placing us so we don't have a good time for that.
    cc.writer().writePointerEvent(new Point(w/2, h/2), 0);
  }

  // Most efficient format (from Viewport's point of view)
  public PixelFormat getPreferredPF()
  {
    return frameBuffer.getPF();
  }

  // Copy the areas of the framebuffer that have been changed (damaged)
  // to the displayed window.
  public void updateWindow() {
    Rect r = frameBuffer.getDamage();
    if (!r.is_empty()) {
      if (image == null)
        image = (BufferedImage)createImage(frameBuffer.width(), frameBuffer.height());
      image.getRaster().setDataElements(r.tl.x, r.tl.y, frameBuffer.getBuffer(r));
      if (cc.cp.width != scaledWidth ||
          cc.cp.height != scaledHeight) {
        AffineTransform t = new AffineTransform(); 
        t.scale((double)scaleRatioX, (double)scaleRatioY);
        Rectangle s = new Rectangle(r.tl.x, r.tl.y, r.width(), r.height());
        s = t.createTransformedShape(s).getBounds();
        paintImmediately(s.x, s.y, s.width, s.height);
      } else {
        paintImmediately(r.tl.x, r.tl.y, r.width(), r.height());
      }
    }
  }

  static final int[] dotcursor_xpm = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0xff000000, 0xff000000, 0xff000000, 0x00000000,
    0x00000000, 0xff000000, 0xff000000, 0xff000000, 0x00000000,
    0x00000000, 0xff000000, 0xff000000, 0xff000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  };

  public void setCursor(int width, int height, Point hotspot,
                        byte[] data)
  {
    int i;

    for (i = 0; i < width*height; i++)
      if (data[i*4 + 3] != 0) break;

    if ((i == width*height) && dotWhenNoCursor.getValue()) {
      vlog.debug("cursor is empty - using dot");
      cursor = new BufferedImage(5, 5, BufferedImage.TYPE_INT_ARGB_PRE);
      cursor.setRGB(0, 0, 5, 5, dotcursor_xpm, 0, 5);
      cursorHotspot.x = cursorHotspot.y = 3;
    } else {
      if ((width == 0) || (height == 0)) {
        cursor = new BufferedImage(tk.getBestCursorSize(0, 0).width,
                                   tk.getBestCursorSize(0, 0).height,
                                   BufferedImage.TYPE_INT_ARGB_PRE);
        cursorHotspot.x = cursorHotspot.y = 0;
      } else {
        IntBuffer buffer = IntBuffer.allocate(width*height);
        buffer.put(ByteBuffer.wrap(data).asIntBuffer());
        cursor =
          new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB_PRE);
        cursor.setRGB(0, 0, width, height, buffer.array(), 0, width);
        cursorHotspot = hotspot;

      }
    }

    int cw = (int)Math.floor((float)cursor.getWidth() * scaleRatioX);
    int ch = (int)Math.floor((float)cursor.getHeight() * scaleRatioY);

    int x = (int)Math.floor((float)cursorHotspot.x * scaleRatioX);
    int y = (int)Math.floor((float)cursorHotspot.y * scaleRatioY);

    java.awt.Cursor softCursor;

    Dimension cs = tk.getBestCursorSize(cw, ch);
    if (cs.width != cw && cs.height != ch) {
      cw = Math.min(cw, cs.width);
      ch = Math.min(ch, cs.height);
      x = (int)Math.min(x, Math.max(cs.width - 1, 0));
      y = (int)Math.min(y, Math.max(cs.height - 1, 0));
      BufferedImage scaledImage = 
        new BufferedImage(cs.width, cs.height, BufferedImage.TYPE_INT_ARGB);
      Graphics2D g2 = scaledImage.createGraphics();
      g2.setRenderingHint(RenderingHints.KEY_RENDERING,
                          RenderingHints.VALUE_RENDER_QUALITY);
      g2.drawImage(cursor,
                   0, 0, cw, ch,
                   0, 0, cursor.getWidth(), cursor.getHeight(), null);
      g2.dispose();
      java.awt.Point hs = new java.awt.Point(x, y);
      softCursor = tk.createCustomCursor(scaledImage, hs, "softCursor");
      scaledImage.flush();
    } else {
      java.awt.Point hs = new java.awt.Point(x, y);
      softCursor = tk.createCustomCursor(cursor, hs, "softCursor");
    }

    cursor.flush();

    setCursor(softCursor);

  }

  public void resize(int x, int y, int w, int h) {
    if ((w != frameBuffer.width()) || (h != frameBuffer.height())) {
      vlog.debug("Resizing framebuffer from "+frameBuffer.width()+"x"+
                 frameBuffer.height()+" to "+w+"x"+h);
      frameBuffer = createFramebuffer(frameBuffer.getPF(), w, h);
      assert(frameBuffer != null);
      cc.setFramebuffer(frameBuffer);
      image = null;
    }
    setScaledSize(w, h);
  }

  private PlatformPixelBuffer createFramebuffer(PixelFormat pf, int w, int h)
  {
    PlatformPixelBuffer fb;

    fb = new JavaPixelBuffer(w, h);

    return fb;
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

  public void paintComponent(Graphics g) {
    Graphics2D g2 = (Graphics2D)g;
    if (cc.cp.width != scaledWidth ||
        cc.cp.height != scaledHeight) {
      g2.setRenderingHint(RenderingHints.KEY_RENDERING,
                          RenderingHints.VALUE_RENDER_QUALITY);
      g2.drawImage(image, 0, 0, scaledWidth, scaledHeight, null);
    } else {
      g2.drawImage(image, 0, 0, null);
    }
    g2.dispose();
  }

  // Mouse-Motion callback function
  private void mouseMotionCB(MouseEvent e) {
    if (!viewOnly.getValue() &&
        e.getX() >= 0 && e.getX() <= scaledWidth &&
        e.getY() >= 0 && e.getY() <= scaledHeight)
      cc.writePointerEvent(translateMouseEvent(e));
  }
  public void mouseDragged(MouseEvent e) { mouseMotionCB(e); }
  public void mouseMoved(MouseEvent e) { mouseMotionCB(e); }

  // Mouse callback function
  private void mouseCB(MouseEvent e) {
    if (!viewOnly.getValue())
      if ((e.getID() == MouseEvent.MOUSE_RELEASED) ||
          (e.getX() >= 0 && e.getX() <= scaledWidth &&
           e.getY() >= 0 && e.getY() <= scaledHeight))
        cc.writePointerEvent(translateMouseEvent(e));
  }
  public void mouseReleased(MouseEvent e) { mouseCB(e); }
  public void mousePressed(MouseEvent e) { mouseCB(e); }
  public void mouseClicked(MouseEvent e) {}
  public void mouseEntered(MouseEvent e) {
    if (embed.getValue())
      requestFocus();
  }
  public void mouseExited(MouseEvent e) {}

  // MouseWheel callback function
  private void mouseWheelCB(MouseWheelEvent e) {
    if (!viewOnly.getValue())
      cc.writeWheelEvent(e);
  }

  public void mouseWheelMoved(MouseWheelEvent e) {
    mouseWheelCB(e);
  }

  private static final Integer keyEventLock = 0; 

  // Handle the key-typed event.
  public void keyTyped(KeyEvent e) { }

  // Handle the key-released event.
  public void keyReleased(KeyEvent e) {
    synchronized(keyEventLock) {
      cc.writeKeyEvent(e);
    }
  }

  // Handle the key-pressed event.
  public void keyPressed(KeyEvent e)
  {
    if (e.getKeyCode() == MenuKey.getMenuKeyCode()) {
      java.awt.Point pt = e.getComponent().getMousePosition();
      if (pt != null) {
        F8Menu menu = new F8Menu(cc);
        menu.show(e.getComponent(), (int)pt.getX(), (int)pt.getY());
      }
      return;
    }
    int ctrlAltShiftMask = Event.SHIFT_MASK | Event.CTRL_MASK | Event.ALT_MASK;
    if ((e.getModifiers() & ctrlAltShiftMask) == ctrlAltShiftMask) {
      switch (e.getKeyCode()) {
        case KeyEvent.VK_A:
          VncViewer.showAbout(this);
          return;
        case KeyEvent.VK_F:
          if (cc.desktop.fullscreen_active())
            cc.desktop.fullscreen_on();
          else
            cc.desktop.fullscreen_off();
          return;
        case KeyEvent.VK_H:
          cc.refresh();
          return;
        case KeyEvent.VK_I:
          cc.showInfo();
          return;
        case KeyEvent.VK_O:
            OptionsDialog.showDialog(this);
          return;
        case KeyEvent.VK_W:
          VncViewer.newViewer();
          return;
        case KeyEvent.VK_LEFT:
        case KeyEvent.VK_RIGHT:
        case KeyEvent.VK_UP:
        case KeyEvent.VK_DOWN:
          return;
      }
    }
    if ((e.getModifiers() & Event.META_MASK) == Event.META_MASK) {
      switch (e.getKeyCode()) {
        case KeyEvent.VK_COMMA:
        case KeyEvent.VK_N:
        case KeyEvent.VK_W:
        case KeyEvent.VK_I:
        case KeyEvent.VK_R:
        case KeyEvent.VK_L:
        case KeyEvent.VK_F:
        case KeyEvent.VK_Z:
        case KeyEvent.VK_T:
          return;
      }
    }
    synchronized(keyEventLock) {
      cc.writeKeyEvent(e);
    }
  }

  public void setScaledSize(int width, int height)
  {
    assert(width != 0 && height != 0);
    String scaleString = scalingFactor.getValue();
    if (remoteResize.getValue()) {
      scaledWidth = width;
      scaledHeight = height;
      scaleRatioX = 1.00f;
      scaleRatioY = 1.00f;
    } else {
      if (scaleString.matches("^[0-9]+$")) {
        int scalingFactor = Integer.parseInt(scaleString);
        scaledWidth =
          (int)Math.floor((float)width * (float)scalingFactor/100.0);
        scaledHeight =
          (int)Math.floor((float)height * (float)scalingFactor/100.0);
      } else if (scaleString.equalsIgnoreCase("Auto")) {
        scaledWidth = width;
        scaledHeight = height;
      } else {
        float widthRatio = (float)width / (float)cc.cp.width;
        float heightRatio = (float)height / (float)cc.cp.height;
        float ratio = Math.min(widthRatio, heightRatio);
        scaledWidth = (int)Math.floor(cc.cp.width * ratio);
        scaledHeight = (int)Math.floor(cc.cp.height * ratio);
      }
      scaleRatioX = (float)scaledWidth / (float)cc.cp.width;
      scaleRatioY = (float)scaledHeight / (float)cc.cp.height;
    }
    if (scaledWidth != getWidth() || scaledHeight != getHeight())
      setSize(new Dimension(scaledWidth, scaledHeight));
  }

  private MouseEvent translateMouseEvent(MouseEvent e)
  {
    if (cc.cp.width != scaledWidth ||
        cc.cp.height != scaledHeight) {
      int sx = (scaleRatioX == 1.00) ?
        e.getX() : (int)Math.floor(e.getX() / scaleRatioX);
      int sy = (scaleRatioY == 1.00) ?
        e.getY() : (int)Math.floor(e.getY() / scaleRatioY);
      e.translatePoint(sx - e.getX(), sy - e.getY());
    }
    return e;
  }

  public void handleOptions()
  {
    /*
    setScaledSize(cc.cp.width, cc.cp.height);
    if (!oldSize.equals(new Dimension(scaledWidth, scaledHeight))) {
    // Re-layout the DesktopWindow when the scaled size changes.
    // Ideally we'd do this with a ComponentListener, but unfortunately
    // sometimes a spurious resize event is triggered on the viewport
    // when the DesktopWindow is manually resized via the drag handles.
    if (cc.desktop != null && cc.desktop.isVisible()) {
      JScrollPane scroll = (JScrollPane)((JViewport)getParent()).getParent();
      scroll.setViewportBorder(BorderFactory.createEmptyBorder(0,0,0,0));
      cc.desktop.pack();
    }
    */
  }

  // access to cc by different threads is specified in CConn
  private CConn cc;
  private BufferedImage image;

  // access to the following must be synchronized:
  public PlatformPixelBuffer frameBuffer;

  static Toolkit tk = Toolkit.getDefaultToolkit();

  public int scaledWidth = 0, scaledHeight = 0;
  float scaleRatioX, scaleRatioY;

  BufferedImage cursor;
  Point cursorHotspot = new Point();

}
