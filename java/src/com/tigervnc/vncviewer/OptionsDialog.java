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
import javax.swing.border.*;
import javax.swing.filechooser.*;
import javax.swing.ImageIcon;
import java.net.URL;
import java.io.IOException;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;

class OptionsDialog extends Dialog implements
                            ActionListener,
                            ItemListener
{

  // Constants
  // Static variables
  static LogWriter vlog = new LogWriter("OptionsDialog");

  OptionsDialogCallback cb;
  JPanel FormatPanel, InputsPanel, MiscPanel, DefaultsPanel, SecPanel;
  JCheckBox autoSelect, customCompressLevel, noJpeg;
  JComboBox menuKey, compressLevel, qualityLevel, scalingFactor;
  ButtonGroup encodingGroup, colourGroup;
  JRadioButton zrle, hextile, tight, raw;
  JRadioButton fullColour, mediumColour, lowColour, veryLowColour;
  JCheckBox viewOnly, acceptClipboard, sendClipboard, acceptBell;
  JCheckBox fullScreen, shared, useLocalCursor, fastCopyRect;
  JCheckBox secVeNCrypt, encNone, encTLS, encX509;
  JCheckBox secNone, secVnc, secPlain, secIdent, sendLocalUsername;
  JButton okButton, cancelButton;
  JButton ca, crl;
  JButton defSaveButton;
  UserPrefs defaults;

  boolean autoScale = false;
  boolean fixedRatioScale = false;

  public OptionsDialog(OptionsDialogCallback cb_) { 
    super(false);
    cb = cb_;
    setResizable(false);
    setTitle("VNC Viewer Options");
    defaults = new UserPrefs("vncviewer");

    getContentPane().setLayout(
      new BoxLayout(getContentPane(), BoxLayout.PAGE_AXIS));
	
    JTabbedPane tabPane = new JTabbedPane();

    ButtonGroup encodingGroup = new ButtonGroup();
    ButtonGroup colourGroup = new ButtonGroup();

    // Colour & Encoding tab
    FormatPanel=new JPanel(new GridBagLayout());

    autoSelect = new JCheckBox("Auto Select");
    autoSelect.addItemListener(this);

    JPanel encodingPanel = new JPanel(new GridBagLayout());
    encodingPanel.setBorder(BorderFactory.createTitledBorder("Preferred encoding"));
    zrle = addRadioCheckbox("ZRLE", encodingGroup, encodingPanel);
    hextile = addRadioCheckbox("Hextile", encodingGroup, encodingPanel);
    tight = addRadioCheckbox("Tight", encodingGroup, encodingPanel);
    raw = addRadioCheckbox("Raw", encodingGroup, encodingPanel);

    JPanel tightPanel = new JPanel(new GridBagLayout());
    customCompressLevel = new JCheckBox("Custom Compression Level");
    customCompressLevel.addItemListener(this);
    Object[] compressionLevels = { 1, 2, 3, 4, 5, 6 };
    compressLevel  = new JComboBox(compressionLevels);
    compressLevel.setEditable(true);
    JLabel compressionLabel = new JLabel("Level (1=fast, 6=best [4-6 are rarely useful])");
    noJpeg = new JCheckBox("Allow JPEG Compression");
    noJpeg.addItemListener(this);
    Object[] qualityLevels = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    qualityLevel  = new JComboBox(qualityLevels);
    JLabel qualityLabel = new JLabel("Level (0=poor, 9=best)");
    compressLevel.setPreferredSize(qualityLevel.getPreferredSize());
    // Hack to set the left inset on editable JComboBox
    if (UIManager.getLookAndFeel().getID() == "Windows") {
      compressLevel.setBorder(BorderFactory.createCompoundBorder(compressLevel.getBorder(),
        BorderFactory.createEmptyBorder(0,1,0,0)));
    } else {
      ComboBoxEditor editor = compressLevel.getEditor();
      JTextField jtf = (JTextField)editor.getEditorComponent();
      jtf.setBorder(new CompoundBorder(jtf.getBorder(), new EmptyBorder(0,1,0,0)));
    }
    addGBComponent(customCompressLevel, tightPanel, 0, 0, 2, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(0,2,0,0));
    addGBComponent(compressLevel, tightPanel,       0, 1, 1, 1, 2, 2, 0, 0, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(0,20,0,0));
    addGBComponent(compressionLabel, tightPanel,    1, 1, 1, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(0,5,0,0));
    addGBComponent(noJpeg, tightPanel,              0, 2, 2, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(0,2,0,0));
    addGBComponent(qualityLevel, tightPanel,        0, 3, 1, 1, 2, 2, 0, 0, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(0,20,0,0));
    addGBComponent(qualityLabel, tightPanel,        1, 3, 1, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(0,5,0,0));


    JPanel colourPanel = new JPanel(new GridBagLayout());
    colourPanel.setBorder(BorderFactory.createTitledBorder("Colour level"));
    fullColour = addRadioCheckbox("Full (all available colours)", colourGroup, colourPanel);
    mediumColour = addRadioCheckbox("Medium (256 colours)", colourGroup, colourPanel);
    lowColour = addRadioCheckbox("Low (64 colours)", colourGroup, colourPanel);
    veryLowColour = addRadioCheckbox("Very low(8 colours)", colourGroup, colourPanel);

    addGBComponent(autoSelect,FormatPanel,    0, 0, 2, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(0,2,0,0));
    addGBComponent(encodingPanel,FormatPanel, 0, 1, 1, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(0,2,0,0));
    addGBComponent(colourPanel,FormatPanel,   1, 1, 1, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_END, new Insets(0,2,0,0));
    addGBComponent(tightPanel,FormatPanel,    0, 2, 2, GridBagConstraints.REMAINDER, 2, 2, 1, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(0,2,0,0));

    // Inputs tab
    InputsPanel=new JPanel(new GridBagLayout());

    viewOnly = new JCheckBox("View Only (ignore mouse & keyboard)");
    viewOnly.addItemListener(this);
    acceptClipboard = new JCheckBox("Accept clipboard from server");
    acceptClipboard.addItemListener(this);
    sendClipboard = new JCheckBox("Send clipboard to server");
    sendClipboard.addItemListener(this);
    JLabel menuKeyLabel = new JLabel("Menu Key");
    String[] menuKeys = 
      { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12" };
    menuKey  = new JComboBox(menuKeys);
    menuKey.addItemListener(this);
    addGBComponent(viewOnly,InputsPanel,        0, 0, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(acceptClipboard,InputsPanel, 0, 1, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(sendClipboard,InputsPanel,   0, 2, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(menuKeyLabel,InputsPanel,    0, 3, 1, GridBagConstraints.REMAINDER, 0, 0, 1, 1, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(8,8,0,4));
    addGBComponent(menuKey,InputsPanel,         1, 3, 1, GridBagConstraints.REMAINDER, 0, 0, 25, 1, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(4,4,0,4));

    // Misc tab
    MiscPanel=new JPanel(new GridBagLayout());

    fullScreen = new JCheckBox("Full-screen mode");
    fullScreen.addItemListener(this);
    shared = new JCheckBox("Shared connection (do not disconnect other viewers)");
    shared.addItemListener(this);
    useLocalCursor = new JCheckBox("Render cursor locally");
    useLocalCursor.addItemListener(this);
    fastCopyRect = new JCheckBox("Fast CopyRect");
    fastCopyRect.addItemListener(this);
    acceptBell = new JCheckBox("Beep when requested by the server");
    acceptBell.addItemListener(this);
    JLabel scalingFactorLabel = new JLabel("Scaling Factor");
    Object[] scalingFactors = { 
      "Auto", "Fixed Aspect Ratio", "50%", "75%", "95%", "100%", "105%", 
      "125%", "150%", "175%", "200%", "250%", "300%", "350%", "400%" };
    scalingFactor = new JComboBox(scalingFactors);
    scalingFactor.setEditable(true);
    scalingFactor.addItemListener(this);
    addGBComponent(fullScreen,MiscPanel,     0, 0, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(shared,MiscPanel,         0, 1, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(useLocalCursor,MiscPanel, 0, 2, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(fastCopyRect,MiscPanel,   0, 3, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(4,4,0,4));
    addGBComponent(acceptBell,MiscPanel,     0, 4, 2, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(4,4,0,4));
    addGBComponent(scalingFactorLabel,MiscPanel, 0, 5, 1, GridBagConstraints.REMAINDER, 0, 0, 1, 1, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(8,8,0,4));
    addGBComponent(scalingFactor,MiscPanel, 1, 5, 1, GridBagConstraints.REMAINDER, 0, 0, 25, 1, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(4,4,0,4));

    // load/save tab
    DefaultsPanel=new JPanel(new GridBagLayout());

    JPanel configPanel = new JPanel(new GridBagLayout());
    configPanel.setBorder(BorderFactory.createTitledBorder("Configuration File"));
    JButton cfReloadButton = new JButton("Reload");
    cfReloadButton.addActionListener(this);
    addGBComponent(cfReloadButton,configPanel, 0, 0, 1, 1, 0, 0, 0, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.CENTER, new Insets(4,8,4,8));
    JButton cfSaveButton = new JButton("Save");
    cfSaveButton.addActionListener(this);
    addGBComponent(cfSaveButton,configPanel, 0, 1, 1, 1, 0, 0, 0, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.CENTER, new Insets(4,8,4,8));
    JButton cfSaveAsButton = new JButton("Save As...");
    cfSaveAsButton.addActionListener(this);
    addGBComponent(cfSaveAsButton,configPanel, 0, 2, 1, 1, 0, 0, 1, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.CENTER, new Insets(4,8,4,8));
    cfReloadButton.setEnabled(false);
    cfSaveButton.setEnabled(false);
    //cfSaveAsButton.setEnabled(!applet);

    JPanel defaultsPanel = new JPanel(new GridBagLayout());
    defaultsPanel.setBorder(BorderFactory.createTitledBorder("Defaults"));
    JButton defReloadButton = new JButton("Reload");
    defReloadButton.addActionListener(this);
    addGBComponent(defReloadButton,defaultsPanel, 0, 0, 1, 1, 0, 0, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.CENTER, new Insets(4,8,4,8));
    defSaveButton = new JButton("Save");
    defSaveButton.addActionListener(this);
    addGBComponent(defSaveButton,defaultsPanel, 0, 1, 1, 1, 0, 0, 0, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.CENTER, new Insets(4,8,4,8));

    addGBComponent(configPanel,DefaultsPanel, 0, 0, 1, GridBagConstraints.REMAINDER, 0, 0, 1, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.PAGE_START, new Insets(4,4,4,4));
    addGBComponent(defaultsPanel,DefaultsPanel, 1, 0, 1, GridBagConstraints.REMAINDER, 0, 0, 1, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.PAGE_START, new Insets(4,4,4,4));
    //defReloadButton.setEnabled(!applet);
    //defSaveButton.setEnabled(!applet);

    // security tab
    SecPanel=new JPanel(new GridBagLayout());

    JPanel encryptionPanel = new JPanel(new GridBagLayout());
    encryptionPanel.setBorder(BorderFactory.createTitledBorder("Session Encryption"));
    encNone = addCheckbox("None", null, encryptionPanel);
    encTLS = addCheckbox("Anonymous TLS", null, encryptionPanel);
    encX509 = addJCheckBox("TLS with X.509 certificates", null, encryptionPanel, new GridBagConstraints(0,2,1,1,1,1,GridBagConstraints.LINE_START,GridBagConstraints.REMAINDER,new Insets(0,0,0,60),0,0));

    JPanel x509Panel = new JPanel(new GridBagLayout());
    x509Panel.setBorder(BorderFactory.createTitledBorder("X.509 certificates"));
    ca = new JButton("Load CA certificate");
    ca.setPreferredSize(new Dimension(145,25));
    ca.addActionListener(this);
    crl = new JButton("Load CRL certificate");
    crl.setPreferredSize(new Dimension(145,25));
    crl.addActionListener(this);
    addGBComponent(ca, x509Panel,  0, 0, 1, 1, 2, 2, 0, 1, GridBagConstraints.NONE, GridBagConstraints.LINE_START, new Insets(2,2,2,2));
    addGBComponent(crl, x509Panel, 1, 0, 1, 1, 2, 2, 1, 1, GridBagConstraints.NONE, GridBagConstraints.LINE_START, new Insets(2,2,2,2));

    JPanel authPanel = new JPanel(new GridBagLayout());
    authPanel.setBorder(BorderFactory.createTitledBorder("Authentication"));
    secNone = addCheckbox("None", null, authPanel);
    secVnc = addCheckbox("Standard VNC", null, authPanel);
    secPlain = addJCheckBox("Plaintext", null, authPanel, new GridBagConstraints(0,2,1,1,1,1,GridBagConstraints.LINE_START,GridBagConstraints.NONE,new Insets(0,0,0,5),0,0));
    secIdent = addJCheckBox("Ident", null, authPanel, new GridBagConstraints(0,3,1,1,1,1,GridBagConstraints.LINE_START,GridBagConstraints.NONE,new Insets(0,0,0,5),0,0));
    sendLocalUsername = new JCheckBox("Send Local Username");
    sendLocalUsername.addItemListener(this);
    addGBComponent(sendLocalUsername, authPanel, 1, 2, 1, 2, 0, 0, 2, 1, GridBagConstraints.HORIZONTAL, GridBagConstraints.LINE_START, new Insets(0,20,0,0));

    secVeNCrypt = new JCheckBox("Extended encryption and authentication methods (VeNCrypt)");
    secVeNCrypt.addItemListener(this);
    addGBComponent(secVeNCrypt,SecPanel,        0, 0, 1, 1, 2, 2, 1, 0, GridBagConstraints.HORIZONTAL, GridBagConstraints.FIRST_LINE_START, new Insets(0,2,0,20));
    addGBComponent(encryptionPanel,SecPanel, 0, 1, 1, 1, 2, 2, 1, 0, GridBagConstraints.NONE, GridBagConstraints.LINE_START, new Insets(0,4,2,4));
    addGBComponent(x509Panel,SecPanel,       0, 2, 1, 1, 2, 2, 1, 0, GridBagConstraints.NONE, GridBagConstraints.LINE_START, new Insets(2,4,2,4));
    addGBComponent(authPanel,SecPanel,       0, 3, 1, 1, 2, 2, 1, 1, GridBagConstraints.NONE, GridBagConstraints.FIRST_LINE_START, new Insets(2,4,2,4));

    tabPane.add(FormatPanel);
    tabPane.add(InputsPanel);
    tabPane.add(MiscPanel);
    tabPane.add(DefaultsPanel);
    tabPane.add(SecPanel);
    tabPane.addTab("Colour & Encoding", FormatPanel);
    tabPane.addTab("Inputs", InputsPanel);
    tabPane.addTab("Misc", MiscPanel);
    tabPane.addTab("Load / Save", DefaultsPanel);
    tabPane.addTab("Security", SecPanel);
    tabPane.setBorder(BorderFactory.createEmptyBorder(4,4,0,4));

    okButton = new JButton("OK");
    okButton.setPreferredSize(new Dimension(90,30));
    okButton.addActionListener(this);
    cancelButton = new JButton("Cancel");
    cancelButton.setPreferredSize(new Dimension(90,30));
    cancelButton.addActionListener(this);

    JPanel buttonPane = new JPanel();
    buttonPane.setLayout(new BoxLayout(buttonPane, BoxLayout.LINE_AXIS));
    buttonPane.setBorder(BorderFactory.createEmptyBorder(4,0,0,0));
    buttonPane.add(Box.createHorizontalGlue());
    buttonPane.add(okButton);
    buttonPane.add(Box.createRigidArea(new Dimension(4,0)));
    buttonPane.add(cancelButton);
    buttonPane.add(Box.createRigidArea(new Dimension(4,0)));

    this.getContentPane().add(tabPane);
    this.getContentPane().add(buttonPane);

    pack();
	
  }

  public void initDialog() {
    if (cb != null) cb.setOptions();
    zrle.setEnabled(!autoSelect.isSelected());
    hextile.setEnabled(!autoSelect.isSelected());
    tight.setEnabled(!autoSelect.isSelected());
    raw.setEnabled(!autoSelect.isSelected());
    fullColour.setEnabled(!autoSelect.isSelected());
    mediumColour.setEnabled(!autoSelect.isSelected());
    lowColour.setEnabled(!autoSelect.isSelected());
    veryLowColour.setEnabled(!autoSelect.isSelected());
    compressLevel.setEnabled(customCompressLevel.isSelected());
    qualityLevel.setEnabled(noJpeg.isSelected());
    autoScale = (scalingFactor.getSelectedItem().equals("Auto"));
    fixedRatioScale = 
      (scalingFactor.getSelectedItem().equals("Fixed Aspect Ratio"));
    sendLocalUsername.setEnabled(secVeNCrypt.isEnabled()&&
      (secPlain.isSelected()||secIdent.isSelected()));
  }

  JRadioButton addRadioCheckbox(String str, ButtonGroup group, JPanel panel) {
    JRadioButton c = new JRadioButton(str);
    GridBagConstraints gbc = new GridBagConstraints();
    gbc.anchor = GridBagConstraints.LINE_START;
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.weightx = 1;
    gbc.weighty = 1;
    panel.add(c,gbc);
    group.add(c);
    c.addItemListener(this);
    return c;
  }

  JCheckBox addCheckbox(String str, ButtonGroup group, JPanel panel) {
    JCheckBox c = new JCheckBox(str);
    GridBagConstraints gbc = new GridBagConstraints();
    gbc.anchor = GridBagConstraints.LINE_START;
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.weightx = 1;
    gbc.weighty = 1;
    panel.add(c,gbc);
    if (group != null)
      group.add(c);
    c.addItemListener(this);
    return c;
  }

  JCheckBox addJCheckBox(String str, ButtonGroup group, JPanel panel,
      GridBagConstraints gbc) {
    JCheckBox c = new JCheckBox(str);
    panel.add(c,gbc);
    if (group != null)
      group.add(c);
    c.addItemListener(this);
    
    return c;
  }

  public void actionPerformed(ActionEvent e) {
    Object s = e.getSource();
    if (s instanceof JButton && (JButton)s == okButton) {
      autoScale = (scalingFactor.getSelectedItem().equals("Auto"));
      fixedRatioScale = 
        (scalingFactor.getSelectedItem().equals("Fixed Aspect Ratio"));
      ok = true;
      if (cb != null) cb.getOptions();
      endDialog();
    } else if (s instanceof JButton && (JButton)s == cancelButton) {
      ok = false;
      endDialog();
    } else if (s instanceof JButton && (JButton)s == defSaveButton) {
      try {
        defaults.Save();
      } catch (java.lang.Exception x) { }
    } else if (s instanceof JButton && (JButton)s == ca) {
      JFileChooser fc = new JFileChooser();
      fc.setDialogTitle("Path to X509 CA certificate");
      int ret = fc.showOpenDialog(this);
      if (ret == JFileChooser.APPROVE_OPTION)
        CSecurityTLS.x509ca.setParam(fc.getSelectedFile().toString());
    } else if (s instanceof JButton && (JButton)s == crl) {
      JFileChooser fc = new JFileChooser();
      fc.setDialogTitle("Path to X509 CRL file");
      int ret = fc.showOpenDialog(this);
      if (ret == JFileChooser.APPROVE_OPTION)
        CSecurityTLS.x509crl.setParam(fc.getSelectedFile().toString());
    }
  }

  public void itemStateChanged(ItemEvent e) {
    Object s = e.getSource();
    if (s instanceof JCheckBox && (JCheckBox)s == autoSelect) {
      zrle.setEnabled(!autoSelect.isSelected());
      hextile.setEnabled(!autoSelect.isSelected());
      tight.setEnabled(!autoSelect.isSelected());
      raw.setEnabled(!autoSelect.isSelected());
      fullColour.setEnabled(!autoSelect.isSelected());
      mediumColour.setEnabled(!autoSelect.isSelected());
      lowColour.setEnabled(!autoSelect.isSelected());
      veryLowColour.setEnabled(!autoSelect.isSelected());
      defaults.setPref("autoSelect",(autoSelect.isSelected()) ? "on" : "off");
    } 
    if (s instanceof JCheckBox && (JCheckBox)s == customCompressLevel) {
      compressLevel.setEnabled(customCompressLevel.isSelected());
      defaults.setPref("customCompressLevel",(customCompressLevel.isSelected()) ? "on" : "off");
    }
    if (s instanceof JCheckBox && (JCheckBox)s == noJpeg) {
      qualityLevel.setEnabled(noJpeg.isSelected());
      defaults.setPref("noJpeg",(noJpeg.isSelected()) ? "on" : "off");
    }
    if (s instanceof JCheckBox && (JCheckBox)s == sendLocalUsername) {
      defaults.setPref("sendLocalUsername",(sendLocalUsername.isSelected()) ? "on" : "off");
    }
    if (s instanceof JCheckBox && (JCheckBox)s == secVeNCrypt) {
      encNone.setEnabled(secVeNCrypt.isSelected());
      encTLS.setEnabled(secVeNCrypt.isSelected());
      encX509.setEnabled(secVeNCrypt.isSelected());
      ca.setEnabled(secVeNCrypt.isSelected());
      crl.setEnabled(secVeNCrypt.isSelected());
      secIdent.setEnabled(secVeNCrypt.isSelected());
      secNone.setEnabled(secVeNCrypt.isSelected());
      secVnc.setEnabled(secVeNCrypt.isSelected());
      secPlain.setEnabled(secVeNCrypt.isSelected());
      sendLocalUsername.setEnabled(secVeNCrypt.isSelected());
    }
    if (s instanceof JCheckBox && (JCheckBox)s == secIdent ||
        s instanceof JCheckBox && (JCheckBox)s == secPlain) {
      sendLocalUsername.setEnabled(secIdent.isSelected()||secPlain.isSelected());
    }
  }

}
