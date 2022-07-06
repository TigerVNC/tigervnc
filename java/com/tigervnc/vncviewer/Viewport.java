/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2006 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2009 Paul Donohue.  All Rights Reserved.
 * Copyright (C) 2010, 2012-2013 D. R. Commander.  All Rights Reserved.
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
// DesktopWindow is an AWT Canvas representing a VNC desktop.
//
// Methods on DesktopWindow are called from both the GUI thread and the thread
// which processes incoming RFB messages ("the RFB thread").  This means we
// need to be careful with synchronization here.
//

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.event.*;
import java.awt.geom.AffineTransform;
import java.awt.image.*;
import java.nio.*;
import java.util.*;
import javax.swing.*;

import javax.imageio.*;
import java.io.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Cursor;
import com.tigervnc.rfb.Exception;
import com.tigervnc.rfb.Point;

import static java.awt.event.KeyEvent.*;
import static com.tigervnc.vncviewer.Parameters.*;
import static com.tigervnc.rfb.Keysymdef.*;

class Viewport extends JPanel implements ActionListener {

  static LogWriter vlog = new LogWriter("Viewport");

  enum ID { EXIT, FULLSCREEN, MINIMIZE, RESIZE, NEWVIEWER,
            CTRL, ALT, MENUKEY, CTRLALTDEL, CLIPBOARD,
            REFRESH, OPTIONS, INFO, ABOUT, DISMISS }

  enum MENU { INACTIVE, TOGGLE, VALUE, RADIO,
              INVISIBLE, SUBMENU_POINTER, SUBMENU, DIVIDER }

  public Viewport(int w, int h, PixelFormat serverPF, CConn cc_)
  {
    cc = cc_;
    setScaledSize(w, h);
    frameBuffer = createFramebuffer(serverPF, w, h);
    assert(frameBuffer != null);
    setBackground(Color.BLACK);

    cc.setFramebuffer(frameBuffer);

    contextMenu = new JPopupMenu();

    OptionsDialog.addCallback("handleOptions", this);

    addMouseListener(new MouseAdapter() {
      public void mouseClicked(MouseEvent e) { }
      public void mouseEntered(MouseEvent e) { handle(e); }
      public void mouseExited(MouseEvent e) { handle(e); }
      public void mouseReleased(MouseEvent e) { handle(e); }
      public void mousePressed(MouseEvent e) { handle(e); }
    });
    addMouseWheelListener(new MouseAdapter() {
      public void mouseWheelMoved(MouseWheelEvent e) { handle(e); }
    });
    addMouseMotionListener(new MouseMotionAdapter() {
      public void mouseDragged(MouseEvent e) { handle(e); }
      public void mouseMoved(MouseEvent e) { handle(e); }
    });
    addKeyListener(new KeyAdapter() {
      public void keyTyped(KeyEvent e) { }
      public void keyPressed(KeyEvent e) { handleSystemEvent(e); }
      public void keyReleased(KeyEvent e) { handleSystemEvent(e); }
    });
    addFocusListener(new FocusAdapter() {
      public void focusGained(FocusEvent e) {
        ClipboardDialog.clientCutText();
      }
      public void focusLost(FocusEvent e) {
        releaseDownKeys();
      }
    });
    // Override default key bindings from L&F
    getActionMap().put("null", new AbstractAction() {
      public void actionPerformed(ActionEvent e) { }
    });
    ArrayList<KeyStroke> keys = new ArrayList<KeyStroke>();
    keys.add(KeyStroke.getKeyStroke(KeyEvent.VK_F10, 0, true));
    keys.add(KeyStroke.getKeyStroke(KeyEvent.VK_ALT, 0, true));
    for (int i=0; i<keys.size(); i++)
      getInputMap(JComponent.WHEN_FOCUSED).put(keys.get(i), "null");

    setFocusTraversalKeysEnabled(false);
    setFocusable(true);

    setMenuKey();

    // Send a fake pointer event so that the server will stop rendering
    // a server-side cursor. Ideally we'd like to send the actual pointer
    // position, but we can't really tell when the window manager is done
    // placing us so we don't have a good time for that.
    handlePointerEvent(new Point(w/2, h/2), 0);
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
      if (cc.server.width() != scaledWidth ||
          cc.server.height() != scaledHeight) {
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
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
    0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
    0xffffffff, 0xff000000, 0xff000000, 0xff000000, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  };

  public void setCursor(int width, int height, Point hotspot,
                        byte[] data)
  {
    int i;

    if (cursor != null)
      cursor.flush();

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

    Dimension cs = tk.getBestCursorSize(cw, ch);
    if (cs.width != cw && cs.height != ch) {
      cw = Math.min(cw, cs.width);
      ch = Math.min(ch, cs.height);
      x = (int)Math.min(x, Math.max(cs.width - 1, 0));
      y = (int)Math.min(y, Math.max(cs.height - 1, 0));
      BufferedImage tmp =
        new BufferedImage(cs.width, cs.height, BufferedImage.TYPE_INT_ARGB_PRE);
      Graphics2D g2 = tmp.createGraphics();
      g2.drawImage(cursor, 0, 0, cw, ch, 0, 0, width, height, null);
      g2.dispose();
      cursor = tmp;
    }

    setCursor(cursor, x, y);
  }

  private void setCursor(Image img, int x, int y)
  {
    java.awt.Point hotspot;
    java.awt.Cursor softCursor;
    String name = "rfb cursor";

    hotspot = new java.awt.Point(x, y);
    softCursor = tk.createCustomCursor(img, hotspot, name);
    setCursor(softCursor);
  }

  public void resize(int x, int y, int w, int h) {
    if ((w != frameBuffer.width()) || (h != frameBuffer.height())) {
      vlog.debug("Resizing framebuffer from "+frameBuffer.width()+"x"+
                 frameBuffer.height()+" to "+w+"x"+h);
      frameBuffer = createFramebuffer(frameBuffer.getPF(), w, h);
      assert(frameBuffer != null);
      cc.setFramebuffer(frameBuffer);
    }
    setScaledSize(w, h);
  }

  public int handle(MouseEvent e)
  {
    int buttonMask, wheelMask;
    switch (e.getID()) {
    case MouseEvent.MOUSE_ENTERED:
      if (cursor != null)
        setCursor(cursor, cursorHotspot.x, cursorHotspot.y);
      return 1;
    case MouseEvent.MOUSE_EXITED:
      setCursor(java.awt.Cursor.getDefaultCursor());
      return 1;
    case MouseEvent.MOUSE_PRESSED:
    case MouseEvent.MOUSE_RELEASED:
    case MouseEvent.MOUSE_DRAGGED:
    case MouseEvent.MOUSE_MOVED:
    case MouseEvent.MOUSE_WHEEL:
      buttonMask = 0;
      if ((e.getModifiersEx() & MouseEvent.BUTTON1_DOWN_MASK) != 0)
        buttonMask |= 1;
      if ((e.getModifiersEx() & MouseEvent.BUTTON2_DOWN_MASK) != 0)
        buttonMask |= 2;
      if ((e.getModifiersEx() & MouseEvent.BUTTON3_DOWN_MASK) != 0)
        buttonMask |= 4;

      if (e.getID() == MouseEvent.MOUSE_WHEEL) {
        wheelMask = 0;
        int clicks = ((MouseWheelEvent)e).getWheelRotation();
        if (clicks < 0)
          wheelMask |= e.isShiftDown() ? 32 : 8;
        else
          wheelMask |= e.isShiftDown() ? 64 : 16;
        Point pt = new Point(e.getX(), e.getY());
        for (int i = 0; i < Math.abs(clicks); i++) {
          handlePointerEvent(pt, buttonMask|wheelMask);
          handlePointerEvent(pt, buttonMask);
        }
        return 1;
      }

      handlePointerEvent(new Point(e.getX(), e.getY()), buttonMask);
      return 1;
    }

    return -1;
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
    synchronized(frameBuffer.getImage()) {
      if (cc.server.width() != scaledWidth ||
          cc.server.height() != scaledHeight) {
        g2.setRenderingHint(RenderingHints.KEY_RENDERING,
                            RenderingHints.VALUE_RENDER_QUALITY);
        g2.drawImage(frameBuffer.getImage(), 0, 0,
                     scaledWidth, scaledHeight, null);
      } else {
        g2.drawImage(frameBuffer.getImage(), 0, 0, null);
      }
    }
    g2.dispose();
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
        float widthRatio = (float)width / (float)cc.server.width();
        float heightRatio = (float)height / (float)cc.server.height();
        float ratio = Math.min(widthRatio, heightRatio);
        scaledWidth = (int)Math.floor(cc.server.width() * ratio);
        scaledHeight = (int)Math.floor(cc.server.height() * ratio);
      }
      scaleRatioX = (float)scaledWidth / (float)cc.server.width();
      scaleRatioY = (float)scaledHeight / (float)cc.server.height();
    }
    if (scaledWidth != getWidth() || scaledHeight != getHeight())
      setSize(new Dimension(scaledWidth, scaledHeight));
  }

  private void handlePointerEvent(Point pos, int buttonMask)
  {
    if (!viewOnly.getValue()) {
      if (buttonMask != lastButtonMask || !pos.equals(lastPointerPos)) {
        try {
          if (cc.server.width() != scaledWidth ||
              cc.server.height() != scaledHeight) {
            int sx = (scaleRatioX == 1.00) ?
              pos.x : (int)Math.floor(pos.x / scaleRatioX);
            int sy = (scaleRatioY == 1.00) ?
              pos.y : (int)Math.floor(pos.y / scaleRatioY);
            pos = pos.translate(new Point(sx - pos.x, sy - pos.y));
          }
          cc.writer().writePointerEvent(pos, buttonMask);
        } catch (Exception e) {
          vlog.error("%s", e.getMessage());
          cc.close();
        }
      }
      lastPointerPos = pos;
      lastButtonMask = buttonMask;
    }
  }

  public void handleKeyPress(long keyCode, int keySym)
  {
    // Prevent recursion if the menu wants to send it's own
    // activation key.
    if ((menuKeySym != 0) && keySym == menuKeySym && !menuRecursion) {
      popupContextMenu();
      return;
    }

    if (viewOnly.getValue())
      return;

    if (keyCode == 0) {
      vlog.error("No key code specified on key press");
      return;
    }

    if (VncViewer.os.startsWith("mac os x")) {
      // Alt on OS X behaves more like AltGr on other systems, and to get
      // sane behaviour we should translate things in that manner for the
      // remote VNC server. However that means we lose the ability to use
      // Alt as a shortcut modifier. Do what RealVNC does and hijack the
      // left command key as an Alt replacement.
      switch (keySym) {
      case XK_Meta_L:
        keySym = XK_Alt_L;
        break;
      case XK_Meta_R:
        keySym = XK_Super_L;
        break;
      case XK_Alt_L:
        keySym = XK_Mode_switch;
        break;
      case XK_Alt_R:
        keySym = XK_ISO_Level3_Shift;
        break;
      }
    }

    if (VncViewer.os.startsWith("windows")) {
      // Ugly hack alert!
      //
      // Windows doesn't have a proper AltGr, but handles it using fake
      // Ctrl+Alt. Unfortunately X11 doesn't generally like the combination
      // Ctrl+Alt+AltGr, which we usually end up with when Xvnc tries to
      // get everything in the correct state. Cheat and temporarily release
      // Ctrl and Alt when we send some other symbol.
      if (downKeySym.containsValue(XK_Control_L) &&
          downKeySym.containsValue(XK_Alt_R)) {
        vlog.debug("Faking release of AltGr (Ctrl_L+Alt_R)");
        try {
          cc.writer().writeKeyEvent(XK_Control_L, false);
          cc.writer().writeKeyEvent(XK_Alt_R, false);
        } catch (Exception e) {
          vlog.error("%s", e.getMessage());
          cc.close();
        }
      }
    }

    // Because of the way keyboards work, we cannot expect to have the same
    // symbol on release as when pressed. This breaks the VNC protocol however,
    // so we need to keep track of what keysym a key _code_ generated on press
    // and send the same on release.
    downKeySym.put(keyCode, keySym);

    vlog.debug("Key pressed: 0x%016x => 0x%04x", keyCode, keySym);

    try {
      // Fake keycode?
      if (keyCode > 0xffffffffL)
        cc.writer().writeKeyEvent(keySym, true);
      else
        cc.writer().writeKeyEvent(keySym, true);
    } catch (Exception e) {
      vlog.error("%s", e.getMessage());
      cc.close();
    }

    if (VncViewer.os.startsWith("windows")) {
      // Ugly hack continued...
      if (downKeySym.containsValue(XK_Control_L) &&
          downKeySym.containsValue(XK_Alt_R)) {
        vlog.debug("Restoring AltGr state");
        try {
          cc.writer().writeKeyEvent(XK_Control_L, true);
          cc.writer().writeKeyEvent(XK_Alt_R, true);
        } catch (Exception e) {
          vlog.error("%s", e.getMessage());
          cc.close();
        }
      }
    }
  }

  public void handleKeyRelease(long keyCode)
  {
    Integer iter;

    if (viewOnly.getValue())
      return;

    iter = downKeySym.get(keyCode);
    if (iter == null) {
      // These occur somewhat frequently so let's not spam them unless
      // logging is turned up.
      vlog.debug("Unexpected release of key code %d", keyCode);
      return;
    }

    vlog.debug("Key released: 0x%016x => 0x%04x", keyCode, iter);

    try {
      if (keyCode > 0xffffffffL)
        cc.writer().writeKeyEvent(iter, false);
      else
        cc.writer().writeKeyEvent(iter, false);
    } catch (Exception e) {
      vlog.error("%s", e.getMessage());
      cc.close();
    }

    downKeySym.remove(keyCode);
  }

  private int handleSystemEvent(AWTEvent event)
  {

    if (event instanceof KeyEvent) {
      KeyEvent ev = (KeyEvent)event;
      if (KeyMap.get_keycode_fallback_extended(ev) == 0) {
        // Not much we can do with this...
        vlog.debug("Ignoring KeyEvent with unknown Java keycode");
        return 0;
      }

      if (ev.getID() == KeyEvent.KEY_PRESSED) {
        // Generate a fake keycode just for tracking if we can't figure
        // out the proper one.  Java virtual key codes aren't unique 
        // between left/right versions of keys, so we can't use them as
        // indexes to the downKeySym map.
        long keyCode = KeyMap.get_keycode_fallback_extended(ev) | ((long)ev.getKeyLocation()<<32);

        // Pressing Ctrl wreaks havoc with the symbol lookup, so turn
        // that off. But AltGr shows up as Ctrl_L+Alt_R in Windows, so
        // construct a new KeyEvent that uses a proper AltGraph for the
        // symbol lookup.
        int keySym;
        if (VncViewer.os.startsWith("windows") &&
            downKeySym.containsValue(XK_Control_L) &&
            downKeySym.containsValue(XK_Alt_R)) {
          int mask = ev.getModifiers();
          mask &= ~CTRL_MASK;
          mask &= ~ALT_MASK;
          mask |= ALT_GRAPH_MASK;
          AWTKeyStroke ks =
            AWTKeyStroke.getAWTKeyStroke(KeyMap.get_keycode_fallback_extended(ev), mask);
          // The mask manipulations above break key combinations involving AltGr
          // and a key with an accented letter on some keyboard layouts (i.e. IT).
          // So the code should first try the modified event, but if it returns no
          // symbol, the original event should be used.
          final KeyEvent winev = new KeyEvent((JComponent)ev.getSource(), ev.getID(),
                            ev.getWhen(), mask, KeyMap.get_keycode_fallback_extended(ev),
                            ks.getKeyChar(), ev.getKeyLocation());
          keySym = KeyMap.vkey_to_keysym(winev);
          if (keySym == KeyMap.NoSymbol)
            keySym = KeyMap.vkey_to_keysym(ev);
          else
            ev = winev;
        } else {
          keySym = KeyMap.vkey_to_keysym(ev);
        }

        if (keySym == KeyMap.NoSymbol)
          vlog.error("No symbol for virtual key 0x%016x", keyCode);

        if (VncViewer.os.startsWith("linux")) {
          switch (keySym) {
          // For the first few years, there wasn't a good consensus on what the
          // Windows keys should be mapped to for X11. So we need to help out a
          // bit and map all variants to the same key...
          case XK_Hyper_L:
            keySym = XK_Super_L;
            break;
          case XK_Hyper_R:
            keySym = XK_Super_R;
            break;
          // There has been several variants for Shift-Tab over the years.
          // RFB states that we should always send a normal tab.
          case XK_ISO_Left_Tab:
            keySym = XK_Tab;
            break;
          }
        }

        handleKeyPress(keyCode, keySym);

        if (VncViewer.os.startsWith("mac os x")) {
          // We don't get any release events for CapsLock, so we have to
          // send the release right away.
          if (keySym == XK_Caps_Lock)
            handleKeyRelease(keyCode);
        }

        return 1;
      } else if (ev.getID() == KeyEvent.KEY_RELEASED) {
        long keyCode = KeyMap.get_keycode_fallback_extended(ev) | ((long)ev.getKeyLocation()<<32);
        handleKeyRelease(keyCode);
        return 1;
      }
    }

    return 0;
  }

  private void initContextMenu()
  {
    contextMenu.setLightWeightPopupEnabled(false);

    contextMenu.removeAll();

    menu_add(contextMenu, "Exit viewer", KeyEvent.VK_X,
             this, ID.EXIT, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Full screen", KeyEvent.VK_F, this, ID.FULLSCREEN,
             window().fullscreen_active() ?
             EnumSet.of(MENU.TOGGLE, MENU.VALUE) : EnumSet.of(MENU.TOGGLE));
    menu_add(contextMenu, "Minimize", KeyEvent.VK_Z,
             this, ID.MINIMIZE, EnumSet.noneOf(MENU.class));
    menu_add(contextMenu, "Resize window to session", KeyEvent.VK_W,
             this, ID.RESIZE,
             window().fullscreen_active() ?
             EnumSet.of(MENU.INACTIVE, MENU.DIVIDER) : EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Clipboard viewer...", KeyEvent.VK_UNDEFINED,
             this, ID.CLIPBOARD, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Ctrl", KeyEvent.VK_C,
             this, ID.CTRL,
             menuCtrlKey ? EnumSet.of(MENU.TOGGLE, MENU.VALUE) : EnumSet.of(MENU.TOGGLE));
    menu_add(contextMenu, "Alt", KeyEvent.VK_A,
             this, ID.ALT,
             menuAltKey ? EnumSet.of(MENU.TOGGLE, MENU.VALUE) : EnumSet.of(MENU.TOGGLE));

    if (menuKeySym != 0) {
      String sendMenuKey = String.format("Send %s", menuKey.getValueStr());
      menu_add(contextMenu, sendMenuKey, menuKeyJava,
               this, ID.MENUKEY, EnumSet.noneOf(MENU.class));
    }

    menu_add(contextMenu, "Send Ctrl-Alt-Del", KeyEvent.VK_D,
             this, ID.CTRLALTDEL, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Refresh screen", KeyEvent.VK_R,
             this, ID.REFRESH, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "New connection...", KeyEvent.VK_N,
             this, ID.NEWVIEWER, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Options...", KeyEvent.VK_O,
             this, ID.OPTIONS, EnumSet.noneOf(MENU.class));
    menu_add(contextMenu, "Connection info...", KeyEvent.VK_I,
             this, ID.INFO, EnumSet.noneOf(MENU.class));
    menu_add(contextMenu, "About TigerVNC viewer...", KeyEvent.VK_T,
             this, ID.ABOUT, EnumSet.of(MENU.DIVIDER));

    menu_add(contextMenu, "Dismiss menu", KeyEvent.VK_M,
             this, ID.DISMISS, EnumSet.noneOf(MENU.class));
  }

  static void menu_add(JPopupMenu menu, String text,
                       int shortcut, ActionListener cb,
                       ID data, EnumSet<MENU> flags)
  {
    JMenuItem item;
    if (flags.contains(MENU.TOGGLE)) {
      item = new JCheckBoxMenuItem(text, flags.contains(MENU.VALUE));
    } else {
      if (shortcut != 0)
        item = new JMenuItem(text, shortcut);
      else
        item = new JMenuItem(text);
    }
    item.setActionCommand(data.toString());
    item.addActionListener(cb);
    item.setEnabled(!flags.contains(MENU.INACTIVE));
    menu.add(item);
    if (flags.contains(MENU.DIVIDER))
      menu.addSeparator();
  }

  void popupContextMenu()
  {
    // initialize context menu before display
    initContextMenu();

    contextMenu.setCursor(java.awt.Cursor.getDefaultCursor());

    contextMenu.show(this, lastPointerPos.x, lastPointerPos.y);
  }

  public void actionPerformed(ActionEvent ev)
  {
    switch(ID.valueOf(ev.getActionCommand())) {
    case EXIT:
      cc.close();
      break;
    case FULLSCREEN:
      if (window().fullscreen_active())
        window().fullscreen_off();
      else
        window().fullscreen_on();
      break;
    case MINIMIZE:
      if (window().fullscreen_active())
        window().fullscreen_off();
      window().setExtendedState(JFrame.ICONIFIED);
      break;
    case RESIZE:
      if (window().fullscreen_active())
        break;
      int dx = window().getInsets().left + window().getInsets().right;
      int dy = window().getInsets().top + window().getInsets().bottom;
      window().setSize(getWidth()+dx, getHeight()+dy);
      break;
    case CLIPBOARD:
      ClipboardDialog.showDialog(window());
      break;
    case CTRL:
      if (((JMenuItem)ev.getSource()).isSelected())
        handleKeyPress(0x1d, XK_Control_L);
      else
        handleKeyRelease(0x1d);
      menuCtrlKey = !menuCtrlKey;
      break;
    case ALT:
      if (((JMenuItem)ev.getSource()).isSelected())
        handleKeyPress(0x38, XK_Alt_L);
      else
        handleKeyRelease(0x38);
      menuAltKey = !menuAltKey;
      break;
    case MENUKEY:
      menuRecursion = true;
      handleKeyPress(menuKeyCode, menuKeySym);
      menuRecursion = false;
      handleKeyRelease(menuKeyCode);
      break;
    case CTRLALTDEL:
      handleKeyPress(0x1d, XK_Control_L);
      handleKeyPress(0x38, XK_Alt_L);
      handleKeyPress(0xd3, XK_Delete);

      handleKeyRelease(0xd3);
      handleKeyRelease(0x38);
      handleKeyRelease(0x1d);
      break;
    case REFRESH:
      cc.refreshFramebuffer();
      break;
    case NEWVIEWER:
      VncViewer.newViewer();
      break;
    case OPTIONS:
      OptionsDialog.showDialog(cc.desktop);
      break;
    case INFO:
      Window fullScreenWindow =
        DesktopWindow.getFullScreenWindow();
      if (fullScreenWindow != null)
        DesktopWindow.setFullScreenWindow(null);
      JOptionPane op = new JOptionPane(cc.connectionInfo(),
                                       JOptionPane.PLAIN_MESSAGE,
                                       JOptionPane.DEFAULT_OPTION);
      JDialog dlg = op.createDialog(window(), "VNC connection info");
      dlg.setIconImage(VncViewer.frameIcon);
      dlg.setAlwaysOnTop(true);
      dlg.setVisible(true);
      if (fullScreenWindow != null)
        DesktopWindow.setFullScreenWindow(fullScreenWindow);
      break;
    case ABOUT:
      VncViewer.about_vncviewer(cc.desktop);
      break;
    case DISMISS:
      break;
    }
  }

  private void setMenuKey()
  {
    menuKeyJava = MenuKey.getMenuKeyJavaCode();
    menuKeyCode = MenuKey.getMenuKeyCode();
    menuKeySym = MenuKey.getMenuKeySym();
  }

  public void handleOptions()
  {
    setMenuKey();
    /*
    setScaledSize(cc.server.width(), cc.server.height());
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

  public void releaseDownKeys() {
    while (!downKeySym.isEmpty())
      handleKeyRelease(downKeySym.keySet().iterator().next());
  }

  private DesktopWindow window() {
    return (DesktopWindow)getTopLevelAncestor();
  }
  private int x() { return getX(); }
  private int y() { return getY(); }
  private int w() { return getWidth(); }
  private int h() { return getHeight(); }
  // access to cc by different threads is specified in CConn
  private CConn cc;

  // access to the following must be synchronized:
  private PlatformPixelBuffer frameBuffer;

  Point lastPointerPos = new Point(0, 0);
  int lastButtonMask = 0;

  private class DownMap extends HashMap<Long, Integer> {
    public DownMap(int capacity) {
      super(capacity);
    }
  }
  DownMap downKeySym = new DownMap(256);

  int menuKeySym;
  int menuKeyCode, menuKeyJava;
  JPopupMenu contextMenu;
  boolean menuRecursion = false;

  boolean menuCtrlKey = false;
  boolean menuAltKey = false;

  static Toolkit tk = Toolkit.getDefaultToolkit();

  public int scaledWidth = 0, scaledHeight = 0;
  float scaleRatioX, scaleRatioY;

  static BufferedImage cursor;
  Point cursorHotspot = new Point();

}
