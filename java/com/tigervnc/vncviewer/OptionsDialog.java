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
import java.text.Format;
import java.text.NumberFormat;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.UIManager.*;
import javax.swing.text.*;
import java.util.*;
import java.util.Map.Entry;


import com.tigervnc.rfb.*;

import static java.awt.GridBagConstraints.BOTH;
import static java.awt.GridBagConstraints.CENTER;
import static java.awt.GridBagConstraints.HORIZONTAL;
import static java.awt.GridBagConstraints.LINE_END;
import static java.awt.GridBagConstraints.LINE_START;
import static java.awt.GridBagConstraints.PAGE_START;
import static java.awt.GridBagConstraints.NONE;
import static java.awt.GridBagConstraints.RELATIVE;
import static java.awt.GridBagConstraints.REMAINDER;
import static java.awt.GridBagConstraints.VERTICAL;

class OptionsDialog extends Dialog {

  private class IntegerDocument extends PlainDocument {
    private int limit;

    IntegerDocument(int limit) {
      super();
      this.limit = limit;
    }

    public void insertString(int offset, String  str, AttributeSet a)
          throws BadLocationException {
      if (str == null || !str.matches("^[0-9]+$")) return;
      if ((getLength() + str.length()) > limit)
        java.awt.Toolkit.getDefaultToolkit().beep();
      else
        super.insertString(offset, str, a);
    }
  }

  private class IntegerTextField extends JFormattedTextField {
    public IntegerTextField(int digits) {
      super();
      this.setDocument(new IntegerDocument(digits));
      Font f = getFont();
      String template = String.format("%0"+digits+"d", 0);
      int w = getFontMetrics(f).stringWidth(template) +
              getMargin().left + getMargin().right + 
              getInsets().left + getInsets().right;
      int h = getPreferredSize().height;
      setPreferredSize(new Dimension(w, h));
    }

    @Override
    protected void processFocusEvent(final FocusEvent e) {
      if (e.isTemporary())
        return;
      if (e.getID() == FocusEvent.FOCUS_LOST)
        if (getText() == null || getText().isEmpty())
          setValue(null);
      super.processFocusEvent(e);
    }
  }

  // Constants
  static LogWriter vlog = new LogWriter("OptionsDialog");

  CConn cc;
  @SuppressWarnings({"rawtypes"})
  JComboBox menuKey, compressLevel, qualityLevel, scalingFactor;
  ButtonGroup encodingGroup, colourGroup;
  JRadioButton zrle, hextile, tight, raw, fullColour, mediumColour,
               lowColour, veryLowColour;
  JCheckBox autoSelect, customCompressLevel, noJpeg, viewOnly,
            acceptClipboard, sendClipboard, acceptBell, desktopSize,
            fullScreen, fullScreenAllMonitors, shared, useLocalCursor,
            secVeNCrypt, encNone, encTLS, encX509, secNone, secVnc,
            secPlain, secIdent, sendLocalUsername;
  JButton okButton, cancelButton, caButton, crlButton, cfLoadButton,
          cfSaveAsButton, defSaveButton, defReloadButton, defClearButton;
  JTextField desktopWidth, desktopHeight, x509ca, x509crl;
  JTabbedPane tabPane;

  @SuppressWarnings({"rawtypes","unchecked"})
  public OptionsDialog(CConn cc_) {
    super(true);
    cc = cc_;
    setTitle("VNC Viewer Options");
    setResizable(false);

    getContentPane().setLayout(
      new BoxLayout(getContentPane(), BoxLayout.PAGE_AXIS));

    tabPane = new JTabbedPane();
    tabPane.setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);

    encodingGroup = new ButtonGroup();
    colourGroup = new ButtonGroup();
    int indent = 0;

    // Compression tab
    JPanel FormatPanel = new JPanel();
    FormatPanel.setLayout(new BoxLayout(FormatPanel,
                                        BoxLayout.PAGE_AXIS));
    FormatPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel autoSelectPane = new JPanel();
    autoSelectPane.setLayout(new BoxLayout(autoSelectPane,
                                           BoxLayout.LINE_AXIS));
    autoSelectPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 5, 0));
    autoSelect = new JCheckBox("Auto Select");
    autoSelectPane.add(autoSelect);
    autoSelectPane.add(Box.createHorizontalGlue());

    JPanel encodingPanel = new JPanel(new GridLayout(4, 1));
    encodingPanel.
      setBorder(BorderFactory.createTitledBorder("Preferred encoding"));
    tight = new GroupedJRadioButton("Tight",
                                    encodingGroup, encodingPanel);
    zrle = new GroupedJRadioButton("ZRLE",
                                    encodingGroup, encodingPanel);
    hextile = new GroupedJRadioButton("Hextile",
                                      encodingGroup, encodingPanel);
    raw = new GroupedJRadioButton("Raw", encodingGroup, encodingPanel);

    JPanel colourPanel = new JPanel(new GridLayout(4, 1));
    colourPanel.setBorder(BorderFactory.createTitledBorder("Color level"));
    fullColour = new GroupedJRadioButton("Full (all available colors)",
                                          colourGroup, colourPanel);
    mediumColour = new GroupedJRadioButton("Medium (256 colors)",
                                            colourGroup, colourPanel);
    lowColour = new GroupedJRadioButton("Low (64 colours)",
                                        colourGroup, colourPanel);
    veryLowColour = new GroupedJRadioButton("Very low(8 colors)",
                                            colourGroup, colourPanel);

    JPanel encodingPane = new JPanel(new GridLayout(1, 2, 5, 0));
    encodingPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 5, 0));
    encodingPane.add(encodingPanel);
    encodingPane.add(colourPanel);

    JPanel tightPanel = new JPanel(new GridBagLayout());
    customCompressLevel = new JCheckBox("Custom Compression Level");
    Object[] compressionLevels = { 1, 2, 3, 4, 5, 6 };
    compressLevel  = new MyJComboBox(compressionLevels);
    ((MyJComboBox)compressLevel).setDocument(new IntegerDocument(1));
    compressLevel.setPrototypeDisplayValue("0.");
    compressLevel.setEditable(true);
    JLabel compressionLabel =
      new JLabel("Level (1=fast, 6=best [4-6 are rarely useful])");
    noJpeg = new JCheckBox("Allow JPEG Compression");
    Object[] qualityLevels = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    qualityLevel  = new MyJComboBox(qualityLevels);
    qualityLevel.setPrototypeDisplayValue("0.");
    JLabel qualityLabel = new JLabel("Quality (0=poor, 9=best)");

    tightPanel.add(customCompressLevel,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));
    indent = getButtonLabelInset(customCompressLevel);
    tightPanel.add(compressLevel,
                   new GridBagConstraints(0, 1,
                                          1, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, indent, 0, 0),
                                          NONE, NONE));
    tightPanel.add(compressionLabel,
                   new GridBagConstraints(1, 1,
                                          1, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, HORIZONTAL,
                                          new Insets(0, 5, 0, 0),
                                          NONE, NONE));
    tightPanel.add(noJpeg,
                   new GridBagConstraints(0, 2,
                                          REMAINDER, 1,
                                          LIGHT, LIGHT, 
                                          LINE_START, NONE,
                                          new Insets(5, 0, 0, 0),
                                          NONE, NONE));
    indent = getButtonLabelInset(noJpeg);
    tightPanel.add(qualityLevel,
                   new GridBagConstraints(0, 3,
                                          1, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, indent, 0, 0),
                                          NONE, NONE));
    tightPanel.add(qualityLabel,
                   new GridBagConstraints(1, 3,
                                          1, 1,
                                          HEAVY, NONE,
                                          LINE_START, HORIZONTAL,
                                          new Insets(0, 5, 0, 0),
                                          NONE, NONE));
    tightPanel.add(Box.createRigidArea(new Dimension(5,0)),
                   new GridBagConstraints(0, 4,
                                          REMAINDER, REMAINDER,
                                          HEAVY, HEAVY,
                                          LINE_START, BOTH,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));
    FormatPanel.add(autoSelectPane);
    FormatPanel.add(encodingPane);
    FormatPanel.add(tightPanel);

    // security tab
    JPanel SecPanel = new JPanel();
    SecPanel.setLayout(new BoxLayout(SecPanel,
                                     BoxLayout.PAGE_AXIS));
    SecPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel vencryptPane = new JPanel();
    vencryptPane.setLayout(new BoxLayout(vencryptPane,
                                         BoxLayout.LINE_AXIS));
    vencryptPane.setBorder(BorderFactory.createEmptyBorder(0,0,5,0));
    secVeNCrypt = new JCheckBox("Extended encryption and "+
                                "authentication methods (VeNCrypt)");
    vencryptPane.add(secVeNCrypt);
    vencryptPane.add(Box.createHorizontalGlue());

    JPanel encrPanel = new JPanel(new GridBagLayout());
    encrPanel.setBorder(BorderFactory.createTitledBorder("Encryption"));
    encNone = new JCheckBox("None");
    encTLS = new JCheckBox("Anonymous TLS");
    encX509 = new JCheckBox("TLS with X.509 certificates");
    JLabel caLabel = new JLabel("X.509 CA Certificate");
    x509ca = new JTextField();
    x509ca.setName(Configuration.getParam("x509ca").getName());
    caButton = new JButton("Browse");
    JLabel crlLabel = new JLabel("X.509 CRL file");
    x509crl = new JTextField();
    x509crl.setName(Configuration.getParam("x509crl").getName());
    crlButton = new JButton("Browse");
    encrPanel.add(encNone,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    encrPanel.add(encTLS,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    encrPanel.add(encX509,
                  new GridBagConstraints(0, 2,
                                         3, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 0, 0),
                                         NONE, NONE));
    indent = getButtonLabelInset(encX509);
    encrPanel.add(caLabel,
                  new GridBagConstraints(0, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_END, NONE,
                                         new Insets(0, indent, 5, 0),
                                         0, 0));
    encrPanel.add(x509ca,
                  new GridBagConstraints(1, 3,
                                         1, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 5, 5, 0),
                                         0, 0));
    encrPanel.add(caButton,
                  new GridBagConstraints(2, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, VERTICAL,
                                         new Insets(0, 5, 5, 0),
                                         0, 0));
    encrPanel.add(crlLabel,
                  new GridBagConstraints(0, 4,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_END, NONE,
                                         new Insets(0, indent, 0, 0),
                                         0, 0));
    encrPanel.add(x509crl,
                  new GridBagConstraints(1, 4,
                                         1, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 5, 0, 0),
                                         0, 0));
    encrPanel.add(crlButton,
                  new GridBagConstraints(2, 4,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, VERTICAL,
                                         new Insets(0, 5, 0, 0),
                                         0, 0));

    JPanel authPanel = new JPanel(new GridBagLayout());
    authPanel.setBorder(BorderFactory.createTitledBorder("Authentication"));

    secNone = new JCheckBox("None");
    secVnc = new JCheckBox("Standard VNC");
    secPlain = new JCheckBox("Plaintext");
    secIdent = new JCheckBox("Ident");
    sendLocalUsername = new JCheckBox("Send Local Username");
    authPanel.add(secNone,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    authPanel.add(secVnc,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    authPanel.add(secPlain,
                  new GridBagConstraints(0, 2,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(secIdent,
                  new GridBagConstraints(0, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 0, 0),
                                         NONE, NONE));
    authPanel.add(sendLocalUsername,
                  new GridBagConstraints(1, 2,
                                         1, 2,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 20, 2, 0),
                                         NONE, NONE));

    SecPanel.add(vencryptPane,
                 new GridBagConstraints(0, 0,
                                        REMAINDER, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    SecPanel.add(encrPanel,
                 new GridBagConstraints(0, 1,
                                        REMAINDER, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    SecPanel.add(authPanel,
                 new GridBagConstraints(0, 2,
                                        REMAINDER, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    SecPanel.add(Box.createRigidArea(new Dimension(0,0)),
                 new GridBagConstraints(0, RELATIVE,
                                        REMAINDER, REMAINDER,
                                        HEAVY, HEAVY,
                                        LINE_START, BOTH,
                                        new Insets(0, 0, 0, 0),
                                        NONE, NONE));

    // Input tab
    JPanel inputPanel = new JPanel(new GridBagLayout());
    inputPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    viewOnly = new JCheckBox("View Only (ignore mouse & keyboard)");
    acceptClipboard = new JCheckBox("Accept clipboard from server");
    sendClipboard = new JCheckBox("Send clipboard to server");
    JLabel menuKeyLabel = new JLabel("Menu Key");
    String[] menuKeys = new String[MenuKey.getMenuKeySymbolCount()];
    for (int i = 0; i < MenuKey.getMenuKeySymbolCount(); i++)
      menuKeys[i] = KeyEvent.getKeyText(MenuKey.getMenuKeySymbols()[i].keycode);
    menuKey  = new JComboBox(menuKeys);

    inputPanel.add(viewOnly,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(acceptClipboard,
                   new GridBagConstraints(0, 1,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(sendClipboard,
                   new GridBagConstraints(0, 2,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(menuKeyLabel,
                   new GridBagConstraints(0, 3,
                                          1, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));
    inputPanel.add(menuKey,
                   new GridBagConstraints(1, 3,
                                          1, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 5, 0, 0),
                                          NONE, NONE));
    inputPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                   new GridBagConstraints(0, 4,
                                          REMAINDER, REMAINDER,
                                          HEAVY, HEAVY,
                                          LINE_START, BOTH,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));

    // Screen tab
    JPanel ScreenPanel = new JPanel(new GridBagLayout());
    ScreenPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));
    desktopSize = new JCheckBox("Resize remote session on connect");
    desktopSize.setEnabled(!cc.viewer.embed.getValue() &&
                           (cc.viewer.desktopSize.getValue() != null));
    desktopWidth = new IntegerTextField(5);
    desktopWidth.setEnabled(desktopSize.isSelected());
    desktopHeight = new IntegerTextField(5);
    desktopHeight.setEnabled(desktopSize.isSelected());
    JPanel desktopSizePanel =
      new JPanel(new FlowLayout(FlowLayout.LEADING, 0, 0));
    desktopSizePanel.add(desktopWidth);
    desktopSizePanel.add(new JLabel(" x "));
    desktopSizePanel.add(desktopHeight);
    fullScreen = new JCheckBox("Full-screen mode");
    fullScreen.setEnabled(!cc.viewer.embed.getValue());
    fullScreenAllMonitors =
      new JCheckBox("Enable full-screen mode over all monitors");
    fullScreenAllMonitors.setEnabled(!cc.viewer.embed.getValue());
    JLabel scalingFactorLabel = new JLabel("Scaling Factor");
    Object[] scalingFactors = {
      "Auto", "Fixed Aspect Ratio", "50%", "75%", "95%", "100%", "105%",
      "125%", "150%", "175%", "200%", "250%", "300%", "350%", "400%" };
    scalingFactor = new MyJComboBox(scalingFactors);
    scalingFactor.setEditable(true);
    scalingFactor.setEnabled(!cc.viewer.embed.getValue());
    ScreenPanel.add(desktopSize,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(desktopSize);
    ScreenPanel.add(desktopSizePanel,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, indent, 0, 0),
                                           NONE, NONE));
    ScreenPanel.add(fullScreen,
                    new GridBagConstraints(0, 2,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(fullScreen);
    ScreenPanel.add(fullScreenAllMonitors,
                    new GridBagConstraints(0, 3,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, indent, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(scalingFactorLabel,
                    new GridBagConstraints(0, 4,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(scalingFactor,
                    new GridBagConstraints(1, 4,
                                           1, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 5, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                    new GridBagConstraints(0, 5,
                                           REMAINDER, REMAINDER,
                                           HEAVY, HEAVY,
                                           LINE_START, BOTH,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));

    // Misc tab
    JPanel MiscPanel = new JPanel(new GridBagLayout());
    MiscPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));
    shared =
      new JCheckBox("Shared connection (do not disconnect other viewers)");
    useLocalCursor = new JCheckBox("Render cursor locally");
    acceptBell = new JCheckBox("Beep when requested by the server");
    MiscPanel.add(shared,
                  new GridBagConstraints(0, 0,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(useLocalCursor,
                  new GridBagConstraints(0, 1,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(acceptBell,
                  new GridBagConstraints(0, 2,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                  new GridBagConstraints(0, 3,
                                         REMAINDER, REMAINDER,
                                         HEAVY, HEAVY,
                                         LINE_START, BOTH,
                                         new Insets(0, 0, 0, 0),
                                         NONE, NONE));


    // load/save tab
    JPanel loadSavePanel = new JPanel(new GridBagLayout());
    loadSavePanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));
    JPanel configPanel = new JPanel(new GridBagLayout());
    configPanel.
      setBorder(BorderFactory.createTitledBorder("Configuration File"));
    cfLoadButton = new JButton("Load");
    cfSaveAsButton = new JButton("Save As...");
    configPanel.add(cfLoadButton,
                    new GridBagConstraints(0, 0,
                                           1, 1,
                                           HEAVY, LIGHT,
                                           CENTER, HORIZONTAL,
                                           new Insets(0, 0, 5, 0),
                                           NONE, NONE));
    configPanel.add(cfSaveAsButton,
                    new GridBagConstraints(0, 1,
                                           1, 1,
                                           HEAVY, HEAVY,
                                           CENTER, HORIZONTAL,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));

    JPanel defaultsPanel = new JPanel(new GridBagLayout());
    defaultsPanel.setBorder(BorderFactory.createTitledBorder("Defaults"));
    defClearButton = new JButton("Clear");
    defReloadButton = new JButton("Reload");
    defSaveButton = new JButton("Save");
    defaultsPanel.add(defClearButton,
                      new GridBagConstraints(0, 0,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             CENTER, HORIZONTAL,
                                             new Insets(0, 0, 5, 0),
                                             NONE, NONE));
    defaultsPanel.add(defReloadButton,
                      new GridBagConstraints(0, 1,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             CENTER, HORIZONTAL,
                                             new Insets(0, 0, 5, 0),
                                             NONE, NONE));
    defaultsPanel.add(defSaveButton,
                      new GridBagConstraints(0, 2,
                                             1, 1,
                                             HEAVY, HEAVY,
                                             CENTER, HORIZONTAL,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));

    loadSavePanel.add(configPanel,
                      new GridBagConstraints(0, 0,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             PAGE_START, HORIZONTAL,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));
    loadSavePanel.add(Box.createRigidArea(new Dimension(5, 0)),
                      new GridBagConstraints(1, 1,
                                             1, 1,
                                             LIGHT, LIGHT,
                                             LINE_START, NONE,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));
    loadSavePanel.add(defaultsPanel,
                      new GridBagConstraints(2, 0,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             PAGE_START, HORIZONTAL,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));
    loadSavePanel.add(Box.createRigidArea(new Dimension(5, 0)),
                      new GridBagConstraints(0, 1,
                                             REMAINDER, REMAINDER,
                                             HEAVY, HEAVY,
                                             LINE_START, BOTH,
                                             new Insets(0, 0, 0, 0),
                                             NONE, NONE));

    // tabPane
    tabPane.addTab("Compression", FormatPanel);
    tabPane.addTab("Security", SecPanel);
    tabPane.addTab("Input", inputPanel);
    tabPane.addTab("Screen", ScreenPanel);
    tabPane.addTab("Misc", MiscPanel);
    tabPane.addTab("Load / Save", loadSavePanel);
    tabPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 0, 0));
    // Resize the tabPane if necessary to prevent scrolling
    Insets tpi =
      (Insets)UIManager.get("TabbedPane:TabbedPaneTabArea.contentMargins");
    int minWidth = tpi.left + tpi.right;
    for (int i = 0; i < tabPane.getTabCount(); i++)
      minWidth += tabPane.getBoundsAt(i).width;
    int minHeight = tabPane.getPreferredSize().height;
    if (tabPane.getPreferredSize().width < minWidth)
      tabPane.setPreferredSize(new Dimension(minWidth, minHeight));

    // button pane
    okButton = new JButton("OK");
    cancelButton = new JButton("Cancel");

    JPanel buttonPane = new JPanel(new GridLayout(1, 5, 10, 10));
    buttonPane.setBorder(BorderFactory.createEmptyBorder(10, 5, 5, 5));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(okButton);
    buttonPane.add(cancelButton);

    this.add(tabPane);
    this.add(buttonPane);
    addListeners(this);
    pack();
  }

  public void initDialog() {
    if (cc != null) cc.setOptions();
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
    sendLocalUsername.setEnabled(secVeNCrypt.isEnabled()&&
      (secPlain.isSelected()||secIdent.isSelected()));
  }

  private void updatePreferences() {
    if (autoSelect.isSelected()) {
      UserPreferences.set("global", "AutoSelect", true);
    } else {
      UserPreferences.set("global", "AutoSelect", false);
      if (zrle.isSelected()) {
        UserPreferences.set("global", "PreferredEncoding", "ZRLE");
      } else if (hextile.isSelected()) {
        UserPreferences.set("global", "PreferredEncoding", "hextile");
      } else if (tight.isSelected()) {
        UserPreferences.set("global", "PreferredEncoding", "Tight");
      } else if (raw.isSelected()) {
        UserPreferences.set("global", "PreferredEncoding", "raw");
      }
    }
    if (fullColour.isSelected()) {
      UserPreferences.set("global", "FullColour", true);
    } else {
      UserPreferences.set("global", "FullColour", false);
      if (mediumColour.isSelected()) {
        UserPreferences.set("global", "LowColorLevel", 2);
      } else if (lowColour.isSelected()) {
        UserPreferences.set("global", "LowColorLevel", 1);
      } else if (veryLowColour.isSelected()) {
        UserPreferences.set("global", "LowColorLevel", 0);
      }
    }
    UserPreferences.set("global", "NoJPEG", !noJpeg.isSelected());
    UserPreferences.set("global",
            "QualityLevel", (Integer)qualityLevel.getSelectedItem());
    UserPreferences.set("global",
            "CustomCompressLevel", customCompressLevel.isSelected());
    UserPreferences.set("global",
            "CompressLevel", (Integer)compressLevel.getSelectedItem());
    UserPreferences.set("global", "ViewOnly", viewOnly.isSelected());
    UserPreferences.set("global",
            "AcceptClipboard", acceptClipboard.isSelected());
    UserPreferences.set("global", "SendClipboard", sendClipboard.isSelected());
    String menuKeyStr =
      MenuKey.getMenuKeySymbols()[menuKey.getSelectedIndex()].name;
    UserPreferences.set("global", "MenuKey", menuKeyStr);
    String desktopSizeString =
      desktopSize.isSelected() ?
        desktopWidth.getText() + "x" + desktopHeight.getText() : "";
    UserPreferences.set("global", "DesktopSize", desktopSizeString);
    UserPreferences.set("global", "FullScreen", fullScreen.isSelected());
    UserPreferences.set("global",
            "FullScreenAllMonitors", fullScreenAllMonitors.isSelected());
    UserPreferences.set("global", "Shared", shared.isSelected());
    UserPreferences.set("global",
            "UseLocalCursor", useLocalCursor.isSelected());
    UserPreferences.set("global", "AcceptBell", acceptBell.isSelected());
    String scaleString = scalingFactor.getSelectedItem().toString();
    if (scaleString.equalsIgnoreCase("Auto")) {
      UserPreferences.set("global", "ScalingFactor", "Auto");
    } else if(scaleString.equalsIgnoreCase("Fixed Aspect Ratio")) {
      UserPreferences.set("global", "ScalingFactor", "FixedRatio");
    } else {
      scaleString=scaleString.substring(0, scaleString.length()-1);
      UserPreferences.set("global", "ScalingFactor", scaleString);
    }
    UserPreferences.set("viewer", "secVeNCrypt", secVeNCrypt.isSelected());
    UserPreferences.set("viewer", "encNone", encNone.isSelected());
    UserPreferences.set("viewer", "encTLS", encTLS.isSelected());
    UserPreferences.set("viewer", "encX509", encX509.isSelected());
    UserPreferences.set("viewer", "secNone", secNone.isSelected());
    UserPreferences.set("viewer", "secVnc", secVnc.isSelected());
    UserPreferences.set("viewer", "secPlain", secPlain.isSelected());
    UserPreferences.set("viewer", "secIdent", secIdent.isSelected());
    UserPreferences.set("global",
            "SendLocalUsername", sendLocalUsername.isSelected());
    if (!CSecurityTLS.x509ca.getValueStr().equals(""))
      UserPreferences.set("viewer", "x509ca",
              CSecurityTLS.x509ca.getValueStr());
    if (!CSecurityTLS.x509crl.getValueStr().equals(""))
      UserPreferences.set("viewer", "x509crl",
              CSecurityTLS.x509crl.getValueStr());
  }

  private void restorePreferences() {
    autoSelect.setSelected(UserPreferences.getBool("global", "AutoSelect"));
    if (!autoSelect.isSelected()) {
      if (UserPreferences.getBool("global", "FullColour")) {
        fullColour.setSelected(true);
      } else {
        switch (UserPreferences.getInt("global", "LowColorLevel")) {
        case 2:
          mediumColour.setSelected(true);
          break;
        case 1:
          lowColour.setSelected(true);
          break;
        case 0:
          veryLowColour.setSelected(true);
          break;
        }
      }
      String encoding = UserPreferences.get("global", "PreferredEncoding");
      if (encoding != null) {
        switch (Encodings.encodingNum(encoding)) {
        case Encodings.encodingZRLE:
          zrle.setSelected(true);
          break;
        case Encodings.encodingHextile:
          hextile.setSelected(true);
          break;
        case Encodings.encodingRaw:
          raw.setSelected(true);
          break;
        default:
          tight.setSelected(true);
        }
      }
    }
    noJpeg.setSelected(!UserPreferences.getBool("global", "NoJPEG"));
    qualityLevel.setSelectedItem(UserPreferences.getInt("global",
            "QualityLevel"));
    customCompressLevel.setSelected(UserPreferences.getBool("global",
            "CustomCompressLevel"));
    compressLevel.setSelectedItem(UserPreferences.getInt("global",
            "CompressLevel"));
    viewOnly.setSelected(UserPreferences.getBool("global", "ViewOnly"));
    acceptClipboard.setSelected(UserPreferences.getBool("global",
            "AcceptClipboard"));
    sendClipboard.setSelected(UserPreferences.getBool("global",
            "SendClipboard"));
    menuKey.setSelectedItem(UserPreferences.get("global", "MenuKey"));
    desktopSize.setSelected(UserPreferences.get("global", "DesktopSize")
            != null);
    if (desktopSize.isSelected()) {
      String desktopSizeString = UserPreferences.get("global", "DesktopSize");
      desktopWidth.setText(desktopSizeString.split("x")[0]);
      desktopHeight.setText(desktopSizeString.split("x")[1]);
    }
    fullScreen.setSelected(UserPreferences.getBool("global", "FullScreen"));
    fullScreenAllMonitors.setSelected(UserPreferences.getBool("global",
            "FullScreenAllMonitors"));
    if (shared.isEnabled())
      shared.setSelected(UserPreferences.getBool("global", "Shared"));
    useLocalCursor.setSelected(UserPreferences.getBool("global",
            "UseLocalCursor"));
    acceptBell.setSelected(UserPreferences.getBool("global", "AcceptBell"));
    String scaleString = UserPreferences.get("global", "ScalingFactor");
    if (scaleString != null) {
      if (scaleString.equalsIgnoreCase("Auto")) {
        scalingFactor.setSelectedItem("Auto");
      } else if (scaleString.equalsIgnoreCase("FixedRatio")) {
        scalingFactor.setSelectedItem("Fixed Aspect Ratio");
      } else {
        scalingFactor.setSelectedItem(scaleString+"%");
      }
    }
    if (secVeNCrypt.isEnabled()) {
      secVeNCrypt.setSelected(UserPreferences.getBool("viewer",
              "secVeNCrypt", true));
      if (secVeNCrypt.isSelected()) {
        encNone.setSelected(UserPreferences.getBool("viewer", "encNone", true));
        encTLS.setSelected(UserPreferences.getBool("viewer", "encTLS", true));
        encX509.setSelected(UserPreferences.getBool("viewer", "encX509", true));
        secPlain.setSelected(UserPreferences.getBool("viewer", "secPlain", true));
        secIdent.setSelected(UserPreferences.getBool("viewer", "secIdent", true));
        sendLocalUsername.setSelected(UserPreferences.getBool("global",
                "SendLocalUsername"));
      }
    }
    if (secNone.isEnabled())
      secNone.setSelected(UserPreferences.getBool("viewer", "secNone", true));
    if (secVnc.isEnabled())
      secVnc.setSelected(UserPreferences.getBool("viewer", "secVnc", true));
  }

  public void endDialog() {
    super.endDialog();
    if (cc.viewport != null && cc.viewport.isVisible()) {
      cc.viewport.toFront();
      cc.viewport.requestFocus();
    }
  }

  public void actionPerformed(ActionEvent e) {
    Object s = e.getSource();
    if (s instanceof JButton) {
      JButton button = (JButton)s;
      if (button == okButton) {
        JTextField[] fields =
          { x509ca, x509crl };
        for (JTextField field : fields) {
          if (field.getText() != null && !field.getText().equals("")) {
            File f = new File(field.getText());
            if (!f.exists() || !f.canRead()) {
              String msg = new String("The file "+f.getAbsolutePath()+
                           " specified for option "+field.getName()+
                           " does not exist or cannot be read.  Please "+
                           "correct before proceeding.");
              JOptionPane.showMessageDialog(this, msg, "WARNING",
                                            JOptionPane.WARNING_MESSAGE);
              return;
            }
          }
        }
        if (cc != null) cc.getOptions();
        endDialog();
      } else if (button == cancelButton) {
        endDialog();
      } else if (button == cfLoadButton) {
        JFileChooser fc = new JFileChooser();
        fc.setDialogTitle("Path to configuration file");
        fc.setApproveButtonText("OK");
        fc.setFileHidingEnabled(false);
        int ret = fc.showOpenDialog(this);
        if (ret == JFileChooser.APPROVE_OPTION) {
          String filename = fc.getSelectedFile().toString();
          if (filename != null)
            Configuration.load(filename);
          cc.setOptions();
        }
      } else if (button == cfSaveAsButton) {
        JFileChooser fc = new JFileChooser();
        fc.setDialogTitle("Save current configuration as:");
        fc.setApproveButtonText("OK");
        fc.setFileHidingEnabled(false);
        int ret = fc.showOpenDialog(this);
        if (ret == JFileChooser.APPROVE_OPTION) {
          String filename = fc.getSelectedFile().toString();
          if (filename != null)
            Configuration.save(filename);
        }
      } else if (button == defSaveButton) {
        updatePreferences();
        UserPreferences.save();
      } else if (button == defReloadButton) {
        restorePreferences();
      } else if (button == defClearButton) {
        UserPreferences.clear();
        cc.setOptions();
      } else if (button == caButton) {
        JFileChooser fc =
          new JFileChooser(new File(CSecurityTLS.getDefaultCA()));
        fc.setDialogTitle("Path to X509 CA certificate");
        fc.setApproveButtonText("OK");
        fc.setFileHidingEnabled(false);
        int ret = fc.showOpenDialog(this);
        if (ret == JFileChooser.APPROVE_OPTION)
          x509ca.setText(fc.getSelectedFile().toString());
      } else if (button == crlButton) {
        JFileChooser fc =
          new JFileChooser(new File(CSecurityTLS.getDefaultCRL()));
        fc.setDialogTitle("Path to X509 CRL file");
        fc.setApproveButtonText("OK");
        fc.setFileHidingEnabled(false);
        int ret = fc.showOpenDialog(this);
        if (ret == JFileChooser.APPROVE_OPTION)
          x509crl.setText(fc.getSelectedFile().toString());
      }
    }
  }

  public void itemStateChanged(ItemEvent e) {
    Object s = e.getSource();
    if (s instanceof JCheckBox) {
      JCheckBox item = (JCheckBox)s;
      boolean enable = item.isSelected();
      if (item == autoSelect) {
        ButtonGroup[] groups = { encodingGroup, colourGroup };
        for (ButtonGroup grp : groups) {
          Enumeration<AbstractButton> elems = grp.getElements();
          while (elems.hasMoreElements())
            elems.nextElement().setEnabled(!enable);
        }
      } else if (item == customCompressLevel) {
        compressLevel.setEnabled(enable);
      } else if (item == desktopSize) {
        desktopWidth.setEnabled(enable);
        desktopHeight.setEnabled(enable);
      } else if (item == noJpeg) {
        qualityLevel.setEnabled(enable);
      } else if (item == encX509) {
        x509ca.setEnabled(enable);
        x509crl.setEnabled(enable);
        caButton.setEnabled(enable);
        crlButton.setEnabled(enable);
      } else if (item == secVeNCrypt) {
        encNone.setEnabled(enable);
        encTLS.setEnabled(enable);
        encX509.setEnabled(enable);
        x509ca.setEnabled(enable && encX509.isSelected());
        x509crl.setEnabled(enable && encX509.isSelected());
        caButton.setEnabled(enable && encX509.isSelected());
        crlButton.setEnabled(enable && encX509.isSelected());
        secIdent.setEnabled(enable);
        secPlain.setEnabled(enable);
        sendLocalUsername.setEnabled(enable);
      } else if (item == encNone) {
        secNone.setSelected(enable &&
          UserPreferences.getBool("viewer", "secNone", true));
        secVnc.setSelected(enable &&
          UserPreferences.getBool("viewer", "secVnc", true));
      } else if (item == secIdent || item == secPlain) {
        sendLocalUsername.setEnabled(secIdent.isSelected() ||
                                     secPlain.isSelected());
      }
    }
  }
}
