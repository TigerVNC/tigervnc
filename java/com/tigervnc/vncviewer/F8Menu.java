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

package com.tigervnc.vncviewer;

import java.awt.Cursor;
import java.awt.event.*;
import javax.swing.JFrame;
import javax.swing.JPopupMenu;
import javax.swing.JMenuItem;
import javax.swing.JCheckBoxMenuItem;

import com.tigervnc.rfb.*;

public class F8Menu extends JPopupMenu implements ActionListener {
  public F8Menu(CConn cc_) {
    super("VNC Menu");
    setLightWeightPopupEnabled(false);
    cc = cc_;
    restore    = addMenuItem("Restore",KeyEvent.VK_R);
    move       = addMenuItem("Move");
    move.setEnabled(false);
    size       = addMenuItem("Size");
    size.setEnabled(false);
    minimize   = addMenuItem("Minimize", KeyEvent.VK_N);
    maximize   = addMenuItem("Maximize", KeyEvent.VK_X);
    addSeparator();
    exit       = addMenuItem("Close Viewer", KeyEvent.VK_C);
    addSeparator();
    fullScreen = new JCheckBoxMenuItem("Full Screen");
    fullScreen.setMnemonic(KeyEvent.VK_F);
    fullScreen.addActionListener(this);
    add(fullScreen);
    addSeparator();
    clipboard  = addMenuItem("Clipboard...");
    addSeparator();
    f8 = addMenuItem("Send F"+(cc.menuKey-Keysyms.F1+1));
    ctrlAltDel = addMenuItem("Send Ctrl-Alt-Del");
    addSeparator();
    refresh    = addMenuItem("Refresh Screen", KeyEvent.VK_H);
    addSeparator();
    newConn    = addMenuItem("New connection...", KeyEvent.VK_W);
    options    = addMenuItem("Options...", KeyEvent.VK_O);
    info       = addMenuItem("Connection info...", KeyEvent.VK_I);
    about      = addMenuItem("About VncViewer...", KeyEvent.VK_A);
    addSeparator();
    dismiss    = addMenuItem("Dismiss menu");
    setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
  }

  JMenuItem addMenuItem(String str, int mnemonic) {
    JMenuItem item = new JMenuItem(str, mnemonic);
    item.addActionListener(this);
    add(item);
    return item;
  }

  JMenuItem addMenuItem(String str) {
    JMenuItem item = new JMenuItem(str);
    item.addActionListener(this);
    add(item);
    return item;
  }

  boolean actionMatch(ActionEvent ev, JMenuItem item) {
    return ev.getActionCommand().equals(item.getActionCommand());
  }

  public void actionPerformed(ActionEvent ev) {
    if (actionMatch(ev, exit)) {
      cc.close();
    } else if (actionMatch(ev, fullScreen)) {
      cc.toggleFullScreen();
    } else if (actionMatch(ev, restore)) {
      if (cc.fullScreen) cc.toggleFullScreen();
      cc.viewport.setExtendedState(JFrame.NORMAL);
    } else if (actionMatch(ev, minimize)) {
      if (cc.fullScreen) cc.toggleFullScreen();
      cc.viewport.setExtendedState(JFrame.ICONIFIED);
    } else if (actionMatch(ev, maximize)) {
      if (cc.fullScreen) cc.toggleFullScreen();
      cc.viewport.setExtendedState(JFrame.MAXIMIZED_BOTH);
    } else if (actionMatch(ev, clipboard)) {
      cc.clipboardDialog.showDialog();
    } else if (actionMatch(ev, f8)) {
      cc.writeKeyEvent(cc.menuKey, true);
      cc.writeKeyEvent(cc.menuKey, false);
    } else if (actionMatch(ev, ctrlAltDel)) {
      cc.writeKeyEvent(Keysyms.Control_L, true);
      cc.writeKeyEvent(Keysyms.Alt_L, true);
      cc.writeKeyEvent(Keysyms.Delete, true);
      cc.writeKeyEvent(Keysyms.Delete, false);
      cc.writeKeyEvent(Keysyms.Alt_L, false);
      cc.writeKeyEvent(Keysyms.Control_L, false);
    } else if (actionMatch(ev, refresh)) {
      cc.refresh();
    } else if (actionMatch(ev, newConn)) {
      VncViewer.newViewer(cc.viewer);
    } else if (actionMatch(ev, options)) {
      cc.options.showDialog();
    } else if (actionMatch(ev, info)) {
      cc.showInfo();
    } else if (actionMatch(ev, about)) {
      cc.showAbout();
    } else if (actionMatch(ev, dismiss)) {
      firePopupMenuCanceled();
    }
  }

  CConn cc;
  JMenuItem restore, move, size, minimize, maximize;
  JMenuItem exit, clipboard, ctrlAltDel, refresh;
  JMenuItem newConn, options, info, about, dismiss;
  static JMenuItem f8;
  JCheckBoxMenuItem fullScreen;
  static LogWriter vlog = new LogWriter("F8Menu");
}
