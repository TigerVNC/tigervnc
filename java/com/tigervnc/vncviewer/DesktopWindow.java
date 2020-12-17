/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2019 Brian P. Hinz
 * Copyright (C) 2012-2013 D. R. Commander.  All Rights Reserved.
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

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.event.*;
import java.lang.reflect.*;
import java.util.*;
import javax.swing.*;
import javax.swing.Timer;
import javax.swing.border.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Point;
import java.lang.Exception;

import static javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER;
import static javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_NEVER;
import static javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED;
import static javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED;
import static javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS;
import static javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS;

import static com.tigervnc.vncviewer.Parameters.*;

public class DesktopWindow extends JFrame
{

  static LogWriter vlog = new LogWriter("DesktopWindow");

  public DesktopWindow(int w, int h, String name,
                       PixelFormat serverPF, CConn cc_)
  {
    cc = cc_;
    firstUpdate = true;
    delayedFullscreen = false; delayedDesktopSize = false;

    setFocusable(false);
    setFocusTraversalKeysEnabled(false);
    getToolkit().setDynamicLayout(false);
    if (!VncViewer.os.startsWith("mac os x"))
      setIconImage(VncViewer.frameIcon);
    UIManager.getDefaults().put("ScrollPane.ancestorInputMap",
      new UIDefaults.LazyInputMap(new Object[]{}));
    scroll = new JScrollPane(new Viewport(w, h, serverPF, cc));
    viewport = (Viewport)scroll.getViewport().getView();
    scroll.setBorder(BorderFactory.createEmptyBorder(0,0,0,0));
    getContentPane().add(scroll);

    setName(name);

    lastScaleFactor = scalingFactor.getValue();
    if (VncViewer.os.startsWith("mac os x"))
      if (!noLionFS.getValue())
        enableLionFS();

    OptionsDialog.addCallback("handleOptions", this);

    addWindowFocusListener(new WindowAdapter() {
      public void windowGainedFocus(WindowEvent e) {
        if (isVisible())
          if (scroll.getViewport() != null)
            scroll.getViewport().getView().requestFocusInWindow();
      }
      public void windowLostFocus(WindowEvent e) {
        viewport.releaseDownKeys();
      }
    });

    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        cc.close();
      }
      public void windowDeiconified(WindowEvent e) {
        // ViewportBorder sometimes lost when window is shaded or de-iconified
        repositionViewport();
      }
    });

    addWindowStateListener(new WindowAdapter() {
      public void windowStateChanged(WindowEvent e) {
        int state = e.getNewState();
        if ((state & JFrame.MAXIMIZED_BOTH) != JFrame.MAXIMIZED_BOTH) {
          Rectangle b = getGraphicsConfiguration().getBounds();
          if (!b.contains(getLocationOnScreen()))
            setLocation((int)b.getX(), (int)b.getY());
        }
        // ViewportBorder sometimes lost when restoring on Windows
        repositionViewport();
      }
    });

    // Window resize events
    timer = new Timer(500, new AbstractAction() {
      public void actionPerformed(ActionEvent e) {
        handleResizeTimeout();
      }
    });
    timer.setRepeats(false);
    addComponentListener(new ComponentAdapter() {
      public void componentResized(ComponentEvent e) {
        if (remoteResize.getValue()) {
          if (timer.isRunning())
            timer.restart();
          else
            // Try to get the remote size to match our window size, provided
            // the following conditions are true:
            //
            // a) The user has this feature turned on
            // b) The server supports it
            // c) We're not still waiting for a chance to handle DesktopSize
            // d) We're not still waiting for startup fullscreen to kick in
            if (!firstUpdate && !delayedFullscreen &&
                remoteResize.getValue() && cc.server.supportsSetDesktopSize)
              timer.start();
        } else {
          String scaleString = scalingFactor.getValue();
          if (!scaleString.matches("^[0-9]+$")) {
            Dimension maxSize = getContentPane().getSize();
            if ((maxSize.width != viewport.scaledWidth) ||
                (maxSize.height != viewport.scaledHeight))
              viewport.setScaledSize(maxSize.width, maxSize.height);
            if (!scaleString.equals("Auto")) {
              if (!isMaximized() && !fullscreen_active()) {
                int dx = getInsets().left + getInsets().right;
                int dy = getInsets().top + getInsets().bottom;
                setSize(viewport.scaledWidth+dx, viewport.scaledHeight+dy);
              }
            }
          }
          repositionViewport();
        }
      }
    });

  }

  // Remove resize listener in order to prevent recursion when resizing
  @Override
  public void setSize(Dimension d)
  {
    ComponentListener[] listeners = getListeners(ComponentListener.class);
    for (ComponentListener l : listeners)
      removeComponentListener(l);
    super.setSize(d);
    for (ComponentListener l : listeners)
      addComponentListener(l);
  }

  @Override
  public void setSize(int width, int height)
  {
    ComponentListener[] listeners = getListeners(ComponentListener.class);
    for (ComponentListener l : listeners)
      removeComponentListener(l);
    super.setSize(width, height);
    for (ComponentListener l : listeners)
      addComponentListener(l);
  }

  @Override
  public void setBounds(Rectangle r)
  {
    ComponentListener[] listeners = getListeners(ComponentListener.class);
    for (ComponentListener l : listeners)
      removeComponentListener(l);
    super.setBounds(r);
    for (ComponentListener l : listeners)
      addComponentListener(l);
  }

  private void repositionViewport()
  {
    scroll.revalidate();
    Rectangle r = scroll.getViewportBorderBounds();
    int dx = r.width - viewport.scaledWidth;
    int dy = r.height - viewport.scaledHeight;
    int top = (int)Math.max(Math.floor(dy/2), 0);
    int left = (int)Math.max(Math.floor(dx/2), 0);
    int bottom = (int)Math.max(dy - top, 0);
    int right = (int)Math.max(dx - left, 0);
    Insets insets = new Insets(top, left, bottom, right);
    scroll.setViewportBorder(new MatteBorder(insets, Color.BLACK));
    scroll.revalidate();
  }

  public PixelFormat getPreferredPF()
  {
    return viewport.getPreferredPF();
  }

  public void setName(String name)
  {
    setTitle(name);
  }

  // Copy the areas of the framebuffer that have been changed (damaged)
  // to the displayed window.

  public void updateWindow()
  {
    if (firstUpdate) {
      pack();
      if (fullScreen.getValue())
        fullscreen_on();
      else
        setVisible(true);

      if (maximize.getValue())
        setExtendedState(JFrame.MAXIMIZED_BOTH);

      if (cc.server.supportsSetDesktopSize && !desktopSize.getValue().equals("")) {
        // Hack: Wait until we're in the proper mode and position until
        // resizing things, otherwise we might send the wrong thing.
        if (delayedFullscreen)
          delayedDesktopSize = true;
        else
          handleDesktopSize();
      }
      firstUpdate = false;
    }

    viewport.updateWindow();
  }

  public void resizeFramebuffer(int new_w, int new_h)
  {
    if ((new_w == viewport.scaledWidth) && (new_h == viewport.scaledHeight))
      return;

    // If we're letting the viewport match the window perfectly, then
    // keep things that way for the new size, otherwise just keep things
    // like they are.
    int dx = getInsets().left + getInsets().right;
    int dy = getInsets().top + getInsets().bottom;
    if (!fullscreen_active()) {
      if ((w() == viewport.scaledWidth) && (h() == viewport.scaledHeight))
        setSize(new_w+dx, new_h+dy);
      else {
        // Make sure the window isn't too big. We do this manually because
        // we have to disable the window size restriction (and it isn't
        // entirely trustworthy to begin with).
        if ((w() > new_w) || (h() > new_h))
          setSize(Math.min(w(), new_w)+dx, Math.min(h(), new_h)+dy);
      }
    }

    viewport.resize(0, 0, new_w, new_h);

    // We might not resize the main window, so we need to manually call this
    // to make sure the viewport is centered.
    repositionViewport();

    // repositionViewport() makes sure the scroll widget notices any changes
    // in position, but it might be just the size that changes so we also
    // need a poke here as well.
    validate();
  }

  public void setCursor(int width, int height, Point hotspot,
                        byte[] data)
  {
    viewport.setCursor(width, height, hotspot, data);
  }

  public void fullscreen_on()
  {
    fullScreen.setParam(true);
    lastState = getExtendedState();
    lastBounds = getBounds();
    dispose();
    // Screen bounds calculation affected by maximized window?
    setExtendedState(JFrame.NORMAL);
    setUndecorated(true);
    setVisible(true);
    setBounds(getScreenBounds());
  }

  public void fullscreen_off()
  {
    fullScreen.setParam(false);
    dispose();
    setUndecorated(false);
    setExtendedState(lastState);
    setBounds(lastBounds);
    setVisible(true);
  }

  public boolean fullscreen_active()
  {
    return isUndecorated();
  }

  private void handleDesktopSize()
  {
    if (!desktopSize.getValue().equals("")) {
      int width, height;

      // An explicit size has been requested

      if (desktopSize.getValue().split("x").length != 2)
        return;

      width = Integer.parseInt(desktopSize.getValue().split("x")[0]);
      height = Integer.parseInt(desktopSize.getValue().split("x")[1]);
      remoteResize(width, height);
    } else if (remoteResize.getValue()) {
      // No explicit size, but remote resizing is on so make sure it
      // matches whatever size the window ended up being
      remoteResize(w(), h());
    }
  }

  public void handleResizeTimeout()
  {
    DesktopWindow self = (DesktopWindow)this;

    assert(self != null);

    self.remoteResize(self.w(), self.h());
  }

  private void remoteResize(int width, int height)
  {
    ScreenSet layout;
    ListIterator<Screen> iter;

    if (!fullscreen_active() || (width > w()) || (height > h())) {
      // In windowed mode (or the framebuffer is so large that we need
      // to scroll) we just report a single virtual screen that covers
      // the entire framebuffer.

      layout = cc.server.screenLayout();

      // Not sure why we have no screens, but adding a new one should be
      // safe as there is nothing to conflict with...
      if (layout.num_screens() == 0)
        layout.add_screen(new Screen());
      else if (layout.num_screens() != 1) {
        // More than one screen. Remove all but the first (which we
        // assume is the "primary").

        while (true) {
          iter = layout.begin();
          Screen screen = iter.next();

          if (iter == layout.end())
            break;

          layout.remove_screen(screen.id);
        }
      }

      // Resize the remaining single screen to the complete framebuffer
      ((Screen)layout.begin().next()).dimensions.tl.x = 0;
      ((Screen)layout.begin().next()).dimensions.tl.y = 0;
      ((Screen)layout.begin().next()).dimensions.br.x = width;
      ((Screen)layout.begin().next()).dimensions.br.y = height;
    } else {
      layout = new ScreenSet();
      int id;
      int sx, sy, sw, sh;
      Rect viewport_rect = new Rect();
      Rect screen_rect = new Rect();

      // In full screen we report all screens that are fully covered.

      viewport_rect.setXYWH(x() + (w() - width)/2, y() + (h() - height)/2,
                            width, height);

      // If we can find a matching screen in the existing set, we use
      // that, otherwise we create a brand new screen.
      //
      // FIXME: We should really track screens better so we can handle
      //        a resized one.
      //
      GraphicsEnvironment ge =
        GraphicsEnvironment.getLocalGraphicsEnvironment();
      for (GraphicsDevice gd : ge.getScreenDevices()) {
        for (GraphicsConfiguration gc : gd.getConfigurations()) {
          Rectangle bounds = gc.getBounds();
          sx = bounds.x;
          sy = bounds.y;
          sw = bounds.width;
          sh = bounds.height;

          // Check that the screen is fully inside the framebuffer
          screen_rect.setXYWH(sx, sy, sw, sh);
          if (!screen_rect.enclosed_by(viewport_rect))
            continue;

          // Adjust the coordinates so they are relative to our viewport
          sx -= viewport_rect.tl.x;
          sy -= viewport_rect.tl.y;

          // Look for perfectly matching existing screen...
          for (iter = cc.server.screenLayout().begin();
              iter != cc.server.screenLayout().end(); iter.next()) {
            Screen screen = iter.next(); iter.previous();
            if ((screen.dimensions.tl.x == sx) &&
                (screen.dimensions.tl.y == sy) &&
                (screen.dimensions.width() == sw) &&
                (screen.dimensions.height() == sh))
              break;
          }

          // Found it?
          if (iter != cc.server.screenLayout().end()) {
            layout.add_screen(iter.next());
            continue;
          }

          // Need to add a new one, which means we need to find an unused id
          Random rng = new Random();
          while (true) {
            id = rng.nextInt();
            for (iter = cc.server.screenLayout().begin();
                iter != cc.server.screenLayout().end(); iter.next()) {
              Screen screen = iter.next(); iter.previous();
              if (screen.id == id)
                break;
            }

            if (iter == cc.server.screenLayout().end())
              break;
          }

          layout.add_screen(new Screen(id, sx, sy, sw, sh, 0));
        }

        // If the viewport doesn't match a physical screen, then we might
        // end up with no screens in the layout. Add a fake one...
        if (layout.num_screens() == 0)
          layout.add_screen(new Screen(0, 0, 0, width, height, 0));
      }
    }

    // Do we actually change anything?
    if ((width == cc.server.width()) &&
        (height == cc.server.height()) &&
        (layout == cc.server.screenLayout()))
      return;

    String buffer;
    vlog.debug(String.format("Requesting framebuffer resize from %dx%d to %dx%d",
               cc.server.width(), cc.server.height(), width, height));
    layout.debug_print();

    if (!layout.validate(width, height)) {
      vlog.error("Invalid screen layout computed for resize request!");
      return;
    }

    cc.writer().writeSetDesktopSize(width, height, layout);
  }

  boolean lionFSSupported() { return canDoLionFS; }

  private int x() { return getContentPane().getX(); }
  private int y() { return getContentPane().getY(); }
  private int w() { return getContentPane().getWidth(); }
  private int h() { return getContentPane().getHeight(); }

  void enableLionFS() {
    try {
      String version = System.getProperty("os.version");
      int firstDot = version.indexOf('.');
      int lastDot = version.lastIndexOf('.');
      if (lastDot > firstDot && lastDot >= 0) {
        version = version.substring(0, version.indexOf('.', firstDot + 1));
      }
      double v = Double.parseDouble(version);
      if (v < 10.7)
        throw new Exception("Operating system version is " + v);

      Class fsuClass = Class.forName("com.apple.eawt.FullScreenUtilities");
      Class argClasses[] = new Class[]{Window.class, Boolean.TYPE};
      Method setWindowCanFullScreen =
        fsuClass.getMethod("setWindowCanFullScreen", argClasses);
      setWindowCanFullScreen.invoke(fsuClass, this, true);

      canDoLionFS = true;
    } catch (Exception e) {
      vlog.debug("Could not enable OS X 10.7+ full-screen mode: " +
                 e.getMessage());
    }
  }

  public void toggleLionFS() {
    try {
      Class appClass = Class.forName("com.apple.eawt.Application");
      Method getApplication = appClass.getMethod("getApplication",
                                                 (Class[])null);
      Object app = getApplication.invoke(appClass);
      Method requestToggleFullScreen =
        appClass.getMethod("requestToggleFullScreen", Window.class);
      requestToggleFullScreen.invoke(app, this);
    } catch (Exception e) {
      vlog.debug("Could not toggle OS X 10.7+ full-screen mode: " +
                 e.getMessage());
    }
  }


  public boolean isMaximized()
  {
    int state = getExtendedState();
    return ((state & JFrame.MAXIMIZED_BOTH) == JFrame.MAXIMIZED_BOTH);
  }

  public Dimension getScreenSize() {
    return getScreenBounds().getSize();
  }

  public Rectangle getScreenBounds() {
    GraphicsEnvironment ge =
      GraphicsEnvironment.getLocalGraphicsEnvironment();
    Rectangle r = new Rectangle();
    if (fullScreenAllMonitors.getValue()) {
      for (GraphicsDevice gd : ge.getScreenDevices())
        for (GraphicsConfiguration gc : gd.getConfigurations())
          r = r.union(gc.getBounds());
    } else {
      GraphicsConfiguration gc = getGraphicsConfiguration();
      r = gc.getBounds();
    }
    return r;
  }

  public static Window getFullScreenWindow() {
    GraphicsEnvironment ge =
      GraphicsEnvironment.getLocalGraphicsEnvironment();
    for (GraphicsDevice gd : ge.getScreenDevices()) {
      Window fullScreenWindow = gd.getFullScreenWindow();
      if (fullScreenWindow != null)
        return fullScreenWindow;
    }
    return null;
  }

  public static void setFullScreenWindow(Window fullScreenWindow) {
    GraphicsEnvironment ge =
      GraphicsEnvironment.getLocalGraphicsEnvironment();
    if (fullScreenAllMonitors.getValue()) {
      for (GraphicsDevice gd : ge.getScreenDevices())
        gd.setFullScreenWindow(fullScreenWindow);
    } else {
      GraphicsDevice gd = ge.getDefaultScreenDevice();
      gd.setFullScreenWindow(fullScreenWindow);
    }
  }

  public void handleOptions()
  {

    if (fullScreen.getValue() && !fullscreen_active())
      fullscreen_on();
    else if (!fullScreen.getValue() && fullscreen_active())
      fullscreen_off();

    if (remoteResize.getValue()) {
      scroll.setHorizontalScrollBarPolicy(HORIZONTAL_SCROLLBAR_AS_NEEDED);
      scroll.setVerticalScrollBarPolicy(VERTICAL_SCROLLBAR_AS_NEEDED);
      remoteResize(w(), h());
    } else {
      String scaleString = scalingFactor.getValue();
      if (!scaleString.equals(lastScaleFactor)) {
        if (scaleString.matches("^[0-9]+$")) {
          scroll.setHorizontalScrollBarPolicy(HORIZONTAL_SCROLLBAR_AS_NEEDED);
          scroll.setVerticalScrollBarPolicy(VERTICAL_SCROLLBAR_AS_NEEDED);
          viewport.setScaledSize(cc.server.width(), cc.server.height());
        } else {
          scroll.setHorizontalScrollBarPolicy(HORIZONTAL_SCROLLBAR_NEVER);
          scroll.setVerticalScrollBarPolicy(VERTICAL_SCROLLBAR_NEVER);
          viewport.setScaledSize(w(), h());
        }

        if (isMaximized() || fullscreen_active()) {
          repositionViewport();
        } else {
          int dx = getInsets().left + getInsets().right;
          int dy = getInsets().top + getInsets().bottom;
          setSize(viewport.scaledWidth+dx, viewport.scaledHeight+dy);
        }

        repositionViewport();
        lastScaleFactor = scaleString;
      }
    }

    if (isVisible()) {
      toFront();
      requestFocus();
    }
  }

  public void handleFullscreenTimeout()
  {
    DesktopWindow self = (DesktopWindow)this;

    assert(self != null);

    self.delayedFullscreen = false;

    if (self.delayedDesktopSize) {
      self.handleDesktopSize();
      self.delayedDesktopSize = false;
    }
  }

  private CConn cc;
  private JScrollPane scroll;
  public Viewport viewport;

  private boolean firstUpdate;
  private boolean delayedFullscreen;
  private boolean delayedDesktopSize;
  private boolean canDoLionFS;
  private String lastScaleFactor;
  private Rectangle lastBounds;
  private int lastState;
  private Timer timer;
}

