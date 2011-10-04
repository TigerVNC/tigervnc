/* Copyright (C) 2010 D. R. Commander.  All Rights Reserved.
 * Copyright (C) 2009 Paul Donohue.  All Rights Reserved.
 * Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
// DesktopWindow is an AWT Canvas representing a VNC desktop.
//
// Methods on DesktopWindow are called from both the GUI thread and the thread
// which processes incoming RFB messages ("the RFB thread").  This means we
// need to be careful with synchronization here.
//

package com.tigervnc.vncviewer;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.Clipboard;
import javax.swing.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Cursor;
import com.tigervnc.rfb.Exception;
import com.tigervnc.rfb.Point;

class DesktopWindow extends JPanel implements
                                   Runnable,
                                   MouseListener,
                                   MouseMotionListener,
                                   MouseWheelListener,
                                   KeyListener
{

  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the RFB thread

  public DesktopWindow(int width, int height, PixelFormat serverPF, CConn cc_) {
    cc = cc_;
    setSize(width, height);
    im = new PixelBufferImage(width, height, cc, this);

    cursor = new Cursor();
    cursorBacking = new ManagedPixelBuffer();
    addMouseListener(this);
    addMouseWheelListener(this);
    addMouseMotionListener(this);
    addKeyListener(this);
    addFocusListener(new FocusAdapter() {
      public void focusGained(FocusEvent e) {
        checkClipboard();
      }
    });
    setFocusTraversalKeysEnabled(false);
    setFocusable(true);
    setDoubleBuffered(true);
  }
  
  public int width() {
    return getWidth();
  }

  public int height() {
    return getHeight();
  }

  // initGraphics() is needed because for some reason you can't call
  // getGraphics() on a newly-created awt Component.  It is called when the
  // DesktopWindow has actually been made visible so that getGraphics() ought
  // to work.

  public void initGraphics() { 
    cc.viewport.g = cc.viewport.getGraphics(); 
    graphics = getComponentGraphics(cc.viewport.g);
    prepareImage(im.image, scaledWidth, scaledHeight, this);
  }

  final public PixelFormat getPF() { return im.getPF(); }

  synchronized public void setPF(PixelFormat pf) { 
    im.setPF(pf); 
  }

  public void setViewport(ViewportFrame viewport)
  {
    viewport.setChild(this);
  }

  // Methods called from the RFB thread - these need to be synchronized
  // wherever they access data shared with the GUI thread.

  public void setCursor(int w, int h, Point hotspot,
                                     int[] data, byte[] mask) {
    // strictly we should use a mutex around this test since useLocalCursor
    // might be being altered by the GUI thread.  However it's only a single
    // boolean and it doesn't matter if we get the wrong value anyway.

    synchronized(this) {
    if (!cc.viewer.useLocalCursor.getValue()) return;

    hideLocalCursor();

    cursor.hotspot = hotspot;

    Dimension bsc = tk.getBestCursorSize(w, h);

    cursor.setSize(((int)bsc.getWidth() > w ? (int)bsc.getWidth() : w),
                   ((int)bsc.getHeight() > h ? (int)bsc.getHeight() : h));
    cursor.setPF(getPF());

    cursorBacking.setSize(cursor.width(), cursor.height());
    cursorBacking.setPF(getPF());

    cursor.data = new int[cursor.width() * cursor.height()];
    cursor.mask = new byte[cursor.maskLen()];

    // set the masked pixels of the cursor transparent by using an extra bit in
    // the colormap.  We'll OR this into the data based on the values in the mask.
    if (cursor.getPF().bpp == 8) {
      cursor.cm = new DirectColorModel(9, 7, (7 << 3), (3 << 6), (1 << 8));
    }

    int maskBytesPerRow = (w + 7) / 8;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int byte_ = y * maskBytesPerRow + x / 8;
        int bit = 7 - x % 8;
        if ((mask[byte_] & (1 << bit)) > 0) {
          cursor.data[y * cursor.width() + x] = (cursor.getPF().bpp == 8) ?
            data[y * w + x] | (1 << 8) : data[y * w + x];
        }
      }
      System.arraycopy(mask, y * maskBytesPerRow, cursor.mask, 
        y * ((cursor.width() + 7) / 8), maskBytesPerRow);
    }

    MemoryImageSource bitmap = 
      new MemoryImageSource(cursor.width(), cursor.height(), cursor.cm,
                            cursor.data, 0, cursor.width());
    int cw = (int)Math.floor((float)cursor.width() * scaleWidthRatio);
    int ch = (int)Math.floor((float)cursor.height() * scaleHeightRatio);
    int hint = java.awt.Image.SCALE_DEFAULT;
    hotspot = new Point((int)Math.floor((float)hotspot.x * scaleWidthRatio),
                        (int)Math.floor((float)hotspot.y * scaleHeightRatio));
    Image cursorImage = (cw <= 0 || ch <= 0) ? tk.createImage(bitmap) :
      tk.createImage(bitmap).getScaledInstance(cw,ch,hint);
    softCursor = tk.createCustomCursor(cursorImage,
                  new java.awt.Point(hotspot.x,hotspot.y), "Cursor");
    }

    if (softCursor != null) {
      setCursor(softCursor); 
      cursorAvailable = true;
      return;
    }

    if (!cursorAvailable) {
      cursorAvailable = true;
    }

    showLocalCursor();
    return;
  }

  // setColourMapEntries() changes some of the entries in the colourmap.
  // Unfortunately these messages are often sent one at a time, so we delay the
  // settings taking effect unless the whole colourmap has changed.  This is
  // because getting java to recalculate its internal translation table and
  // redraw the screen is expensive.

  synchronized public void setColourMapEntries(int firstColour, int nColours,
                                               int[] rgbs) {
    im.setColourMapEntries(firstColour, nColours, rgbs);
    if (nColours <= 256) {
      im.updateColourMap();
      im.put(0, 0, im.width(), im.height(), graphics);
    } else {
      if (setColourMapEntriesTimerThread == null) {
        setColourMapEntriesTimerThread = new Thread(this);
        setColourMapEntriesTimerThread.start();
      }
    }
  }

// Update the actual window with the changed parts of the framebuffer.

  public void framebufferUpdateEnd()
  {
    drawInvalidRect();
  }

  // resize() is called when the desktop has changed size
  synchronized public void resize() {
    int w = cc.cp.width;
    int h = cc.cp.height;
    hideLocalCursor();
    setSize(w, h);
    im.resize(w, h);
  }

  final void drawInvalidRect() {
    if (!invalidRect) return;
    int x = invalidLeft;
    int w = invalidRight - x;
    int y = invalidTop;
    int h = invalidBottom - y;
    invalidRect = false;

    synchronized (this) {
      im.put(x, y, w, h, graphics);
    }
  }

  final void invalidate(int x, int y, int w, int h) {
    if (invalidRect) {
      if (x < invalidLeft) invalidLeft = x;
      if (x + w > invalidRight) invalidRight = x + w;
      if (y < invalidTop) invalidTop = y;
      if (y + h > invalidBottom) invalidBottom = y + h;
    } else {
      invalidLeft = x;
      invalidRight = x + w;
      invalidTop = y;
      invalidBottom = y + h;
      invalidRect = true;
    }

    if ((invalidRight - invalidLeft) * (invalidBottom - invalidTop) > 100000)
      drawInvalidRect();
  }

  public void beginRect(int x, int y, int w, int h, int encoding) {
    invalidRect = false;
  }

  public void endRect(int x, int y, int w, int h, int encoding) {
    drawInvalidRect();
  }

  synchronized final public void fillRect(int x, int y, int w, int h, int pix)
  {
    if (overlapsCursor(x, y, w, h)) hideLocalCursor();
    im.fillRect(x, y, w, h, pix);
    invalidate(x, y, w, h);
    if (softCursor == null)
      showLocalCursor();
  }

  synchronized final public void imageRect(int x, int y, int w, int h,
                                           int[] pix) {
    if (overlapsCursor(x, y, w, h)) hideLocalCursor();
    im.imageRect(x, y, w, h, pix);
    invalidate(x, y, w, h);
    if (softCursor == null)
      showLocalCursor();
  }

  synchronized final public void copyRect(int x, int y, int w, int h,
                                          int srcX, int srcY) {
    if (overlapsCursor(x, y, w, h) || overlapsCursor(srcX, srcY, w, h))
      hideLocalCursor();
    im.copyRect(x, y, w, h, srcX, srcY);
    if (!cc.viewer.fastCopyRect.getValue()) {
      invalidate(x, y, w, h);
    }
  }


  // mutex MUST be held when overlapsCursor() is called
  final boolean overlapsCursor(int x, int y, int w, int h) {
    return (x < cursorBackingX + cursorBacking.width() &&
            y < cursorBackingY + cursorBacking.height() &&
            x+w > cursorBackingX && y+h > cursorBackingY);
  }


  ////////////////////////////////////////////////////////////////////
  // The following methods are all called from the GUI thread

  synchronized void resetLocalCursor() {
    hideLocalCursor();
    cursorAvailable = false;
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

  public void setScaledSize() {
    if (!cc.options.autoScale && !cc.options.fixedRatioScale) {
      scaledWidth = (int)Math.floor((float)cc.cp.width * (float)cc.scaleFactor/100.0);
      scaledHeight = (int)Math.floor((float)cc.cp.height * (float)cc.scaleFactor/100.0);
    } else {
      if (cc.viewport == null) {
        scaledWidth = cc.cp.width;
        scaledHeight = cc.cp.height;
      } else {
        Dimension availableSize = cc.viewport.sp.getSize();
        if (availableSize.width == 0 || availableSize.height == 0)
          availableSize = new Dimension(cc.cp.width, cc.cp.height);
        if (cc.options.fixedRatioScale) {
          float widthRatio = (float)availableSize.width / (float)cc.cp.width;
          float heightRatio = (float)availableSize.height / (float)cc.cp.height;
          float ratio = Math.min(widthRatio, heightRatio);
          scaledWidth = (int)Math.floor(cc.cp.width * ratio);
          scaledHeight = (int)Math.floor(cc.cp.height * ratio);
        } else {
          scaledWidth = availableSize.width;
          scaledHeight = availableSize.height;
        }
      }
    }
    scaleWidthRatio = (float)scaledWidth / (float)cc.cp.width;
    scaleHeightRatio = (float)scaledHeight / (float)cc.cp.height;
  }

  synchronized public void paintComponent(Graphics g) {
    Graphics2D g2 = (Graphics2D) g;
    g2.setRenderingHint(RenderingHints.KEY_INTERPOLATION, 
                        RenderingHints.VALUE_INTERPOLATION_BILINEAR);  
    g2.setRenderingHint(RenderingHints.KEY_RENDERING, 
                        RenderingHints.VALUE_RENDER_QUALITY);
    if (cc.cp.width == scaledWidth && cc.cp.height == scaledHeight) {
      g2.drawImage(im.image, 0, 0, null);
    } else {
      g2.drawImage(im.image, 0, 0, scaledWidth, scaledHeight, null);  
    }
  }

  public void repaint() {
    if (graphics != null)
      super.update(graphics);
  }

  String oldContents = "";
  
  synchronized public void checkClipboard() {
    Clipboard cb = null;
    if (!cc.viewer.applet)
      cb = Toolkit.getDefaultToolkit().getSystemClipboard();
    if (cb != null && cc.viewer.sendClipboard.getValue()) {
      Transferable t = cb.getContents(null);
      if ((t != null) && t.isDataFlavorSupported(DataFlavor.stringFlavor)) {
        try {
          String newContents = (String)t.getTransferData(DataFlavor.stringFlavor);
          if (newContents != null && !newContents.equals(oldContents)) {
            cc.writeClientCutText(newContents, newContents.length());
            oldContents = newContents;
            cc.clipboardDialog.setContents(newContents);
          }
        } catch (java.lang.Exception e) {
          System.out.println("Exception getting clipboard data: " + e.getMessage());
        }
      }
    }
  }

  /** Mouse-Motion callback function */
  private void mouseMotionCB(MouseEvent e) {
    if (!cc.viewer.viewOnly.getValue())
      cc.writePointerEvent(e);
    // - If local cursor rendering is enabled then use it
    synchronized(this) {
      if (cursorAvailable) {
        // - Render the cursor!
        if (e.getX() != cursorPosX || e.getY() != cursorPosY) {
          hideLocalCursor();
          if (e.getX() >= 0 && e.getX() < im.width() &&
              e.getY() >= 0 && e.getY() < im.height()) {
            cursorPosX = e.getX();
            cursorPosY = e.getY();
            if (softCursor == null)
              showLocalCursor();
          }
        }
      }
    }
    lastX = e.getX();
    lastY = e.getY();      
  }
  public void mouseDragged(MouseEvent e) { mouseMotionCB(e);}
  public void mouseMoved(MouseEvent e) { mouseMotionCB(e);}

  /** Mouse callback function */
  private void mouseCB(MouseEvent e) {
    if (!cc.viewer.viewOnly.getValue())
      cc.writePointerEvent(e);
    lastX = e.getX();
    lastY = e.getY();
  }
  public void mouseReleased(MouseEvent e){ mouseCB(e);}
  public void mousePressed(MouseEvent e) { mouseCB(e);}
  public void mouseClicked(MouseEvent e){}
  public void mouseEntered(MouseEvent e){}
  public void mouseExited(MouseEvent e){}  
  
  /** MouseWheel callback function */
  private void mouseWheelCB(MouseWheelEvent e) {
    if (!cc.viewer.viewOnly.getValue())
      cc.writeWheelEvent(e);
  }
  public void mouseWheelMoved(MouseWheelEvent e){ 
    mouseWheelCB(e);
  }

  /** Handle the key-typed event. */
  public void keyTyped(KeyEvent e) {}
  /** Handle the key-released event. */
  public void keyReleased(KeyEvent e) {}
  /** Handle the key-pressed event. */
  public void keyPressed(KeyEvent e) {
    if (e.getKeyCode() == 
        (KeyEvent.VK_F1+cc.menuKey-Keysyms.F1)) {
      cc.showMenu(lastX, lastY);
      return;
    }
    if (!cc.viewer.viewOnly.getValue())
      cc.writeKeyEvent(e);
  }

  ////////////////////////////////////////////////////////////////////
  // The following methods are called from both RFB and GUI threads

  // Note that mutex MUST be held when hideLocalCursor() and showLocalCursor()
  // are called.

  private void hideLocalCursor() {
    // - Blit the cursor backing store over the cursor
    if (cursorVisible) {
      cursorVisible = false;
      im.imageRect(cursorBackingX, cursorBackingY, cursorBacking.width(),
                   cursorBacking.height(), cursorBacking.data);
      im.put(cursorBackingX, cursorBackingY, cursorBacking.width(),
             cursorBacking.height(), graphics);
    }
  }

  private void showLocalCursor() {
    if (cursorAvailable && !cursorVisible) {
      if (!im.getPF().equal(cursor.getPF()) ||
          cursor.width() == 0 || cursor.height() == 0) {
        vlog.debug("attempting to render invalid local cursor");
        cursorAvailable = false;
        return;
      }
      cursorVisible = true;
      if (softCursor != null) return;

      int cursorLeft = cursor.hotspot.x;
      int cursorTop = cursor.hotspot.y;
      int cursorRight = cursorLeft + cursor.width();
      int cursorBottom = cursorTop + cursor.height();

      int x = (cursorLeft >= 0 ? cursorLeft : 0);
      int y = (cursorTop >= 0 ? cursorTop : 0);
      int w = ((cursorRight < im.width() ? cursorRight : im.width()) - x);
      int h = ((cursorBottom < im.height() ? cursorBottom : im.height()) - y);

      cursorBackingX = x;
      cursorBackingY = y;
      cursorBacking.setSize(w, h);
      
      for (int j = 0; j < h; j++)
        System.arraycopy(im.data, (y+j) * im.width() + x,
                         cursorBacking.data, j*w, w);

      im.maskRect(cursorLeft, cursorTop, cursor.width(), cursor.height(),
                  cursor.data, cursor.mask);
      im.put(x, y, w, h, graphics);
    }
  }


  // run() is executed by the setColourMapEntriesTimerThread - it sleeps for
  // 100ms before actually updating the colourmap.
  public void run() {
    try {
      Thread.sleep(100);
    } catch (InterruptedException e) {}
    synchronized (this) {
      im.updateColourMap();
      im.put(0, 0, im.width(), im.height(), graphics);
      setColourMapEntriesTimerThread = null;
    }
  }

  // access to cc by different threads is specified in CConn
  CConn cc;

  // access to the following must be synchronized:
  PixelBufferImage im;
  Graphics graphics;
  Thread setColourMapEntriesTimerThread;

  Cursor cursor;
  boolean cursorVisible;     // Is cursor currently rendered?
  boolean cursorAvailable;   // Is cursor available for rendering?
  int cursorPosX, cursorPosY;
  ManagedPixelBuffer cursorBacking;
  int cursorBackingX, cursorBackingY;
  java.awt.Cursor softCursor;
  static Toolkit tk = Toolkit.getDefaultToolkit();

  public int scaledWidth = 0, scaledHeight = 0;
  float scaleWidthRatio, scaleHeightRatio;

  // the following are only ever accessed by the RFB thread:
  boolean invalidRect;
  int invalidLeft, invalidRight, invalidTop, invalidBottom;

  // the following are only ever accessed by the GUI thread:
  int lastX, lastY;

  static LogWriter vlog = new LogWriter("DesktopWindow");
}
