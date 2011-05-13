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

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.net.URL;

class PasswdDialog extends Dialog implements KeyListener{

  public PasswdDialog(String title, boolean userDisabled, boolean passwdDisabled) {
    super(true);
    setResizable(false);
    setTitle(title);
    setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);

    JPanel p1 = new JPanel();
    userLabel = new JLabel("Username:");
    p1.add(userLabel);
    userEntry = new JTextField(30);
    userEntry.setEnabled(!userDisabled);
    userLabel.setEnabled(!userDisabled);
    p1.add(userEntry);
    userEntry.addKeyListener(this);

    JPanel p2 = new JPanel();
    passwdLabel = new JLabel("Password:");
    passwdLabel.setPreferredSize(userLabel.getPreferredSize());
    p2.add(passwdLabel);
    passwdEntry = new JPasswordField(30);
    passwdEntry.setEnabled(!passwdDisabled);
    passwdLabel.setEnabled(!passwdDisabled);
    p2.add(passwdEntry);
    passwdEntry.addKeyListener(this);

    getContentPane().setLayout(new BoxLayout(getContentPane(),BoxLayout.Y_AXIS));
    getContentPane().add(p1);
    getContentPane().add(p2);
    pack();
    if (userEntry.isEnabled()) {
      userEntry.requestFocus();
    } else {
      passwdEntry.requestFocus();
    }
  }

  /** Handle the key-typed event. */
  public void keyTyped(KeyEvent event) { }
  /** Handle the key-released event. */
  public void keyReleased(KeyEvent event) { }
  /** Handle the key-pressed event. */
  public void keyPressed(KeyEvent event) {
    Object s = event.getSource();
    if (s instanceof JTextField && (JTextField)s == userEntry) {
       if (event.getKeyCode() == KeyEvent.VK_ENTER) {
         ok = true;
         endDialog();
        }
    } else if (s instanceof JPasswordField && (JPasswordField)s == passwdEntry) {
        if (event.getKeyCode() == KeyEvent.VK_ENTER) {
         ok = true;
         endDialog();
        }
    }
  }

  JLabel userLabel;
  JTextField userEntry;
  JLabel passwdLabel;
  JTextField passwdEntry;
}
