/* Copyright (C) 2017 Brian P. Hinz
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
import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;
import javax.swing.plaf.LayerUI;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Point;
import com.tigervnc.rfb.Exception;

import static com.tigervnc.vncviewer.Parameters.*;
import static javax.swing.GroupLayout.*;
import static javax.swing.JOptionPane.*;


public class UserDialog implements UserPasswdGetter, UserMsgBox
{
  private class MyLayerUI extends LayerUI {
    // Using a JButton for the "?" icon yields the best look, but there
    // does not seem to be any reasonable way to disable a JButton without
    // also changing the color.  This wrapper just intercepts any mouse
    // click events so that the button just looks like an icon.
    @Override
    public void eventDispatched(AWTEvent e, JLayer l) {
      if (e instanceof InputEvent)
        ((InputEvent) e).consume();
    }

    @Override
    public void installUI(JComponent c) {
      super.installUI(c);
      if (c instanceof JLayer) {
        JLayer<?> layer = (JLayer<?>)c;
        layer.setLayerEventMask(AWTEvent.MOUSE_EVENT_MASK);
      }
    }

    @Override
    protected void processMouseEvent(MouseEvent e, JLayer l) {
      super.processMouseEvent(e, l);
    }
  }

  public final void getUserPasswd(boolean secure, StringBuffer user, StringBuffer password)
  {
    String passwordFileStr = passwordFile.getValue();

    if ((password == null) && sendLocalUsername.getValue()) {
      user.append((String)System.getProperties().get("user.name"));
      return;
    }

    if (user == null && !passwordFileStr.equals("")) {
      InputStream fp = null;
      try {
        fp = new FileInputStream(passwordFileStr);
      } catch(FileNotFoundException e) {
        throw new Exception("Opening password file failed");
      }
      byte[] obfPwd = new byte[256];
      try {
        fp.read(obfPwd);
        fp.close();
      } catch(IOException e) {
        throw new Exception("Failed to read VncPasswd file");
      }
      String PlainPasswd = VncAuth.unobfuscatePasswd(obfPwd);
      password.append(PlainPasswd);
      password.setLength(PlainPasswd.length());
      return;
    }

    JDialog win;
    JLabel banner;
    JTextField username = null;
    JLayer icon;

    int y;

    JPanel msg = new JPanel(null);
    msg.setSize(410, 145);

    banner = new JLabel();
    banner.setBounds(0, 0, msg.getPreferredSize().width, 20);
    banner.setHorizontalAlignment(JLabel.CENTER);
    banner.setOpaque(true);

    if (secure) {
      banner.setText("This connection is secure");
      banner.setBackground(Color.GREEN);
      ImageIcon secure_icon =
        new ImageIcon(VncViewer.class.getResource("secure.png"));
      banner.setIcon(secure_icon);
    } else {
      banner.setText("This connection is not secure");
      banner.setBackground(Color.RED);
      ImageIcon insecure_icon =
        new ImageIcon(VncViewer.class.getResource("insecure.png"));
      banner.setIcon(insecure_icon);
    }
    msg.add(banner);

    y = 20 + 10;

    JButton iconb = new JButton("?");
    iconb.setVerticalAlignment(JLabel.CENTER);
    iconb.setFont(new Font("Times", Font.BOLD, 34));
    iconb.setForeground(Color.BLUE);
    LayerUI ui = new MyLayerUI();
    icon = new JLayer(iconb, ui);
    icon.setBounds(10, y, 50, 50);
    msg.add(icon);

    y += 5;

    if (user != null && !sendLocalUsername.getValue()) {
      JLabel userLabel = new JLabel("Username:");
      userLabel.setBounds(70, y, msg.getSize().width-70-10, 20);
      msg.add(userLabel);
      y += 20 + 5;
      username = new JTextField(30);
      username.setBounds(70, y, msg.getSize().width-70-10, 25);
      msg.add(username);
      y += 25 + 5;
    }

    JLabel passwdLabel = new JLabel("Password:");
    passwdLabel.setBounds(70, y, msg.getSize().width-70-10, 20);
    msg.add(passwdLabel);
    y += 20 + 5;
    final JPasswordField passwd = new JPasswordField(30);
    passwd.setBounds(70, y, msg.getSize().width-70-10, 25);
    msg.add(passwd);
    y += 25 + 5;

    msg.setPreferredSize(new Dimension(410, y));

    Object[] options = {"OK  \u21B5", "Cancel"};
    JOptionPane pane = new JOptionPane(msg,
                                      PLAIN_MESSAGE,
                                      OK_CANCEL_OPTION,
                                      null,       //do not use a custom Icon
                                      options,    //the titles of buttons
                                      options[0]){//default button title
      @Override
      public void selectInitialValue() {
        passwd.requestFocusInWindow();
      }
    };
    pane.setBorder(new EmptyBorder(0,0,0,0));
    Component c = pane.getComponent(pane.getComponentCount()-1);
    ((JComponent)c).setBorder(new EmptyBorder(0,0,10,10));
    win = pane.createDialog("VNC Authentication");

    win.setVisible(true);

    if (pane.getValue() == null || pane.getValue().equals("Cancel"))
      throw new Exception("Authentication cancelled");

    if (user != null)
      if (sendLocalUsername.getValue())
         user.append((String)System.getProperties().get("user.name"));
      else
         user.append(username.getText());
    if (password != null)
      password.append(new String(passwd.getPassword()));
  }

  public boolean showMsgBox(int flags, String title, String text)
  {
    switch (flags & 0xf) {
    case OK_CANCEL_OPTION:
      return (showConfirmDialog(null, text, title, OK_CANCEL_OPTION) == OK_OPTION);
    case YES_NO_OPTION:
      return (showConfirmDialog(null, text, title, YES_NO_OPTION) == YES_OPTION);
    default:
      if (((flags & 0xf0) == ERROR_MESSAGE) ||
          ((flags & 0xf0) == WARNING_MESSAGE))
        showMessageDialog(null, text, title, (flags & 0xf0));
      else
        showMessageDialog(null, text, title, PLAIN_MESSAGE);
      return true;
    }
  }
}
