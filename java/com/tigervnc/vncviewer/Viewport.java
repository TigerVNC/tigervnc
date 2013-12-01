/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2013 Brian P. Hinz
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

import java.awt.Color;
import java.awt.event.*;
import java.awt.Dimension;
import java.awt.Event;
import java.awt.GraphicsConfiguration;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Window;
import java.lang.reflect.*;
import javax.swing.*;

import com.tigervnc.rfb.*;
import java.lang.Exception;

public class Viewport extends JFrame
{
  public Viewport(String name, CConn cc_) {
    cc = cc_;
    setTitle(name+" - TigerVNC");
    setFocusable(false);
    setFocusTraversalKeysEnabled(false);
    setIconImage(VncViewer.frameIcon);
    UIManager.getDefaults().put("ScrollPane.ancestorInputMap",
      new UIDefaults.LazyInputMap(new Object[]{}));
    sp = new JScrollPane();
    sp.getViewport().setBackground(Color.BLACK);
    sp.setBorder(BorderFactory.createEmptyBorder(0,0,0,0));
    getContentPane().add(sp);
    if (VncViewer.os.startsWith("mac os x")) {
      if (!VncViewer.noLionFS.getValue())
        enableLionFS();
    }
    addWindowFocusListener(new WindowAdapter() {
      public void windowGainedFocus(WindowEvent e) {
        if (isVisible())
          sp.getViewport().getView().requestFocusInWindow();
      }
      public void windowLostFocus(WindowEvent e) {
        cc.releaseModifiers();
      }
    });
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        if (VncViewer.nViewers == 1) {
          cc.viewer.exit(1);
        } else {
          cc.close();
        }
      }
    });
    addComponentListener(new ComponentAdapter() {
      public void componentResized(ComponentEvent e) {
        String scaleString = cc.viewer.scalingFactor.getValue();
        if (scaleString.equalsIgnoreCase("Auto") ||
            scaleString.equalsIgnoreCase("FixedRatio")) {
          if ((sp.getSize().width != cc.desktop.scaledWidth) ||
              (sp.getSize().height != cc.desktop.scaledHeight)) {
            cc.desktop.setScaledSize();
            sp.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER);
            sp.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_NEVER);
            sp.validate();
            if (getExtendedState() != JFrame.MAXIMIZED_BOTH &&
                !cc.fullScreen) {
              sp.setSize(new Dimension(cc.desktop.scaledWidth,
                                       cc.desktop.scaledHeight));
              int w = cc.desktop.scaledWidth + getInsets().left + getInsets().right;
              int h = cc.desktop.scaledHeight + getInsets().top + getInsets().bottom;
              if (scaleString.equalsIgnoreCase("FixedRatio"))
                setSize(w, h);
            }
          }
        } else {
          sp.setHorizontalScrollBarPolicy(ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
          sp.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED);
          sp.validate();
        }
        if (cc.desktop.cursor != null) {
          Cursor cursor = cc.desktop.cursor;
          cc.setCursor(cursor.width(),cursor.height(),cursor.hotspot,
                       cursor.data, cursor.mask);
        }
      }
    });
  }

  boolean lionFSSupported() { return canDoLionFS; }

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

  public void setChild(DesktopWindow child) {
    sp.getViewport().setView(child);
  }

  public void setGeometry(int x, int y, int w, int h, boolean pack) {
    if (pack) {
      pack();
    } else {
      setSize(w, h);
    }
    if (!cc.fullScreen)
      setLocation(x, y);
  }

  public static Window getFullScreenWindow() {
      GraphicsEnvironment ge =
        GraphicsEnvironment.getLocalGraphicsEnvironment();
      GraphicsDevice gd = ge.getDefaultScreenDevice();
      Window fullScreenWindow = gd.getFullScreenWindow();
      return fullScreenWindow;
  }

  public static void setFullScreenWindow(Window fullScreenWindow) {
      GraphicsEnvironment ge =
        GraphicsEnvironment.getLocalGraphicsEnvironment();
      GraphicsDevice gd = ge.getDefaultScreenDevice();
      if (gd.isFullScreenSupported())
        gd.setFullScreenWindow(fullScreenWindow);
  }

  CConn cc;
  JScrollPane sp;
  boolean canDoLionFS;
  static LogWriter vlog = new LogWriter("Viewport");
}

