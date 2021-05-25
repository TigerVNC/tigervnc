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
import java.io.File;
import java.nio.CharBuffer;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.filechooser.*;
import javax.swing.WindowConstants.*;
import java.util.*;

import com.tigervnc.rfb.*;

import static java.awt.GridBagConstraints.HORIZONTAL;
import static java.awt.GridBagConstraints.LINE_START;
import static java.awt.GridBagConstraints.LINE_END;
import static java.awt.GridBagConstraints.NONE;
import static java.awt.GridBagConstraints.REMAINDER;

import static com.tigervnc.vncviewer.Parameters.*;

class ServerDialog extends Dialog implements Runnable {

  @SuppressWarnings({"unchecked","rawtypes"})
  public ServerDialog(String defaultServerName,
                      CharBuffer vncServerName) {
    super(true);
    this.vncServerName = vncServerName;
    setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
    setTitle("VNC Viewer: Connection Details");
    setResizable(false);
    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        endDialog();
        System.exit(1);
      }
    });

    JLabel serverLabel = new JLabel("VNC server:", JLabel.RIGHT);
    String valueStr = new String(defaultServerName);
    ArrayList<String> servernames = new ArrayList<String>();
    if (!valueStr.isEmpty())
      servernames.add(valueStr);
    String history = UserPreferences.get("ServerDialog", "history");
    if (history != null) {
      for (String s : history.split(",")) {
        if (servernames.indexOf(s) < 0)
          servernames.add(s);
      }
    }
    serverName = new MyJComboBox(servernames.toArray());
    serverName.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComboBox s = (JComboBox)e.getSource();
        if (e.getActionCommand().equals("comboBoxEdited")) {
          s.insertItemAt(editor.getItem(), 0);
          s.setSelectedIndex(0);
        }
      }
    });
    if (servernames.size() == 0)
      serverName.setPrototypeDisplayValue("255.255.255.255:5900");

    serverName.setEditable(true);
    editor = serverName.getEditor();
    editor.getEditorComponent().addKeyListener(new KeyListener() {
      public void keyTyped(KeyEvent e) {}
      public void keyReleased(KeyEvent e) {}
      public void keyPressed(KeyEvent e) {
        if (KeyMap.get_keycode_fallback_extended(e) == KeyEvent.VK_ENTER) {
          serverName.insertItemAt(editor.getItem(), 0);
          serverName.setSelectedIndex(0);
          handleConnect();
        }
      }
    });

    Container contentPane = this.getContentPane();
    contentPane.setLayout(new GridBagLayout());

    JLabel icon = new JLabel(VncViewer.logoIcon);
    optionsButton = new JButton("Options...");
    optionsButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleOptions();
      }
    });
    JButton loadButton = new JButton("Load...");
    loadButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleLoad();
      }
    });
    JButton saveAsButton = new JButton("Save As...");
    saveAsButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleSaveAs();
      }
    });
    aboutButton = new JButton("About...");
    aboutButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleAbout();
      }
    });
    cancelButton = new JButton("Cancel");
    cancelButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleCancel();
      }
    });
    connectButton = new JButton("Connect   \u21B5");
    connectButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        handleConnect();
      }
    });

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
    contentPane.add(serverName,
                    new GridBagConstraints(2, 0,
                                           REMAINDER, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(5, 0, 5, 5),
                                           NONE, NONE));
    JPanel buttonPane1 = new JPanel();
    Box box = Box.createHorizontalBox();
    JSeparator separator1 = new JSeparator();
    JSeparator separator2 = new JSeparator();
    GroupLayout layout = new GroupLayout(buttonPane1);
    buttonPane1.setLayout(layout);
    layout.setAutoCreateGaps(false);
    layout.setAutoCreateContainerGaps(false);
		layout.setHorizontalGroup(
		  layout.createSequentialGroup()
		    .addGroup(layout.createParallelGroup(GroupLayout.Alignment.LEADING)
		      .addGroup(layout.createSequentialGroup()
		        .addGap(10)
		        .addComponent(optionsButton))
		      .addComponent(separator1)
		      .addGroup(layout.createSequentialGroup()
		        .addGap(10)
		        .addComponent(aboutButton)))
		    .addGroup(layout.createParallelGroup(GroupLayout.Alignment.TRAILING)
		      .addGroup(layout.createSequentialGroup()
		        .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		        .addComponent(loadButton)
		        .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		        .addComponent(saveAsButton)
		        .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		        .addComponent(box)
		        .addGap(10))
		      .addComponent(separator2)
		      .addGroup(layout.createSequentialGroup()
		        .addComponent(cancelButton)
		        .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		        .addComponent(connectButton)
		        .addGap(10)))
		);
		layout.setVerticalGroup(
		  layout.createSequentialGroup()
		    .addGroup(layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
		      .addComponent(optionsButton)
		      .addComponent(loadButton)
		      .addComponent(saveAsButton)
		      .addComponent(box))
		    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		    .addGroup(layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
		      .addComponent(separator1)
		      .addComponent(separator2))
		    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		    .addGroup(layout.createParallelGroup(GroupLayout.Alignment.BASELINE)
		      .addComponent(aboutButton)
		      .addComponent(cancelButton)
		      .addComponent(connectButton))
		    .addPreferredGap(LayoutStyle.ComponentPlacement.RELATED)
		);
		layout.linkSize(SwingConstants.HORIZONTAL,
                    optionsButton, loadButton, saveAsButton,
                    aboutButton, cancelButton, box);
    contentPane.add(buttonPane1,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(5, 0, 10, 0),
                                           NONE, NONE));
    pack();
  }

  public void run() {
    this.showDialog();
  }

  private void handleOptions() {
    // quirk for mac os x
    if (VncViewer.os.startsWith("mac os x"))
      this.setAlwaysOnTop(false);
    OptionsDialog.showDialog(this);
  }

  private void handleLoad() {
    String title = "Select a TigerVNC configuration file";
    File dflt = new File(FileUtils.getVncHomeDir().concat("default.tigervnc"));
    FileNameExtensionFilter filter =
      new FileNameExtensionFilter("TigerVNC configuration (*.tigervnc)", "tigervnc");
    File f = showChooser(title, dflt, filter);
    if (f != null && f.exists() && f.canRead())
      loadViewerParameters(f.getAbsolutePath());
  }

  private void handleSaveAs() {
    String title = "Save the TigerVNC configuration to file";
    File dflt = new File(FileUtils.getVncHomeDir().concat("default.tigervnc"));
    if (!dflt.exists() || !dflt.isFile())
      dflt = new File(FileUtils.getVncHomeDir());
    FileNameExtensionFilter filter =
      new FileNameExtensionFilter("TigerVNC configuration (*.tigervnc)", "tigervnc");
    File f = showChooser(title, dflt, filter);
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
        f = showChooser(title, f, filter);
    }
    if (f != null && (!f.exists() || f.canWrite()))
      saveViewerParameters(f.getAbsolutePath(), (String)serverName.getSelectedItem());
  }

  private void handleAbout() {
    VncViewer.about_vncviewer(this);
  }

  private void handleCancel() {
    endDialog();
  }

  private void handleConnect() {
    String servername = (String)serverName.getSelectedItem();
    servername.trim();
    vncServerName.put(servername).flip();
    saveViewerParameters(null, servername);
    endDialog();
  }

  @SuppressWarnings("rawtypes")
  MyJComboBox serverName;
  ComboBoxEditor editor;
  JButton aboutButton, optionsButton, connectButton, cancelButton;
  OptionsDialog options;
  CharBuffer vncServerName;
  static LogWriter vlog = new LogWriter("ServerDialog");

}
