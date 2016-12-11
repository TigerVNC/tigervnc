/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2014 Brian P. Hinz
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
import java.awt.Cursor;
import java.awt.event.*;
import java.io.File;
import javax.swing.filechooser.*;
import javax.swing.JCheckBoxMenuItem;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JFileChooser;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPopupMenu;

import com.tigervnc.rfb.*;

import static com.tigervnc.vncviewer.Parameters.*;

public class F8Menu extends JPopupMenu implements ActionListener {
  public F8Menu(CConn cc) {
    super("VNC Menu");
    setLightWeightPopupEnabled(false);
    String os = System.getProperty("os.name");
    if (os.startsWith("Windows"))
      com.sun.java.swing.plaf.windows.WindowsLookAndFeel.setMnemonicHidden(false);
    this.cc = cc;
    restore    = addMenuItem("Restore",KeyEvent.VK_R);
    restore.setEnabled(!embed.getValue());
    move       = addMenuItem("Move");
    move.setEnabled(false);
    size       = addMenuItem("Size");
    size.setEnabled(false);
    minimize   = addMenuItem("Minimize", KeyEvent.VK_N);
    minimize.setEnabled(!embed.getValue());
    maximize   = addMenuItem("Maximize", KeyEvent.VK_X);
    maximize.setEnabled(!embed.getValue());
    addSeparator();
    exit       = addMenuItem("Close Viewer", KeyEvent.VK_C);
    addSeparator();
    fullScreenCheckbox = new JCheckBoxMenuItem("Full Screen");
    fullScreenCheckbox.setMnemonic(KeyEvent.VK_F);
    fullScreenCheckbox.setSelected(fullScreen.getValue());
    fullScreenCheckbox.addActionListener(this);
    fullScreenCheckbox.setEnabled(!embed.getValue());
    add(fullScreenCheckbox);
    addSeparator();
    clipboard  = addMenuItem("Clipboard...");
    addSeparator();
    int keyCode = MenuKey.getMenuKeyCode();
    String keyText = KeyEvent.getKeyText(keyCode);
    f8 = addMenuItem("Send "+keyText, keyCode);
    ctrlAltDel = addMenuItem("Send Ctrl-Alt-Del");
    addSeparator();
    refresh    = addMenuItem("Refresh Screen", KeyEvent.VK_H);
    addSeparator();
    newConn    = addMenuItem("New connection...", KeyEvent.VK_W);
    newConn.setEnabled(!embed.getValue());
    options    = addMenuItem("Options...", KeyEvent.VK_O);
    save       = addMenuItem("Save connection info as...", KeyEvent.VK_S);
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
    } else if (actionMatch(ev, fullScreenCheckbox)) {
      if (fullScreenCheckbox.isSelected())
        cc.desktop.fullscreen_on();
      else
        cc.desktop.fullscreen_off();
    } else if (actionMatch(ev, restore)) {
      if (cc.desktop.fullscreen_active())
        cc.desktop.fullscreen_off();
      cc.desktop.setExtendedState(JFrame.NORMAL);
    } else if (actionMatch(ev, minimize)) {
      if (cc.desktop.fullscreen_active())
        cc.desktop.fullscreen_off();
      cc.desktop.setExtendedState(JFrame.ICONIFIED);
    } else if (actionMatch(ev, maximize)) {
      if (cc.desktop.fullscreen_active())
        cc.desktop.fullscreen_off();
      cc.desktop.setExtendedState(JFrame.MAXIMIZED_BOTH);
    } else if (actionMatch(ev, clipboard)) {
      ClipboardDialog.showDialog(cc.desktop);
    } else if (actionMatch(ev, f8)) {
      cc.writeKeyEvent(MenuKey.getMenuKeySym(), true);
      cc.writeKeyEvent(MenuKey.getMenuKeySym(), false);
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
      VncViewer.newViewer();
    } else if (actionMatch(ev, options)) {
      OptionsDialog.showDialog(cc.desktop);
    } else if (actionMatch(ev, save)) {
	    String title = "Save the TigerVNC configuration to file";
	    File dflt = new File(FileUtils.getVncHomeDir().concat("default.tigervnc"));
	    if (!dflt.exists() || !dflt.isFile())
	      dflt = new File(FileUtils.getVncHomeDir());
      FileNameExtensionFilter filter =
        new FileNameExtensionFilter("TigerVNC configuration (*.tigervnc)", "tigervnc");
	    File f = Dialog.showChooser(title, dflt, this, filter);
	    while (f != null && f.exists() && f.isFile()) {
	      String msg = f.getAbsolutePath();
	      msg = msg.concat(" already exists. Do you want to overwrite?");
	      Object[] options = {"Overwrite", "No  \u21B5"};
	      JOptionPane op =
	        new JOptionPane(msg, JOptionPane.QUESTION_MESSAGE,
	                        JOptionPane.OK_CANCEL_OPTION, null, options, options[1]);
	      JDialog dlg = op.createDialog(this, "TigerVNC Viewer");
	      dlg.setIconImage(VncViewer.frameIcon);
	      dlg.setAlwaysOnTop(true);
	      dlg.setVisible(true);
	      if (op.getValue() == options[0])
	        break;
	      else
	        f = Dialog.showChooser(title, f, this, filter);
	    }
	    if (f != null && (!f.exists() || f.canWrite()))
	      saveViewerParameters(f.getAbsolutePath(), vncServerName.getValue());
    } else if (actionMatch(ev, info)) {
      cc.showInfo();
    } else if (actionMatch(ev, about)) {
      VncViewer.showAbout(cc.desktop);
    } else if (actionMatch(ev, dismiss)) {
      firePopupMenuCanceled();
    }
  }

  public void show(Component invoker, int x, int y) {
    // lightweight components can't show in FullScreen Exclusive mode
    /*
    Window fsw = DesktopWindow.getFullScreenWindow();
    GraphicsDevice gd = null;
    if (fsw != null) {
      gd = fsw.getGraphicsConfiguration().getDevice();
      if (gd.isFullScreenSupported())
        DesktopWindow.setFullScreenWindow(null);
    }
    */
    super.show(invoker, x, y);
    /*
    if (fsw != null && gd.isFullScreenSupported())
      DesktopWindow.setFullScreenWindow(fsw);
      */
  }

  CConn cc;
  JMenuItem restore, move, size, minimize, maximize;
  JMenuItem exit, clipboard, ctrlAltDel, refresh;
  JMenuItem newConn, options, save, info, about, dismiss;
  static JMenuItem f8;
  JCheckBoxMenuItem fullScreenCheckbox;
  static LogWriter vlog = new LogWriter("F8Menu");
}
