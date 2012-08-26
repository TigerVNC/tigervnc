/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2012 Brian P. Hinz
 * Copyright (C) 2012 D. R. Commander.  All Rights Reserved.
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
import javax.swing.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rfb.*;

public class Viewport extends JFrame
{
  public Viewport(String name, CConn cc_) {
    cc = cc_;
    setTitle(name+" - TigerVNC");
    setFocusable(false);
    setFocusTraversalKeysEnabled(false);
    UIManager.getDefaults().put("ScrollPane.ancestorInputMap", 
      new UIDefaults.LazyInputMap(new Object[]{}));
    sp = new JScrollPane();
    sp.setBorder(BorderFactory.createEmptyBorder(0,0,0,0));
    getContentPane().add(sp);
    addWindowFocusListener(new WindowAdapter() {
      public void windowGainedFocus(WindowEvent e) {
        if (isVisible())
          sp.getViewport().getView().requestFocusInWindow();
      }
    });
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        cc.close();
      }
    });
    addComponentListener(new ComponentAdapter() {
      public void componentResized(ComponentEvent e) {
        if ((getExtendedState() != JFrame.MAXIMIZED_BOTH) &&
            cc.fullScreen) {
          cc.toggleFullScreen();
        }
        String scaleString = cc.viewer.scalingFactor.getValue();
        if (scaleString.equals("Auto") || scaleString.equals("FixedRatio")) {
          if ((sp.getSize().width != cc.desktop.scaledWidth) ||
              (sp.getSize().height != cc.desktop.scaledHeight)) {
            int policy = ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER;
            sp.setHorizontalScrollBarPolicy(policy);
            cc.desktop.setScaledSize();
            sp.setSize(new Dimension(cc.desktop.scaledWidth,
                                     cc.desktop.scaledHeight));
            sp.validate();
            if (getExtendedState() != JFrame.MAXIMIZED_BOTH &&
                scaleString.equals("FixedRatio")) {
              int w = cc.desktop.scaledWidth + getInsets().left + getInsets().right;
              int h = cc.desktop.scaledHeight + getInsets().top + getInsets().bottom;
              setSize(w, h);
            }
            if (cc.desktop.cursor != null) {
              Cursor cursor = cc.desktop.cursor;
              cc.setCursor(cursor.width(),cursor.height(),cursor.hotspot, 
                           cursor.data, cursor.mask);
            }
          }
        } else {
          int policy = ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED;
          sp.setHorizontalScrollBarPolicy(policy);
          sp.validate();
        }
      }
    });
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
    setBackground(Color.BLACK);
  }


  CConn cc;
  JScrollPane sp;
  static LogWriter vlog = new LogWriter("Viewport");
}

