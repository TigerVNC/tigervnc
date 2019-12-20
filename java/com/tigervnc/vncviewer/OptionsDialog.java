/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2019 Brian P. Hinz
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
import java.lang.reflect.*;
import java.text.Format;
import java.text.NumberFormat;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.filechooser.*;
import javax.swing.UIManager.*;
import javax.swing.text.*;
import java.util.*;
import java.util.List;
import java.util.Map.Entry;
import java.util.prefs.*;

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

import static com.tigervnc.vncviewer.Parameters.*;

class OptionsDialog extends Dialog {

  private class IntegerDocument extends PlainDocument {
    private int limit;

    public IntegerDocument(int max) {
      super();
      limit = max;
    }

    public void insertString(int offset, String str, AttributeSet a)
          throws BadLocationException {
      if (str == null || !str.matches("^[0-9]+$")) return;
      if ((getLength() + str.length()) > limit)
        Toolkit.getDefaultToolkit().beep();
      else
        super.insertString(offset, str, a);
    }
  }

  private class IntegerTextField extends JFormattedTextField {
    public IntegerTextField(int digits) {
      super();
      setDocument(new IntegerDocument(digits));
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

  private static Map<Object, String> callbacks = new HashMap<Object, String>();
  /* Compression */
  JCheckBox autoselectCheckbox;

  ButtonGroup encodingGroup;
  JRadioButton tightButton;
  JRadioButton zrleButton;
  JRadioButton hextileButton;
  JRadioButton rawButton;

  ButtonGroup colorlevelGroup;
  JRadioButton fullcolorButton;
  JRadioButton mediumcolorButton;
  JRadioButton lowcolorButton;
  JRadioButton verylowcolorButton;

  JCheckBox compressionCheckbox;
  JCheckBox jpegCheckbox;
  JComboBox compressionInput;
  JComboBox jpegInput;

  /* Security */
  JCheckBox encNoneCheckbox;
  JCheckBox encTLSCheckbox;
  JCheckBox encX509Checkbox;
  JTextField caInput;
  JTextField crlInput;
  JButton caChooser;
  JButton crlChooser;

  JCheckBox authNoneCheckbox;
  JCheckBox authVncCheckbox;
  JCheckBox authPlainCheckbox;
  JCheckBox authIdentCheckbox;
  JCheckBox sendLocalUsernameCheckbox;

  /* Input */
  JCheckBox viewOnlyCheckbox;
  JCheckBox acceptClipboardCheckbox;
  JCheckBox sendClipboardCheckbox;
  JComboBox menuKeyChoice;

  /* Screen */
  JCheckBox desktopSizeCheckbox;
  JTextField desktopWidthInput;
  JTextField desktopHeightInput;

  ButtonGroup sizingGroup;
  JRadioButton remoteResizeButton;
  JRadioButton remoteScaleButton;
  JComboBox scalingFactorInput;

  JCheckBox fullScreenCheckbox;
  JCheckBox fullScreenAllMonitorsCheckbox;

  /* Misc. */
  JCheckBox sharedCheckbox;
  JCheckBox dotWhenNoCursorCheckbox;
  JCheckBox acceptBellCheckbox;

  /* SSH */
  JCheckBox tunnelCheckbox;
  JCheckBox viaCheckbox;
  JTextField viaUserInput;
  JTextField viaHostInput;
  JTextField viaPortInput;
  JCheckBox extSSHCheckbox;
  JTextField sshClientInput;
  JButton sshClientChooser;
  JRadioButton sshArgsDefaultButton;
  JRadioButton sshArgsCustomButton;
  JTextField sshArgsInput;
  JTextField sshConfigInput;
  JTextField sshKeyFileInput;
  JButton sshConfigChooser;
  JButton sshKeyFileChooser;

  @SuppressWarnings({"rawtypes","unchecked"})
  public OptionsDialog() {
    super(true);
    setTitle("VNC Viewer Options");
    setResizable(false);

    getContentPane().setLayout(
      new BoxLayout(getContentPane(), BoxLayout.PAGE_AXIS));

    JTabbedPane tabPane = new JTabbedPane();
    tabPane.setTabLayoutPolicy(JTabbedPane.SCROLL_TAB_LAYOUT);

    encodingGroup = new ButtonGroup();
    colorlevelGroup = new ButtonGroup();

    // tabPane
    tabPane.addTab("Compression", createCompressionPanel());
    tabPane.addTab("Security", createSecurityPanel());
    tabPane.addTab("Input", createInputPanel());
    tabPane.addTab("Screen", createScreenPanel());
    tabPane.addTab("Misc", createMiscPanel());
    tabPane.addTab("SSH", createSshPanel());
    tabPane.setBorder(BorderFactory.createEmptyBorder());
    // Resize the tabPane if necessary to prevent scrolling
    int minWidth = 0;
    Object tpi = UIManager.get("TabbedPane:TabbedPaneTabArea.contentMargins");
    if (tpi != null)
      minWidth += ((Insets)tpi).left + ((Insets)tpi).right;
    for (int i = 0; i < tabPane.getTabCount(); i++)
      minWidth += tabPane.getBoundsAt(i).width;
    int minHeight = tabPane.getPreferredSize().height;
    if (tabPane.getPreferredSize().width < minWidth)
      tabPane.setPreferredSize(new Dimension(minWidth, minHeight));

    // button pane
    JButton okButton = new JButton("OK  \u21B5");
    okButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        storeOptions();
        endDialog();
      }
    });
    JButton cancelButton = new JButton("Cancel");
    cancelButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        endDialog();
      }
    });

    JPanel buttonPane = new JPanel(new GridLayout(1, 5, 10, 10));
    buttonPane.setBorder(BorderFactory.createEmptyBorder(10, 5, 5, 5));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(Box.createRigidArea(new Dimension()));
    buttonPane.add(cancelButton);
    buttonPane.add(okButton);

    this.add(tabPane);
    this.add(buttonPane);
    addListeners(this);
    pack();
  }

  public static void showDialog(Container c) {
    OptionsDialog dialog = new OptionsDialog();
    dialog.show(c);
  }

  public void show(Container c) {
    loadOptions();
    super.showDialog(c);
  }

  public static void addCallback(String cb, Object obj)
  {
    callbacks.put(obj, cb);
  }

  public static void removeCallback(Object obj)
  {
    callbacks.remove(obj);
  }

  public void endDialog() {
    super.endDialog();
    // Making a new dialog is so cheap that it's not worth keeping
    this.dispose();
  }

  private void loadOptions()
  {
    /* Compression */
    autoselectCheckbox.setSelected(autoSelect.getValue());

    int encNum = Encodings.encodingNum(preferredEncoding.getValueStr());

    switch (encNum) {
    case Encodings.encodingTight:
      tightButton.setSelected(true);
      break;
    case Encodings.encodingZRLE:
      zrleButton.setSelected(true);
      break;
    case Encodings.encodingHextile:
      hextileButton.setSelected(true);
      break;
    case Encodings.encodingRaw:
      rawButton.setSelected(true);
      break;
    }

    if (fullColor.getValue())
      fullcolorButton.setSelected(true);
    else {
      switch (lowColorLevel.getValue()) {
      case 0:
        verylowcolorButton.setSelected(true);
        break;
      case 1:
        lowcolorButton.setSelected(true);
        break;
      case 2:
        mediumcolorButton.setSelected(true);
        break;
      }
    }

    int digit = 0;

    compressionCheckbox.setSelected(customCompressLevel.getValue());
    jpegCheckbox.setSelected(!noJpeg.getValue());
    digit = 0 + compressLevel.getValue();
    compressionInput.setSelectedItem(digit);
    digit = 0 + qualityLevel.getValue();
    jpegInput.setSelectedItem(digit);

    handleAutoselect();
    handleCompression();
    handleJpeg();

    /* Security */
    Security security = new Security(SecurityClient.secTypes);

    List<Integer> secTypes;
    Iterator<Integer> iter;

    List<Integer> secTypesExt;
    Iterator<Integer> iterExt;

    encNoneCheckbox.setSelected(false);
    encTLSCheckbox.setSelected(false);
    encX509Checkbox.setSelected(false);

    authNoneCheckbox.setSelected(false);
    authVncCheckbox.setSelected(false);
    authPlainCheckbox.setSelected(false);
    authIdentCheckbox.setSelected(false);
    sendLocalUsernameCheckbox.setSelected(sendLocalUsername.getValue());

    secTypes = security.GetEnabledSecTypes();
    for (iter = secTypes.iterator(); iter.hasNext(); ) {
      switch ((Integer)iter.next()) {
      case Security.secTypeNone:
        encNoneCheckbox.setSelected(true);
        authNoneCheckbox.setSelected(true);
        break;
      case Security.secTypeVncAuth:
        encNoneCheckbox.setSelected(true);
        authVncCheckbox.setSelected(true);
        break;
      }
    }

    secTypesExt = security.GetEnabledExtSecTypes();
    for (iterExt = secTypesExt.iterator(); iterExt.hasNext(); ) {
      switch ((Integer)iterExt.next()) {
      case Security.secTypePlain:
        encNoneCheckbox.setSelected(true);
        authPlainCheckbox.setSelected(true);
        break;
      case Security.secTypeIdent:
        encNoneCheckbox.setSelected(true);
        authIdentCheckbox.setSelected(true);
        break;
      case Security.secTypeTLSNone:
        encTLSCheckbox.setSelected(true);
        authNoneCheckbox.setSelected(true);
        break;
      case Security.secTypeTLSVnc:
        encTLSCheckbox.setSelected(true);
        authVncCheckbox.setSelected(true);
        break;
      case Security.secTypeTLSPlain:
        encTLSCheckbox.setSelected(true);
        authPlainCheckbox.setSelected(true);
        break;
      case Security.secTypeTLSIdent:
        encTLSCheckbox.setSelected(true);
        authIdentCheckbox.setSelected(true);
        break;
      case Security.secTypeX509None:
        encX509Checkbox.setSelected(true);
        authNoneCheckbox.setSelected(true);
        break;
      case Security.secTypeX509Vnc:
        encX509Checkbox.setSelected(true);
        authVncCheckbox.setSelected(true);
        break;
      case Security.secTypeX509Plain:
        encX509Checkbox.setSelected(true);
        authPlainCheckbox.setSelected(true);
        break;
      case Security.secTypeX509Ident:
        encX509Checkbox.setSelected(true);
        authIdentCheckbox.setSelected(true);
        break;
      }
    }

    File caFile = new File(CSecurityTLS.X509CA.getValueStr());
    if (caFile.exists() && caFile.canRead())
      caInput.setText(caFile.getAbsolutePath());
    File crlFile = new File(CSecurityTLS.X509CRL.getValueStr());
    if (crlFile.exists() && crlFile.canRead())
      crlInput.setText(crlFile.getAbsolutePath());

    handleX509();
    handleSendLocalUsername();

    /* Input */
    viewOnlyCheckbox.setSelected(viewOnly.getValue());
    acceptClipboardCheckbox.setSelected(acceptClipboard.getValue());
    sendClipboardCheckbox.setSelected(sendClipboard.getValue());

    menuKeyChoice.setSelectedIndex(0);

    String menuKeyStr = menuKey.getValueStr();
    for (int i = 0; i < menuKeyChoice.getItemCount(); i++)
      if (menuKeyStr.equals(menuKeyChoice.getItemAt(i)))
        menuKeyChoice.setSelectedIndex(i);

    /* Screen */
    String width, height;

    if (desktopSize.getValueStr().isEmpty() ||
        desktopSize.getValueStr().split("x").length != 2) {
      desktopSizeCheckbox.setSelected(false);
      desktopWidthInput.setText("1024");
      desktopHeightInput.setText("768");
    } else {
      desktopSizeCheckbox.setSelected(true);
      width = desktopSize.getValueStr().split("x")[0];
      desktopWidthInput.setText(width);
      height = desktopSize.getValueStr().split("x")[1];
      desktopHeightInput.setText(height);
    }
    if (remoteResize.getValue())
      remoteResizeButton.setSelected(true);
    else
      remoteScaleButton.setSelected(true);
    fullScreenCheckbox.setSelected(fullScreen.getValue());
    fullScreenAllMonitorsCheckbox.setSelected(fullScreenAllMonitors.getValue());

    scalingFactorInput.setSelectedItem("100%");
    String scaleStr = scalingFactor.getValueStr();
    if (scaleStr.matches("^[0-9]+$"))
      scaleStr = scaleStr.concat("%");
    if (scaleStr.matches("^FixedRatio$"))
      scaleStr = new String("Fixed Aspect Ratio");
    for (int i = 0; i < scalingFactorInput.getItemCount(); i++)
      if (scaleStr.equals(scalingFactorInput.getItemAt(i)))
        scalingFactorInput.setSelectedIndex(i);

    handleDesktopSize();

    /* Misc. */
    sharedCheckbox.setSelected(shared.getValue());
    dotWhenNoCursorCheckbox.setSelected(dotWhenNoCursor.getValue());
    acceptBellCheckbox.setSelected(acceptBell.getValue());

    /* SSH */
    File f;
    tunnelCheckbox.setSelected(tunnel.getValue() || !via.getValueStr().isEmpty());
    viaCheckbox.setSelected(!via.getValueStr().isEmpty());
    if (viaCheckbox.isSelected()) {
      viaUserInput.setText(Tunnel.getSshUser());
      viaHostInput.setText(Tunnel.getSshHost());
      viaPortInput.setText(Integer.toString(Tunnel.getSshPort()));
    }
    extSSHCheckbox.setSelected(extSSH.getValue());
    f = new File(extSSHClient.getValueStr());
    if (f.exists() && f.isFile() && f.canExecute())
      sshClientInput.setText(f.getAbsolutePath());
    if (extSSHArgs.getValueStr().isEmpty()) {
      sshArgsDefaultButton.setSelected(true);
    } else {
      sshArgsCustomButton.setSelected(true);
      sshArgsInput.setText(extSSHArgs.getValueStr());
    }
    f = new File(sshKeyFile.getValueStr());
    if (f.exists() && f.isFile() && f.canRead())
      sshKeyFileInput.setText(f.getAbsolutePath());
    f = new File(sshConfig.getValueStr());
    if (f.exists() && f.isFile() && f.canRead())
      sshConfigInput.setText(f.getAbsolutePath());

    handleTunnel();
    handleVia();
    handleExtSSH();
    handleRfbState();
  }

  private void storeOptions() {
    /* Compression */
    autoSelect.setParam(autoselectCheckbox.isSelected());

    if (tightButton.isSelected())
      preferredEncoding.setParam(Encodings.encodingName(Encodings.encodingTight));
    else if (zrleButton.isSelected())
      preferredEncoding.setParam(Encodings.encodingName(Encodings.encodingZRLE));
    else if (hextileButton.isSelected())
      preferredEncoding.setParam(Encodings.encodingName(Encodings.encodingHextile));
    else if (rawButton.isSelected())
      preferredEncoding.setParam(Encodings.encodingName(Encodings.encodingRaw));

    fullColor.setParam(fullcolorButton.isSelected());
    if (verylowcolorButton.isSelected())
      lowColorLevel.setParam(0);
    else if (lowcolorButton.isSelected())
      lowColorLevel.setParam(1);
    else if (mediumcolorButton.isSelected())
      lowColorLevel.setParam(2);

    customCompressLevel.setParam(compressionCheckbox.isSelected());
    noJpeg.setParam(!jpegCheckbox.isSelected());
    compressLevel.setParam((Integer)compressionInput.getSelectedItem());
    qualityLevel.setParam((Integer)jpegInput.getSelectedItem());

    /* Security */
    Security security = new Security();

    /* Process security types which don't use encryption */
    if (encNoneCheckbox.isSelected()) {
      if (authNoneCheckbox.isSelected())
        security.EnableSecType(Security.secTypeNone);
      if (authVncCheckbox.isSelected())
        security.EnableSecType(Security.secTypeVncAuth);
      if (authPlainCheckbox.isSelected())
        security.EnableSecType(Security.secTypePlain);
      if (authIdentCheckbox.isSelected())
        security.EnableSecType(Security.secTypeIdent);
    }

    /* Process security types which use TLS encryption */
    if (encTLSCheckbox.isSelected()) {
      if (authNoneCheckbox.isSelected())
        security.EnableSecType(Security.secTypeTLSNone);
      if (authVncCheckbox.isSelected())
        security.EnableSecType(Security.secTypeTLSVnc);
      if (authPlainCheckbox.isSelected())
        security.EnableSecType(Security.secTypeTLSPlain);
      if (authIdentCheckbox.isSelected())
        security.EnableSecType(Security.secTypeTLSIdent);
    }

    /* Process security types which use X509 encryption */
    if (encX509Checkbox.isSelected()) {
      if (authNoneCheckbox.isSelected())
        security.EnableSecType(Security.secTypeX509None);
      if (authVncCheckbox.isSelected())
        security.EnableSecType(Security.secTypeX509Vnc);
      if (authPlainCheckbox.isSelected())
        security.EnableSecType(Security.secTypeX509Plain);
      if (authIdentCheckbox.isSelected())
        security.EnableSecType(Security.secTypeX509Ident);
    }

    if (authIdentCheckbox.isSelected() ||
        authPlainCheckbox.isSelected()) {
      sendLocalUsername.setParam(sendLocalUsernameCheckbox.isSelected());
    }

    SecurityClient.secTypes.setParam(security.ToString());

    File caFile = new File(caInput.getText());
    if (caFile.exists() && caFile.canRead())
      CSecurityTLS.X509CA.setParam(caFile.getAbsolutePath());
    File crlFile = new File(crlInput.getText());
    if (crlFile.exists() && crlFile.canRead())
      CSecurityTLS.X509CRL.setParam(crlFile.getAbsolutePath());

    /* Input */
    viewOnly.setParam(viewOnlyCheckbox.isSelected());
    acceptClipboard.setParam(acceptClipboardCheckbox.isSelected());
    sendClipboard.setParam(sendClipboardCheckbox.isSelected());

    String menuKeyStr =
      MenuKey.getMenuKeySymbols()[menuKeyChoice.getSelectedIndex()].name;
    menuKey.setParam(menuKeyStr);

    /* Screen */
    if (desktopSizeCheckbox.isSelected() &&
        !desktopWidthInput.getText().isEmpty() &&
        !desktopHeightInput.getText().isEmpty()) {
      String width = desktopWidthInput.getText();
      String height = desktopHeightInput.getText();
      desktopSize.setParam(width.concat("x").concat(height));
    } else {
      desktopSize.setParam("");
    }
    remoteResize.setParam(remoteResizeButton.isSelected());
    fullScreen.setParam(fullScreenCheckbox.isSelected());
    fullScreenAllMonitors.setParam(fullScreenAllMonitorsCheckbox.isSelected());

    String scaleStr =
      ((String)scalingFactorInput.getSelectedItem()).replace("%", "");
    scaleStr.replace("Fixed Aspect Ratio", "FixedRatio");
    scalingFactor.setParam(scaleStr);

    /* Misc. */
    shared.setParam(sharedCheckbox.isSelected());
    dotWhenNoCursor.setParam(dotWhenNoCursorCheckbox.isSelected());
    acceptBell.setParam(acceptBellCheckbox.isSelected());

    /* SSH */
    tunnel.setParam(tunnelCheckbox.isSelected());
    if (viaCheckbox.isSelected() &&
        !viaUserInput.getText().isEmpty() &&
        !viaHostInput.getText().isEmpty() &&
        !viaPortInput.getText().isEmpty()) {
      String sshUser = viaUserInput.getText();
      String sshHost = viaHostInput.getText();
      String sshPort = viaPortInput.getText();
      String viaStr = sshUser.concat("@").concat(sshHost).concat(":").concat(sshPort);
      via.setParam(viaStr);
    } else {
      via.setParam("");
    }
    extSSH.setParam(extSSHCheckbox.isSelected());
    if (!sshClientInput.getText().isEmpty())
      extSSHClient.setParam(sshClientInput.getText());
    if (sshArgsCustomButton.isSelected() &&
        !sshArgsInput.getText().isEmpty()) {
        extSSHArgs.setParam(sshArgsInput.getText());
    } else {
      extSSHArgs.setParam(new String());
    }
    if (!sshConfigInput.getText().isEmpty())
      sshConfig.setParam(sshConfigInput.getText());
    if (!sshKeyFileInput.getText().isEmpty())
      sshKeyFile.setParam(sshKeyFileInput.getText());

    try {
      for (Map.Entry<Object, String> iter : callbacks.entrySet()) {
        Object obj = iter.getKey();
        Method cb = obj.getClass().getMethod(iter.getValue(), new Class[]{});
        if (cb == null)
          vlog.info(obj.getClass().getName());
        cb.invoke(obj);
      }
    } catch (NoSuchMethodException e) {
      vlog.error("NoSuchMethodException: "+e.getMessage());
    } catch (IllegalAccessException e) {
      vlog.error("IllegalAccessException: "+e.getMessage());
    } catch (InvocationTargetException e) {
      vlog.error("InvocationTargetException: "+e.getMessage());
    }
  }

  private JPanel createCompressionPanel() {
    JPanel FormatPanel = new JPanel();
    FormatPanel.setLayout(new BoxLayout(FormatPanel,
                                        BoxLayout.PAGE_AXIS));
    FormatPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel autoSelectPane = new JPanel();
    autoSelectPane.setLayout(new BoxLayout(autoSelectPane,
                                           BoxLayout.LINE_AXIS));
    autoSelectPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 5, 0));
    autoselectCheckbox = new JCheckBox("Auto Select");
    autoselectCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleAutoselect();
      }
    });
    autoSelectPane.add(autoselectCheckbox);
    autoSelectPane.add(Box.createHorizontalGlue());

    JPanel encodingPanel = new JPanel(new GridLayout(4, 1));
    encodingPanel.setBorder(BorderFactory.createTitledBorder("Preferred encoding"));
    tightButton = new GroupedJRadioButton("Tight", encodingGroup, encodingPanel);
    zrleButton = new GroupedJRadioButton("ZRLE", encodingGroup, encodingPanel);
    hextileButton = new GroupedJRadioButton("Hextile", encodingGroup, encodingPanel);
    rawButton = new GroupedJRadioButton("Raw", encodingGroup, encodingPanel);

    JPanel colorPanel = new JPanel(new GridLayout(4, 1));
    colorPanel.setBorder(BorderFactory.createTitledBorder("Color level"));
    fullcolorButton = new GroupedJRadioButton("Full", colorlevelGroup, colorPanel);
    mediumcolorButton = new GroupedJRadioButton("Medium", colorlevelGroup, colorPanel);
    lowcolorButton = new GroupedJRadioButton("Low", colorlevelGroup, colorPanel);
    verylowcolorButton = new GroupedJRadioButton("Very low", colorlevelGroup, colorPanel);

    JPanel encodingPane = new JPanel(new GridLayout(1, 2, 5, 0));
    encodingPane.setBorder(BorderFactory.createEmptyBorder(0, 0, 5, 0));
    encodingPane.add(encodingPanel);
    encodingPane.add(colorPanel);

    JPanel tightPanel = new JPanel(new GridBagLayout());
    compressionCheckbox = new JCheckBox("Custom Compression Level");
    compressionCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleCompression();
      }
    });
    Object[] compressionLevels = { 1, 2, 3, 4, 5, 6 };
    compressionInput = new MyJComboBox(compressionLevels);
    ((MyJComboBox)compressionInput).setDocument(new IntegerDocument(1));
    compressionInput.setPrototypeDisplayValue("0.");
    compressionInput.setEditable(true);
    JLabel compressionLabel =
      new JLabel("Level (0=fast, 9=best)");
    jpegCheckbox = new JCheckBox("Allow JPEG Compression");
    jpegCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleJpeg();
      }
    });
    Object[] qualityLevels = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    jpegInput = new MyJComboBox(qualityLevels);
    jpegInput.setPrototypeDisplayValue("0.");
    JLabel qualityLabel = new JLabel("Quality (0=poor, 9=best)");

    tightPanel.add(compressionCheckbox,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));
    int indent = getButtonLabelInset(compressionCheckbox);
    tightPanel.add(compressionInput,
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
    tightPanel.add(jpegCheckbox,
                   new GridBagConstraints(0, 2,
                                          REMAINDER, 1,
                                          LIGHT, LIGHT, 
                                          LINE_START, NONE,
                                          new Insets(5, 0, 0, 0),
                                          NONE, NONE));
    indent = getButtonLabelInset(jpegCheckbox);
    tightPanel.add(jpegInput,
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
    return FormatPanel;
  }

  private JPanel createSecurityPanel() {
    JPanel SecPanel = new JPanel();
    SecPanel.setLayout(new BoxLayout(SecPanel,
                                     BoxLayout.PAGE_AXIS));
    SecPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel vencryptPane = new JPanel();
    vencryptPane.setLayout(new BoxLayout(vencryptPane,
                                         BoxLayout.LINE_AXIS));
    vencryptPane.setBorder(BorderFactory.createEmptyBorder(0,0,5,0));

    JPanel encrPanel = new JPanel(new GridBagLayout());
    encrPanel.setBorder(BorderFactory.createTitledBorder("Encryption"));
    encNoneCheckbox = new JCheckBox("None");
    encTLSCheckbox = new JCheckBox("Anonymous TLS");
    encX509Checkbox = new JCheckBox("TLS with X.509 certificates");
    encX509Checkbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleX509();
      }
    });
    JLabel caLabel = new JLabel("X.509 CA Certificate");
    caInput = new JTextField();
    caChooser = new JButton("Browse");
    caChooser.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComponent c = ((JButton)e.getSource()).getRootPane();
        File dflt = new File(CSecurityTLS.X509CA.getValueStr());
        FileNameExtensionFilter filter =
          new FileNameExtensionFilter("X.509 certificate", "crt", "cer", "pem");
        File f = showChooser("Path to X509 CA certificate", dflt, c, filter);
        if (f != null && f.exists() && f.canRead())
          caInput.setText(f.getAbsolutePath());
      }
    });
    JLabel crlLabel = new JLabel("X.509 CRL file");
    crlInput = new JTextField();
    crlChooser = new JButton("Browse");
    crlChooser.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComponent c = ((JButton)e.getSource()).getRootPane();
        File dflt = new File(CSecurityTLS.X509CRL.getValueStr());
        FileNameExtensionFilter filter =
          new FileNameExtensionFilter("X.509 CRL", "crl");
        File f = showChooser("Path to X509 CRL file", dflt, c, filter);
        if (f != null && f.exists() && f.canRead())
          crlInput.setText(f.getAbsolutePath());
      }
    });
    encrPanel.add(encNoneCheckbox,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    encrPanel.add(encTLSCheckbox,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    encrPanel.add(encX509Checkbox,
                  new GridBagConstraints(0, 2,
                                         3, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 0, 0),
                                         NONE, NONE));
    int indent = getButtonLabelInset(encX509Checkbox);
    encrPanel.add(caLabel,
                  new GridBagConstraints(0, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_END, NONE,
                                         new Insets(0, indent, 5, 0),
                                         0, 0));
    encrPanel.add(caInput,
                  new GridBagConstraints(1, 3,
                                         1, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 5, 5, 0),
                                         0, 0));
    encrPanel.add(caChooser,
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
    encrPanel.add(crlInput,
                  new GridBagConstraints(1, 4,
                                         1, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 5, 0, 0),
                                         0, 0));
    encrPanel.add(crlChooser,
                  new GridBagConstraints(2, 4,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, VERTICAL,
                                         new Insets(0, 5, 0, 0),
                                         0, 0));

    JPanel authPanel = new JPanel(new GridBagLayout());
    authPanel.setBorder(BorderFactory.createTitledBorder("Authentication"));

    authNoneCheckbox = new JCheckBox("None");
    authVncCheckbox = new JCheckBox("Standard VNC");
    authPlainCheckbox = new JCheckBox("Plaintext");
    authPlainCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleSendLocalUsername();
      }
    });
    authIdentCheckbox = new JCheckBox("Ident");
    authIdentCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleSendLocalUsername();
      }
    });
    sendLocalUsernameCheckbox = new JCheckBox("Send Local Username");
    authPanel.add(authNoneCheckbox,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    authPanel.add(authVncCheckbox,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    authPanel.add(authPlainCheckbox,
                  new GridBagConstraints(0, 2,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(authIdentCheckbox,
                  new GridBagConstraints(0, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 0, 0),
                                         NONE, NONE));
    authPanel.add(sendLocalUsernameCheckbox,
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
    return SecPanel;
  }

  private JPanel createInputPanel() {
    JPanel inputPanel = new JPanel(new GridBagLayout());
    inputPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    viewOnlyCheckbox = new JCheckBox("View only (ignore mouse and keyboard)");
    acceptClipboardCheckbox = new JCheckBox("Accept clipboard from server");
    sendClipboardCheckbox = new JCheckBox("Send clipboard to server");
    JLabel menuKeyLabel = new JLabel("Menu key");
    String[] menuKeys = new String[MenuKey.getMenuKeySymbolCount()];
    //menuKeys[0] = "None";
    for (int i = 0; i < MenuKey.getMenuKeySymbolCount(); i++)
      menuKeys[i] = MenuKey.getKeyText(MenuKey.getMenuKeySymbols()[i]);
    menuKeyChoice = new JComboBox(menuKeys);

    inputPanel.add(viewOnlyCheckbox,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(acceptClipboardCheckbox,
                   new GridBagConstraints(0, 1,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(sendClipboardCheckbox,
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
    inputPanel.add(menuKeyChoice,
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
    return inputPanel;
  }

  private JPanel createScreenPanel() {
    JPanel ScreenPanel = new JPanel(new GridBagLayout());
    ScreenPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel SizingPanel = new JPanel(new GridBagLayout());
    SizingPanel.setBorder(BorderFactory.createTitledBorder("Desktop Sizing"));
    desktopSizeCheckbox = new JCheckBox("Resize remote session on connect");
    desktopSizeCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleDesktopSize();
      }
    });
    desktopWidthInput = new IntegerTextField(5);
    desktopHeightInput = new IntegerTextField(5);
    JPanel desktopSizePanel =
      new JPanel(new FlowLayout(FlowLayout.LEADING, 0, 0));
    desktopSizePanel.add(desktopWidthInput);
    desktopSizePanel.add(new JLabel(" x "));
    desktopSizePanel.add(desktopHeightInput);
    sizingGroup = new ButtonGroup();
    remoteResizeButton =
      new JRadioButton("Resize remote session to the local window");
    sizingGroup.add(remoteResizeButton);
    remoteScaleButton =
      new JRadioButton("Scale remote session to the local window");
    sizingGroup.add(remoteScaleButton);
    remoteResizeButton.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleRemoteResize();
      }
    });
    JLabel scalingFactorLabel = new JLabel("Scaling Factor");
    Object[] scalingFactors = {
      "Auto", "Fixed Aspect Ratio", "50%", "75%", "95%", "100%", "105%",
      "125%", "150%", "175%", "200%", "250%", "300%", "350%", "400%" };
    scalingFactorInput = new MyJComboBox(scalingFactors);
    scalingFactorInput.setEditable(true);
    fullScreenCheckbox = new JCheckBox("Full-screen mode");
    fullScreenAllMonitorsCheckbox =
      new JCheckBox("Enable full-screen mode over all monitors");
    SizingPanel.add(desktopSizeCheckbox,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    int indent = getButtonLabelInset(desktopSizeCheckbox);
    SizingPanel.add(desktopSizePanel,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, indent, 0, 0),
                                           NONE, NONE));
    SizingPanel.add(remoteResizeButton,
                    new GridBagConstraints(0, 2,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    SizingPanel.add(remoteScaleButton,
                    new GridBagConstraints(0, 3,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(remoteScaleButton);
    SizingPanel.add(scalingFactorLabel,
                    new GridBagConstraints(0, 4,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, indent, 4, 0),
                                           NONE, NONE));
    SizingPanel.add(scalingFactorInput,
                    new GridBagConstraints(1, 4,
                                           1, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 5, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(SizingPanel,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(fullScreenCheckbox,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(fullScreenCheckbox);
    ScreenPanel.add(fullScreenAllMonitorsCheckbox,
                    new GridBagConstraints(0, 2,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, indent, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                    new GridBagConstraints(0, 3,
                                           REMAINDER, REMAINDER,
                                           HEAVY, HEAVY,
                                           LINE_START, BOTH,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    return ScreenPanel;
  }

  private JPanel createMiscPanel() {
    JPanel MiscPanel = new JPanel(new GridBagLayout());
    MiscPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));
    sharedCheckbox =
      new JCheckBox("Shared (don't disconnect other viewers)");
    dotWhenNoCursorCheckbox = new JCheckBox("Show dot when no cursor");
    acceptBellCheckbox = new JCheckBox("Beep when requested by the server");
    MiscPanel.add(sharedCheckbox,
                  new GridBagConstraints(0, 0,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(dotWhenNoCursorCheckbox,
                  new GridBagConstraints(0, 1,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(acceptBellCheckbox,
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
    return MiscPanel;
  }

  private JPanel createSshPanel() {
    JPanel sshPanel = new JPanel(new GridBagLayout());
    sshPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));
    ButtonGroup sshArgsGroup = new ButtonGroup();
    tunnelCheckbox = new JCheckBox("Tunnel VNC over SSH");
    tunnelCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleTunnel();
      }
    });

    JPanel tunnelPanel = new JPanel(new GridBagLayout());

    viaCheckbox = new JCheckBox("Use SSH gateway");
    viaCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleVia();
      }
    });
    JLabel sshUserLabel = new JLabel("Username");
    viaUserInput = new JTextField();
    JLabel sshUserAtLabel = new JLabel("@");
    JLabel sshHostLabel = new JLabel("Hostname (or IP address)");
    viaHostInput = new JTextField("");
    JLabel sshPortLabel = new JLabel("Port");
    viaPortInput = new IntegerTextField(5);

    extSSHCheckbox = new JCheckBox("Use external SSH client");
    extSSHCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleExtSSH();
      }
    });
    sshClientInput = new JTextField();
    sshClientChooser = new JButton("Browse");
    sshClientChooser.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComponent c = ((JButton)e.getSource()).getRootPane();
        File dflt = new File(extSSHClient.getValueStr());
        File f = showChooser("Path to external SSH client", dflt, c);
        if (f != null && f.exists() && f.isFile() && f.canExecute())
          sshClientInput.setText(f.getAbsolutePath());
      }
    });
    JLabel sshConfigLabel = new JLabel("SSH config file");
    sshConfigInput = new JTextField();
    sshConfigChooser = new JButton("Browse");
    sshConfigChooser.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComponent c = ((JButton)e.getSource()).getRootPane();
        File dflt = new File(sshConfig.getValueStr());
        File f = showChooser("Path to OpenSSH client config file", dflt, c);
        if (f != null && f.exists() && f.isFile() && f.canRead())
          sshConfigInput.setText(f.getAbsolutePath());
      }
    });
    JLabel sshKeyFileLabel = new JLabel("SSH identity file");
    sshKeyFileInput = new JTextField();
    sshKeyFileChooser = new JButton("Browse");
    sshKeyFileChooser.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        JComponent c = ((JButton)e.getSource()).getRootPane();
        File f = showChooser("Path to SSH key file", null, c);
        if (f != null && f.exists() && f.isFile() && f.canRead())
          sshKeyFileInput.setText(f.getAbsolutePath());
      }
    });
    JPanel sshArgsPanel = new JPanel(new GridBagLayout());
    JLabel sshArgsLabel = new JLabel("Arguments:");
    sshArgsDefaultButton = new GroupedJRadioButton("Default", sshArgsGroup, sshArgsPanel);
    sshArgsDefaultButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        sshArgsInput.setEnabled(sshArgsCustomButton.isSelected());
      }
    });
    sshArgsCustomButton = new GroupedJRadioButton("Custom", sshArgsGroup, sshArgsPanel);
    sshArgsCustomButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        sshArgsInput.setEnabled(sshArgsCustomButton.isSelected());
      }
    });
    sshArgsInput = new JTextField();

    JPanel gatewayPanel = new JPanel(new GridBagLayout());
    gatewayPanel.add(viaCheckbox,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    int indent = getButtonLabelInset(viaCheckbox);
    gatewayPanel.add(sshUserLabel,
                 new GridBagConstraints(0, 1,
                                        1, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, indent, 4, 0),
                                        NONE, NONE));
    gatewayPanel.add(sshHostLabel,
                 new GridBagConstraints(2, 1,
                                        1, 1,
                                        HEAVY, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    gatewayPanel.add(sshPortLabel,
                 new GridBagConstraints(3, 1,
                                        1, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 5, 4, 0),
                                        NONE, NONE));
    gatewayPanel.add(viaUserInput,
                 new GridBagConstraints(0, 2,
                                        1, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, indent, 0, 0),
                                        NONE, NONE));
    gatewayPanel.add(sshUserAtLabel,
                 new GridBagConstraints(1, 2,
                                        1, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 2, 0, 2),
                                        NONE, NONE));
    gatewayPanel.add(viaHostInput,
                 new GridBagConstraints(2, 2,
                                        1, 1,
                                        HEAVY, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 0, 0),
                                        NONE, NONE));
    gatewayPanel.add(viaPortInput,
                 new GridBagConstraints(3, 2,
                                        1, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 5, 0, 0),
                                        NONE, NONE));

    JPanel clientPanel = new JPanel(new GridBagLayout());
    clientPanel.add(extSSHCheckbox,
                    new GridBagConstraints(0, 0,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    clientPanel.add(sshClientInput,
                    new GridBagConstraints(1, 0,
                                           1, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 5, 0, 0),
                                           NONE, NONE));
    clientPanel.add(sshClientChooser,
                    new GridBagConstraints(2, 0,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 5, 0, 0),
                                           NONE, NONE));
    sshArgsPanel.add(sshArgsLabel,
                    new GridBagConstraints(0, 1,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    sshArgsPanel.add(sshArgsDefaultButton,
                    new GridBagConstraints(1, 1,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 5, 0, 0),
                                           NONE, NONE));
    sshArgsPanel.add(sshArgsCustomButton,
                    new GridBagConstraints(2, 1,
                                           1, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 5, 0, 0),
                                           NONE, NONE));
    sshArgsPanel.add(sshArgsInput,
                    new GridBagConstraints(3, 1,
                                           1, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 5, 0, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(extSSHCheckbox);
    clientPanel.add(sshArgsPanel,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(4, indent, 0, 0),
                                           NONE, NONE));

    JPanel opensshPanel = new JPanel(new GridBagLayout());
    TitledBorder border =
      BorderFactory.createTitledBorder("Embedded SSH client configuration");
    opensshPanel.setBorder(border);
    opensshPanel.add(sshConfigLabel,
                     new GridBagConstraints(0, 0,
                                            1, 1,
                                            LIGHT, LIGHT,
                                            LINE_START, NONE,
                                            new Insets(0, 0, 5, 0),
                                            NONE, NONE));
    opensshPanel.add(sshConfigInput,
                     new GridBagConstraints(1, 0,
                                            1, 1,
                                            HEAVY, LIGHT,
                                            LINE_START, HORIZONTAL,
                                            new Insets(0, 5, 5, 0),
                                            NONE, NONE));
    opensshPanel.add(sshConfigChooser,
                     new GridBagConstraints(2, 0,
                                            1, 1,
                                            LIGHT, LIGHT,
                                            LINE_START, VERTICAL,
                                            new Insets(0, 5, 5, 0),
                                            NONE, NONE));
    opensshPanel.add(sshKeyFileLabel,
                     new GridBagConstraints(0, 1,
                                            1, 1,
                                            LIGHT, LIGHT,
                                            LINE_START, NONE,
                                            new Insets(0, 0, 0, 0),
                                            NONE, NONE));
    opensshPanel.add(sshKeyFileInput,
                     new GridBagConstraints(1, 1,
                                            1, 1,
                                            HEAVY, LIGHT,
                                            LINE_START, HORIZONTAL,
                                            new Insets(0, 5, 0, 0),
                                            NONE, NONE));
    opensshPanel.add(sshKeyFileChooser,
                     new GridBagConstraints(2, 1,
                                            1, 1,
                                            LIGHT, LIGHT,
                                            LINE_START, VERTICAL,
                                            new Insets(0, 5, 0, 0),
                                            NONE, NONE));
    tunnelPanel.add(gatewayPanel,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    tunnelPanel.add(clientPanel,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    tunnelPanel.add(opensshPanel,
                    new GridBagConstraints(0, 2,
                                           REMAINDER, 1,
                                           HEAVY, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));

    sshPanel.add(tunnelCheckbox,
                 new GridBagConstraints(0, 0,
                                        REMAINDER, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, NONE,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    indent = getButtonLabelInset(tunnelCheckbox);
    sshPanel.add(tunnelPanel,
                 new GridBagConstraints(0, 2,
                                        REMAINDER, 1,
                                        LIGHT, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, indent, 4, 0),
                                        NONE, NONE));
    sshPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                 new GridBagConstraints(0, RELATIVE,
                                        REMAINDER, REMAINDER,
                                        HEAVY, HEAVY,
                                        LINE_START, BOTH,
                                        new Insets(0, 0, 0, 0),
                                        NONE, NONE));
    return sshPanel;
  }

  private void handleAutoselect()
  {
    ButtonGroup[] groups = { encodingGroup, colorlevelGroup };
    for (ButtonGroup grp : groups) {
      Enumeration<AbstractButton> elems = grp.getElements();
      while (elems.hasMoreElements())
        elems.nextElement().setEnabled(!autoselectCheckbox.isSelected());
    }

    // JPEG setting is also affected by autoselection
    jpegCheckbox.setEnabled(!autoselectCheckbox.isSelected());
    handleJpeg();
  }

  private void handleCompression()
  {
    compressionInput.setEnabled(compressionCheckbox.isSelected());
  }

  private void handleJpeg()
  {
    if (jpegCheckbox.isSelected() &&
        !autoselectCheckbox.isSelected())
      jpegInput.setEnabled(true);
    else
      jpegInput.setEnabled(false);
  }

  private void handleX509()
  {
    caInput.setEnabled(encX509Checkbox.isSelected());
    caChooser.setEnabled(encX509Checkbox.isSelected());
    crlInput.setEnabled(encX509Checkbox.isSelected());
    crlChooser.setEnabled(encX509Checkbox.isSelected());
  }

  private void handleSendLocalUsername()
  {
    boolean value = authIdentCheckbox.isSelected() ||
                    authPlainCheckbox.isSelected();
        sendLocalUsernameCheckbox.setEnabled(value);
  }

  private void handleDesktopSize()
  {
    desktopWidthInput.setEnabled(desktopSizeCheckbox.isSelected());
    desktopHeightInput.setEnabled(desktopSizeCheckbox.isSelected());
  }

  private void handleRemoteResize()
  {
    scalingFactorInput.setEnabled(!remoteResizeButton.isSelected());
  }

  private void handleTunnel()
  {
    viaCheckbox.setEnabled(tunnelCheckbox.isSelected());
    extSSHCheckbox.setEnabled(tunnelCheckbox.isSelected());
    if (tunnelCheckbox.isSelected()) {
      JComponent[] components = { viaUserInput, viaHostInput, viaPortInput };
      for (JComponent c : components)
        c.setEnabled(viaCheckbox.isSelected());
      sshClientInput.setEnabled(extSSHCheckbox.isSelected());
      sshClientChooser.setEnabled(extSSHCheckbox.isSelected());
      sshArgsDefaultButton.setEnabled(extSSHCheckbox.isSelected());
      sshArgsCustomButton.setEnabled(extSSHCheckbox.isSelected());
      sshArgsInput.setEnabled(extSSHCheckbox.isSelected());
      sshConfigInput.setEnabled(!extSSHCheckbox.isSelected());
      sshConfigChooser.setEnabled(!extSSHCheckbox.isSelected());
      sshKeyFileInput.setEnabled(!extSSHCheckbox.isSelected());
      sshKeyFileChooser.setEnabled(!extSSHCheckbox.isSelected());
    } else {
      JComponent[] components = {
        viaUserInput, viaHostInput, viaPortInput, sshClientInput,
        sshClientChooser, sshArgsDefaultButton, sshArgsCustomButton,
        sshArgsInput, sshConfigInput, sshConfigChooser, sshKeyFileInput,
        sshKeyFileChooser, };
      for (JComponent c : components)
        c.setEnabled(false);
    }
  }

  private void handleVia()
  {
    if (tunnelCheckbox.isSelected()) {
      viaUserInput.setEnabled(viaCheckbox.isSelected());
      viaHostInput.setEnabled(viaCheckbox.isSelected());
      viaPortInput.setEnabled(viaCheckbox.isSelected());
    }
  }

  private void handleExtSSH()
  {
    if (tunnelCheckbox.isSelected()) {
      sshClientInput.setEnabled(extSSHCheckbox.isSelected());
      sshClientChooser.setEnabled(extSSHCheckbox.isSelected());
      sshArgsDefaultButton.setEnabled(extSSHCheckbox.isSelected());
      sshArgsCustomButton.setEnabled(extSSHCheckbox.isSelected());
      sshConfigInput.setEnabled(!extSSHCheckbox.isSelected());
      sshConfigChooser.setEnabled(!extSSHCheckbox.isSelected());
      sshKeyFileInput.setEnabled(!extSSHCheckbox.isSelected());
      sshKeyFileChooser.setEnabled(!extSSHCheckbox.isSelected());
      if (sshArgsCustomButton.isSelected())
        sshArgsInput.setEnabled(extSSHCheckbox.isSelected());
      else
        sshArgsInput.setEnabled(false);
    }
  }

  private void handleRfbState()
  {
    CConn cc = VncViewer.cc;
    if (cc != null && cc.state() == CConnection.stateEnum.RFBSTATE_NORMAL) {
      JComponent[] components = {
          encNoneCheckbox, encTLSCheckbox, encX509Checkbox, authNoneCheckbox,
          authVncCheckbox, authVncCheckbox, authIdentCheckbox, authPlainCheckbox,
          sendLocalUsernameCheckbox, caInput, caChooser, crlInput, crlChooser,
          sharedCheckbox, tunnelCheckbox, viaCheckbox, viaUserInput, viaHostInput,
          viaPortInput, extSSHCheckbox, sshClientInput, sshClientChooser,
          sshArgsDefaultButton, sshArgsCustomButton, sshArgsInput, sshConfigInput,
          sshKeyFileInput, sshConfigChooser, sshKeyFileChooser,
        };
      for (JComponent c : components)
        c.setEnabled(false);
    }
  }

  static LogWriter vlog = new LogWriter("OptionsDialog");
}
