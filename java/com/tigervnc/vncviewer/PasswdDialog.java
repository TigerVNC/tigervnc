/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2012 Brian P. Hinz
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
import javax.swing.*;

import com.jcraft.jsch.*;
import com.tigervnc.rfb.*;

import static java.awt.GridBagConstraints.HORIZONTAL;
import static java.awt.GridBagConstraints.LINE_START;
import static java.awt.GridBagConstraints.NONE;
import static java.awt.GridBagConstraints.REMAINDER;

class PasswdDialog extends Dialog implements UserInfo,
                                             UIKeyboardInteractive {

  public PasswdDialog(String title,
                      boolean userDisabled, boolean passwdDisabled) {
    super(true);
    setResizable(false);
    setTitle(title);
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        endDialog();
      }
    });

    JPanel p1 = new JPanel(new GridBagLayout());
    p1.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

    userLabel = new JLabel("Username:");
    userLabel.setEnabled(!userDisabled);
    p1.add(userLabel, new GridBagConstraints(0, 0,
                                             1, 1,
                                             LIGHT, LIGHT,
                                             LINE_START, NONE,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));
    userEntry = new JTextField(30);
    userEntry.setEnabled(!userDisabled);
    p1.add(userEntry, new GridBagConstraints(1, 0,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             LINE_START, REMAINDER,
                                             new Insets(0, 5, 0, 0),
                                             NONE, NONE));

    passwdLabel = new JLabel("Password:");
    passwdLabel.setEnabled(!passwdDisabled);
    p1.add(passwdLabel, new GridBagConstraints(0, 1,
                                               1, 1,
                                               LIGHT, LIGHT,
                                               LINE_START, NONE,
                                               new Insets(5, 0, 0, 0),
                                               NONE, NONE));
    passwdEntry = new JPasswordField(30);
    passwdEntry.setEnabled(!passwdDisabled);
    p1.add(passwdEntry, new GridBagConstraints(1, 1,
                                               1, 1,
                                               HEAVY, LIGHT,
                                               LINE_START, REMAINDER,
                                               new Insets(5, 5, 0, 0),
                                               NONE, NONE));

    this.add(p1);
    addListeners(this);
    pack();
    if (userEntry.isEnabled()) {
      userEntry.requestFocus();
    } else {
      passwdEntry.requestFocus();
    }
  }

  /** Handle the key-pressed event. */
  public void keyPressed(KeyEvent event) {
    Object s = event.getSource();
    if (s instanceof JTextField && (JTextField)s == userEntry) {
       if (KeyMap.get_keycode_fallback_extended(event) == KeyEvent.VK_ENTER) {
         endDialog();
        }
    } else if (s instanceof JPasswordField
              && (JPasswordField)s == passwdEntry) {
        if (KeyMap.get_keycode_fallback_extended(event) == KeyEvent.VK_ENTER) {
         endDialog();
        }
    }
  }

  public String getPassword() {
    return new String(passwdEntry.getPassword());
  }

  public String getPassphrase() { return null; }
  public boolean promptPassphrase(String message) { return false; }

  public boolean promptPassword(String message) {
    setTitle(message);
    showDialog();
    if (userEntry.isEnabled())
      if (userEntry.getText().equals(""))
        return false;
      else if (!passwdEntry.isEnabled())
        return true;
    if (passwdEntry.isEnabled())
      if (!passwdEntry.getText().equals(""))
        return true;
    return false;
  }

  public void showMessage(String message) {
    JOptionPane.showMessageDialog(null, message, "Message",
                                  JOptionPane.PLAIN_MESSAGE);
  }

  public boolean promptYesNo(String str) {
    Object[] options={ "YES", "NO" };
    int ret=JOptionPane.showOptionDialog(null,
           str,
           "Warning",
           JOptionPane.DEFAULT_OPTION,
           JOptionPane.WARNING_MESSAGE,
           null, options, options[0]);
     return (ret == 0);
  }

  public String[] promptKeyboardInteractive(String destination,
                                            String name,
                                            String instruction,
                                            String[] prompt,
                                            boolean[] echo) {
    Container panel = new JPanel(new GridBagLayout());
    ((JPanel)panel).setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));

    panel.add(new JLabel(instruction),
              new GridBagConstraints(0, 0,
                                     REMAINDER, 1,
                                     LIGHT, LIGHT,
                                     LINE_START, NONE,
                                     new Insets(0, 0, 0, 0),
                                     NONE, NONE));

    JTextField[] texts=new JTextField[prompt.length];
    for (int i = 0; i < prompt.length; i++) {
      panel.add(new JLabel(prompt[i]),
                new GridBagConstraints(0, i+1,
                                       1, 1,
                                       LIGHT, LIGHT,
                                       LINE_START, NONE,
                                       new Insets(5, 0, 0, 0),
                                       NONE, NONE));

      if(echo[i])
        texts[i]=new JTextField(20);
      else
        texts[i]=new JPasswordField(20);

      panel.add(texts[i],
                new GridBagConstraints(1, i+1,
                                       1, 1,
                                       HEAVY, LIGHT,
                                       LINE_START, HORIZONTAL,
                                       new Insets(5, 5, 0, 0),
                                       NONE, NONE));
    }

    if (JOptionPane.showConfirmDialog(null, panel,
                                      destination+": "+name,
                                      JOptionPane.OK_CANCEL_OPTION,
                                      JOptionPane.QUESTION_MESSAGE)
        == JOptionPane.OK_OPTION) {
      String[] response=new String[prompt.length];
      for(int i=0; i<prompt.length; i++){
        response[i]=texts[i].getText();
      }
	    return response;
    } else{
      return null;  // cancel
    }
  }

  JLabel userLabel;
  JTextField userEntry;
  JLabel passwdLabel;
  JPasswordField passwdEntry;
  static LogWriter vlog = new LogWriter("PasswdDialog");
}
