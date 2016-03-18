/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2016 Brian P. Hinz
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
import javax.swing.border.*;
import javax.swing.WindowConstants.*;
import java.util.*;

import com.tigervnc.rfb.*;

import static java.awt.GridBagConstraints.HORIZONTAL;
import static java.awt.GridBagConstraints.LINE_START;
import static java.awt.GridBagConstraints.NONE;
import static java.awt.GridBagConstraints.REMAINDER;

class ServerDialog extends Dialog {

  @SuppressWarnings({"unchecked","rawtypes"})
  public ServerDialog(OptionsDialog options_,
                      String defaultServerName, CConn cc_) {

    super(true);
    cc = cc_;
    setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
    setTitle("VNC Viewer: Connection Details");
    setResizable(false);
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        if (VncViewer.nViewers == 1) {
          cc.viewer.exit(1);
        } else {
          ret = false;
          endDialog();
        }
      }
    });

    options = options_;

    JLabel serverLabel = new JLabel("VNC Server:", JLabel.RIGHT);
    String valueStr = new String("");
    if (UserPreferences.get("ServerDialog", "history") != null)
      valueStr = UserPreferences.get("ServerDialog", "history");
    server = new MyJComboBox(valueStr.split(","));
    if (valueStr.equals(""))
      server.setPrototypeDisplayValue("255.255.255.255:5900");

    server.setEditable(true);
    editor = server.getEditor();
    editor.getEditorComponent().addKeyListener(new KeyListener() {
      public void keyTyped(KeyEvent e) {}
      public void keyReleased(KeyEvent e) {}
      public void keyPressed(KeyEvent e) {
        if (e.getKeyCode() == KeyEvent.VK_ENTER) {
          server.insertItemAt(editor.getItem(), 0);
          server.setSelectedIndex(0);
          commit();
        }
      }
    });

    Container contentPane = this.getContentPane();
    contentPane.setLayout(new GridBagLayout());

    JLabel icon = new JLabel(VncViewer.logoIcon);
    optionsButton = new JButton("Options...");
    aboutButton = new JButton("About...");
    okButton = new JButton("OK");
    cancelButton = new JButton("Cancel");

    contentPane.add(icon,
                    new GridBagConstraints(0, 0,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(5, 5, 5, 5),
                                           NONE, NONE));
    contentPane.add(serverLabel,
                    new GridBagConstraints(1, 0,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(5, 10, 5, 5),
                                           NONE, NONE));
    contentPane.add(server,
                    new GridBagConstraints(2, 0,
                                           REMAINDER, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(5, 0, 5, 5),
                                           NONE, NONE));
    JPanel buttonPane = new JPanel();
    buttonPane.setLayout(new GridLayout(1, 4, 5, 5));
    buttonPane.add(aboutButton);
    buttonPane.add(optionsButton);
    buttonPane.add(okButton);
    buttonPane.add(cancelButton);
    contentPane.add(buttonPane,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(5, 5, 5, 5),
                                           NONE, NONE));
    addListeners(this);
    pack();
  }

  @SuppressWarnings({"unchecked","rawtypes"})
  public void actionPerformed(ActionEvent e) {
    Object s = e.getSource();
    if (s instanceof JButton && (JButton)s == okButton) {
      commit();
    } else if (s instanceof JButton && (JButton)s == cancelButton) {
      if (VncViewer.nViewers == 1)
        cc.viewer.exit(1);
      ret = false;
      endDialog();
    } else if (s instanceof JButton && (JButton)s == optionsButton) {
      options.showDialog(this);
    } else if (s instanceof JButton && (JButton)s == aboutButton) {
      cc.showAbout();
    } else if (s instanceof JComboBox && (JComboBox)s == server) {
      if (e.getActionCommand().equals("comboBoxEdited")) {
        server.insertItemAt(editor.getItem(), 0);
        server.setSelectedIndex(0);
      }
    }
  }

  private void commit() {
    String serverName = (String)server.getSelectedItem();
    if (serverName == null || serverName.equals("")) {
      vlog.error("Invalid servername specified");
      if (VncViewer.nViewers == 1)
        cc.viewer.exit(1);
      ret = false;
      endDialog();
    }
    // set params
    Configuration.setParam("Server", Hostname.getHost(serverName));
    Configuration.setParam("Port",
                            Integer.toString(Hostname.getPort(serverName)));
    // Update the history list
    String valueStr = UserPreferences.get("ServerDialog", "history");
    String t = (valueStr == null) ? "" : valueStr;
    StringTokenizer st = new StringTokenizer(t, ",");
    StringBuffer sb =
        new StringBuffer().append((String)server.getSelectedItem());
    while (st.hasMoreTokens()) {
      String str = st.nextToken();
      if (!str.equals((String)server.getSelectedItem()) && !str.equals("")) {
        sb.append(',');
        sb.append(str);
      }
    }
    UserPreferences.set("ServerDialog", "history", sb.toString());
    UserPreferences.save("ServerDialog");
    endDialog();
  }

  CConn cc;
  @SuppressWarnings("rawtypes")
  MyJComboBox server;
  ComboBoxEditor editor;
  JButton aboutButton, optionsButton, okButton, cancelButton;
  OptionsDialog options;
  static LogWriter vlog = new LogWriter("ServerDialog");

}
