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
import java.awt.Dialog.*;
import javax.swing.*;

class Dialog extends JDialog {

  public Dialog(boolean modal) {
    if (modal) {
      setModalityType(ModalityType.APPLICATION_MODAL);
    } else {
      setModalityType(ModalityType.MODELESS);
    }
  }

  public boolean showDialog(Component c) {
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
    fullScreenWindow = Viewport.getFullScreenWindow();
    if (fullScreenWindow != null)
      Viewport.setFullScreenWindow(null);

    setVisible(true);
    setFocusable(true);
    setAlwaysOnTop(true);
    return ret;
  }

  public boolean showDialog() {
    return showDialog(null);
  }

  public void endDialog() {
    setVisible(false);
    setFocusable(false);
    setAlwaysOnTop(false);
    if (fullScreenWindow != null)
      Viewport.setFullScreenWindow(fullScreenWindow);
  }

  // initDialog() can be overridden in a derived class.  Typically it is used
  // to make sure that checkboxes have the right state, etc.
  public void initDialog() {
  }

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

  private Window fullScreenWindow;
  protected boolean ret = true;

}
