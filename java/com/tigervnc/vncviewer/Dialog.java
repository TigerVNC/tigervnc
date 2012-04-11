/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 Brian P. Hinz
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
// This Dialog class implements a pop-up dialog.  This is needed because
// apparently you can't use the standard AWT Dialog from within an applet.  The
// dialog can be made visible by calling its showDialog() method.  Dialogs can
// be modal or non-modal.  For a modal dialog box, the showDialog() method must
// be called from a thread other than the GUI thread, and it only returns when
// the dialog box has been dismissed.  For a non-modal dialog box, the
// showDialog() method returns immediately.

package com.tigervnc.vncviewer;

import java.awt.*;
import javax.swing.*;

//class Dialog extends JFrame implements WindowListener {
class Dialog extends JFrame {

  protected boolean ok, done;
  boolean modal;

  public Dialog(boolean modal_) {
    modal = modal_;
    //addWindowListener(this);
  }

  public boolean showDialog(Component c) {
    ok = false;
    done = false;
    initDialog();
    if (c != null) {
      setLocationRelativeTo(c);
    } else {
      Dimension dpySize = getToolkit().getScreenSize();
      Dimension mySize = getSize();
      int x = (dpySize.width - mySize.width) / 2;
      int y = (dpySize.height - mySize.height) / 2;
      setLocation(x, y);
    }
    ClassLoader cl = this.getClass().getClassLoader();
    ImageIcon icon = new ImageIcon(cl.getResource("com/tigervnc/vncviewer/tigervnc.ico"));
    setIconImage(icon.getImage());
    //setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
    //setFont(new Font("SansSerif", Font.PLAIN, 11));

    setVisible(true);
    setFocusable(true);
    if (!modal) return true;
    synchronized(this) {
      try {
        while (!done)
          wait();
      } catch (InterruptedException e) {
      }
    }
    return ok;
  }

  public boolean showDialog() {
    return showDialog(null);
  }

  public void endDialog() {
    done = true;
    setVisible(false);
    setFocusable(false);
    if (modal) {
      synchronized (this) {
        notify();
      }
    }
  }

  // initDialog() can be overridden in a derived class.  Typically it is used
  // to make sure that checkboxes have the right state, etc.
  public void initDialog() {
  }

  //------------------------------------------------------------------ 
  //   implemented blank methods
  //public void windowClosed(WindowEvent event){}
  //public void windowDeiconified(WindowEvent event){}
  //public void windowIconified(WindowEvent event){}
  //public void windowActivated(WindowEvent event){}
  //public void windowDeactivated(WindowEvent event){}
  //public void windowOpened(WindowEvent event){}

  //------------------------------------------------------------------

  // method to check which window was closing
  //public void windowClosing(WindowEvent event) {
  //  ok = false;
  //  endDialog();
  //}

  public void addGBComponent(JComponent c, JComponent cp,
                             int gx, int gy, 
                             int gw, int gh, 
                             int gipx, int gipy,
                             double gwx, double gwy, 
                             int fill, int anchor,
                             Insets insets)
  {
      GridBagConstraints gbc = new GridBagConstraints();
      gbc.anchor = anchor;
      gbc.fill = fill;
      gbc.gridx = gx;
      gbc.gridy = gy;
      gbc.gridwidth = gw;
      gbc.gridheight = gh;
      gbc.insets = insets;
      gbc.ipadx = gipx;
      gbc.ipady = gipy;
      gbc.weightx = gwx;
      gbc.weighty = gwy;
      cp.add(c, gbc);
  }

  final public String getFileSeperator() {
    String seperator = System.getProperties().get("file.separator").toString();
    return seperator;
  }

  final public String getUserName() {
    String userName = (String)System.getProperties().get("user.name");
    return userName;
  }

}
