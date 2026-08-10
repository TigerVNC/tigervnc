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
  JCheckBox encRSAAESCheckbox;
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
  JCheckBox disableArrowScrollCheckbox;
  JComboBox menuKeyChoice;

  /* Screen */
  JCheckBox desktopSizeCheckbox;
  JTextField desktopWidthInput;
  JTextField desktopHeightInput;

  ButtonGroup sizingGroup;
  JRadioButton remoteResizeButton;
  JRadioButton remoteScaleButton;
  JComboBox scalingFactorInput;

  ButtonGroup displayModeGroup;
  JRadioButton modeWindowedButton;
  JRadioButton modeCurrentMonitorButton;
  JRadioButton modeAllMonitorsButton;
  JRadioButton modeSelectedMonitorsButton;
  MonitorArrangement monitorArrangement;

  /* Misc. */
  JCheckBox sharedCheckbox;
  JCheckBox alwaysCursorCheckbox;
  JCheckBox acceptBellCheckbox;

  JComboBox cursorTypeChoice;

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
    setTitle("TigerVNC options");
    setResizable(false);

    getContentPane().setLayout(new BorderLayout());

    encodingGroup = new ButtonGroup();
    colorlevelGroup = new ButtonGroup();

    String[] navItems = { "Compression", "Security", "Input", "Display", "Miscellaneous", "SSH" };
    final JList<String> navList = new JList<String>(navItems);
    navList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
    navList.setSelectedIndex(0);
    navList.setBackground(Color.WHITE);
    navList.setFixedCellHeight(36);
    navList.setPreferredSize(new Dimension(150, 370));
    navList.setBorder(BorderFactory.createMatteBorder(0, 0, 0, 1, new Color(180, 180, 180)));

    navList.setCellRenderer(new DefaultListCellRenderer() {
      @Override
      public Component getListCellRendererComponent(JList<?> list, Object value,
                                                    int index, boolean isSelected,
                                                    boolean cellHasFocus) {
        JLabel label = (JLabel) super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
        label.setOpaque(true);
        label.setHorizontalAlignment(SwingConstants.CENTER);
        label.setBorder(BorderFactory.createEmptyBorder(6, 16, 6, 16));
        if (isSelected) {
          label.setBackground(new Color(0x33, 0x85, 0xFF)); // FLTK blue
          label.setForeground(Color.WHITE);
        } else {
          label.setBackground(Color.WHITE);
          label.setForeground(new Color(0x33, 0x33, 0x33));
        }
        return label;
      }
    });

    final CardLayout cardLayout = new CardLayout();
    final JPanel cardPanel = new JPanel(cardLayout);
    cardPanel.add(createCompressionPanel(), "Compression");
    cardPanel.add(createSecurityPanel(), "Security");
    cardPanel.add(createInputPanel(), "Input");
    cardPanel.add(createScreenPanel(), "Display");
    cardPanel.add(createMiscPanel(), "Miscellaneous");
    cardPanel.add(createSshPanel(), "SSH");

    navList.addListSelectionListener(e -> {
      if (!e.getValueIsAdjusting()) {
        String sel = navList.getSelectedValue();
        if (sel != null) {
          cardLayout.show(cardPanel, sel);
        }
      }
    });

    JPanel centerPane = new JPanel(new BorderLayout());
    centerPane.add(navList, BorderLayout.WEST);
    centerPane.add(cardPanel, BorderLayout.CENTER);

    // button pane
    Dimension buttonSize = new Dimension(115, 27);

    JButton okButton = new JButton("OK");
    okButton.setPreferredSize(buttonSize);
    okButton.setIcon(new ReturnArrowIcon());
    okButton.setHorizontalTextPosition(SwingConstants.LEFT);
    okButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        storeOptions();
        endDialog();
      }
    });
    getRootPane().setDefaultButton(okButton);

    JButton cancelButton = new JButton("Cancel");
    cancelButton.setPreferredSize(buttonSize);
    cancelButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        endDialog();
      }
    });

    JPanel buttonPane = new JPanel(new FlowLayout(FlowLayout.RIGHT, 10, 8));
    buttonPane.setBorder(BorderFactory.createCompoundBorder(
      BorderFactory.createMatteBorder(1, 0, 0, 0, new Color(180, 180, 180)),
      BorderFactory.createEmptyBorder(4, 10, 6, 10)
    ));
    buttonPane.add(cancelButton);
    buttonPane.add(okButton);

    this.add(centerPane, BorderLayout.CENTER);
    this.add(buttonPane, BorderLayout.SOUTH);
    getContentPane().setPreferredSize(new Dimension(580, 480));
    addListeners(this);
    pack();
  }

  private static Border createSectionBorder(String title) {
    TitledBorder titledBorder = BorderFactory.createTitledBorder(BorderFactory.createEmptyBorder(), title);
    Font f = titledBorder.getTitleFont();
    if (f != null) {
      titledBorder.setTitleFont(f.deriveFont(f.getSize2D() + 1.5f));
    }
    return BorderFactory.createCompoundBorder(
      titledBorder,
      BorderFactory.createEmptyBorder(2, 0, 2, 0)
    );
  }

  private static class ReturnArrowIcon implements Icon {
    private final int width = 12;
    private final int height = 12;

    public void paintIcon(Component c, Graphics g, int x, int y) {
      Graphics2D g2 = (Graphics2D) g.create();
      g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
      g2.setColor(c.getForeground());

      int arrowX = x + 1;
      int arrowY = y + 6;

      Polygon head = new Polygon();
      head.addPoint(arrowX, arrowY);
      head.addPoint(arrowX + 4, arrowY - 3);
      head.addPoint(arrowX + 4, arrowY + 3);
      g2.fill(head);

      g2.setStroke(new BasicStroke(1.5f));
      g2.drawLine(arrowX + 3, arrowY, arrowX + 8, arrowY);
      g2.drawLine(arrowX + 8, arrowY, arrowX + 8, arrowY - 4);

      g2.dispose();
    }

    public int getIconWidth() { return width; }
    public int getIconHeight() { return height; }
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
    handleAlwaysCursor();

    /* Security */
    Security security = new Security(SecurityClient.secTypes);

    List<Integer> secTypes;
    Iterator<Integer> iter;

    List<Integer> secTypesExt;
    Iterator<Integer> iterExt;

    encNoneCheckbox.setSelected(false);
    encTLSCheckbox.setSelected(false);
    encX509Checkbox.setSelected(false);
    encRSAAESCheckbox.setSelected(false);

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
      case Security.secTypeRA2:
      case Security.secTypeRA256:
        encRSAAESCheckbox.setSelected(true);
      case Security.secTypeRA2ne:
      case Security.secTypeRAne256:
        authVncCheckbox.setSelected(true);
        authPlainCheckbox.setSelected(true);
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
    disableArrowScrollCheckbox.setSelected(disableArrowScroll.getValue());

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
    String modeStr = fullScreenMode.getValueStr().toLowerCase(Locale.ENGLISH);
    if (!fullScreen.getValue()) {
      modeWindowedButton.setSelected(true);
    } else if (modeStr.equals("current") || (!fullScreenAllMonitors.getValue() && modeStr.equals("all"))) {
      modeCurrentMonitorButton.setSelected(true);
    } else if (modeStr.equals("selected")) {
      modeSelectedMonitorsButton.setSelected(true);
    } else {
      modeAllMonitorsButton.setSelected(true);
    }

    Set<Integer> selectedIndices = new TreeSet<Integer>();
    String selStr = fullScreenSelectedMonitors.getValueStr();
    if (selStr != null && !selStr.trim().isEmpty()) {
      for (String part : selStr.split(",")) {
        try {
          selectedIndices.add(Integer.parseInt(part.trim()) - 1);
        } catch (NumberFormatException ignored) {}
      }
    }
    monitorArrangement.setSelectedMonitors(selectedIndices);
    monitorArrangement.setEnabled(modeSelectedMonitorsButton.isSelected());

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
    alwaysCursorCheckbox.setSelected(alwaysCursor.getValue());
    String cursorTypeStr = cursorType.getValueStr();
    cursorTypeChoice.setSelectedItem(cursorTypeStr);
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
      if (authVncCheckbox.isSelected()) {
        security.EnableSecType(Security.secTypeVncAuth);
        security.EnableSecType(Security.secTypeRA2ne);
        security.EnableSecType(Security.secTypeRAne256);
      }
      if (authPlainCheckbox.isSelected()) {
        security.EnableSecType(Security.secTypePlain);
        security.EnableSecType(Security.secTypeRA2ne);
        security.EnableSecType(Security.secTypeRAne256);
      }
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

    if (encRSAAESCheckbox.isSelected()) {
      security.EnableSecType(Security.secTypeRA2);
      security.EnableSecType(Security.secTypeRA256);
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
    disableArrowScroll.setParam(disableArrowScrollCheckbox.isSelected());

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
    if (modeWindowedButton.isSelected()) {
      fullScreen.setParam(false);
      fullScreenMode.setParam("windowed");
      fullScreenAllMonitors.setParam(false);
    } else {
      fullScreen.setParam(true);
      if (modeCurrentMonitorButton.isSelected()) {
        fullScreenMode.setParam("current");
        fullScreenAllMonitors.setParam(false);
      } else if (modeSelectedMonitorsButton.isSelected()) {
        fullScreenMode.setParam("selected");
        fullScreenAllMonitors.setParam(false);
      } else {
        fullScreenMode.setParam("all");
        fullScreenAllMonitors.setParam(true);
      }
    }

    Set<Integer> selSet = monitorArrangement.getSelectedMonitors();
    StringBuilder sb = new StringBuilder();
    for (int idx : selSet) {
      if (sb.length() > 0) sb.append(",");
      sb.append(idx + 1);
    }
    fullScreenSelectedMonitors.setParam(sb.toString());

    String scaleStr =
      ((String)scalingFactorInput.getSelectedItem()).replace("%", "");
    scaleStr = scaleStr.replace("Fixed Aspect Ratio", "FixedRatio");
    scalingFactor.setParam(scaleStr);

    /* Misc. */
    shared.setParam(sharedCheckbox.isSelected());
    alwaysCursor.setParam(alwaysCursorCheckbox.isSelected());
    acceptBell.setParam(acceptBellCheckbox.isSelected());
    cursorType.setParam((String)cursorTypeChoice.getSelectedItem());

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
    encodingPanel.setBorder(createSectionBorder("Preferred encoding"));
    tightButton = new GroupedJRadioButton("Tight", encodingGroup, encodingPanel);
    zrleButton = new GroupedJRadioButton("ZRLE", encodingGroup, encodingPanel);
    hextileButton = new GroupedJRadioButton("Hextile", encodingGroup, encodingPanel);
    rawButton = new GroupedJRadioButton("Raw", encodingGroup, encodingPanel);

    JPanel colorPanel = new JPanel(new GridLayout(4, 1));
    colorPanel.setBorder(createSectionBorder("Color level"));
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
    JPanel SecPanel = new JPanel(new GridBagLayout());
    SecPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel encrPanel = new JPanel(new GridBagLayout());
    encrPanel.setBorder(createSectionBorder("Encryption"));
    encNoneCheckbox = new JCheckBox("None");
    encTLSCheckbox = new JCheckBox("Anonymous TLS");
    encX509Checkbox = new JCheckBox("TLS with X.509 certificates");
    encX509Checkbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleX509();
      }
    });
    JLabel caLabel = new JLabel("Path to X509 CA certificate");
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
    JLabel crlLabel = new JLabel("Path to X509 CRL file");
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
    encRSAAESCheckbox = new JCheckBox("RSA-AES");
    encRSAAESCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleRSAAES();
      }
    });
    encrPanel.add(encNoneCheckbox,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    encrPanel.add(encTLSCheckbox,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    encrPanel.add(encX509Checkbox,
                  new GridBagConstraints(0, 2,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    encrPanel.add(caLabel,
                  new GridBagConstraints(0, 3,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 20, 2, 0),
                                         0, 0));
    encrPanel.add(caInput,
                  new GridBagConstraints(0, 4,
                                         2, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 20, 4, 5),
                                         0, 0));
    encrPanel.add(caChooser,
                  new GridBagConstraints(2, 4,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         0, 0));
    encrPanel.add(crlLabel,
                  new GridBagConstraints(0, 5,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 20, 2, 0),
                                         0, 0));
    encrPanel.add(crlInput,
                  new GridBagConstraints(0, 6,
                                         2, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, HORIZONTAL,
                                         new Insets(0, 20, 4, 5),
                                         0, 0));
    encrPanel.add(crlChooser,
                  new GridBagConstraints(2, 6,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         0, 0));
    encrPanel.add(encRSAAESCheckbox,
                  new GridBagConstraints(0, 7,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));

    JPanel authPanel = new JPanel(new GridBagLayout());
    authPanel.setBorder(createSectionBorder("Authentication"));

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
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(authVncCheckbox,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(authPlainCheckbox,
                  new GridBagConstraints(0, 2,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(authIdentCheckbox,
                  new GridBagConstraints(0, 3,
                                         1, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 0, 2, 0),
                                         NONE, NONE));
    authPanel.add(sendLocalUsernameCheckbox,
                  new GridBagConstraints(1, 2,
                                         REMAINDER, 2,
                                         HEAVY, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(2, 20, 2, 0),
                                         NONE, NONE));

    SecPanel.add(encrPanel,
                 new GridBagConstraints(0, 0,
                                        REMAINDER, 1,
                                        HEAVY, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    SecPanel.add(authPanel,
                 new GridBagConstraints(0, 1,
                                        REMAINDER, 1,
                                        HEAVY, LIGHT,
                                        LINE_START, HORIZONTAL,
                                        new Insets(0, 0, 4, 0),
                                        NONE, NONE));
    SecPanel.add(Box.createRigidArea(new Dimension(0, 0)),
                 new GridBagConstraints(0, 2,
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

    JPanel mousePanel = new JPanel(new GridBagLayout());
    mousePanel.setBorder(createSectionBorder("Mouse"));

    alwaysCursorCheckbox = new JCheckBox("Show local cursor when not provided by server");
    alwaysCursorCheckbox.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        handleAlwaysCursor();
      }
    });
    JLabel cursorTypeLabel = new JLabel("Cursor type:");
    String[] cursorTypes = {"Dot", "System"};
    cursorTypeChoice = new MyJComboBox(cursorTypes);
    cursorTypeChoice.setPrototypeDisplayValue("System.");

    mousePanel.add(alwaysCursorCheckbox,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(2, 0, 2, 0),
                                          NONE, NONE));
    mousePanel.add(cursorTypeLabel,
                   new GridBagConstraints(0, 1,
                                          1, 1,
                                          LIGHT, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(2, 4, 2, 0),
                                          NONE, NONE));
    mousePanel.add(cursorTypeChoice,
                   new GridBagConstraints(1, 1,
                                          1, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(2, 5, 2, 0),
                                          NONE, NONE));

    JPanel keyboardPanel = new JPanel(new GridBagLayout());
    keyboardPanel.setBorder(createSectionBorder("Keyboard"));

    disableArrowScrollCheckbox = new JCheckBox("Disable arrow-key scrolling when scrollbars visible");
    JLabel menuKeyLabel = new JLabel("Menu key");
    String[] menuKeys = new String[MenuKey.getMenuKeySymbolCount()];
    for (int i = 0; i < MenuKey.getMenuKeySymbolCount(); i++)
      menuKeys[i] = MenuKey.getKeyText(MenuKey.getMenuKeySymbols()[i]);
    menuKeyChoice = new JComboBox(menuKeys);

    keyboardPanel.add(disableArrowScrollCheckbox,
                      new GridBagConstraints(0, 0,
                                             REMAINDER, 1,
                                             HEAVY, LIGHT,
                                             LINE_START, NONE,
                                             new Insets(2, 0, 2, 0),
                                             NONE, NONE));
    keyboardPanel.add(menuKeyLabel,
                      new GridBagConstraints(0, 1,
                                             1, 1,
                                             LIGHT, LIGHT,
                                             LINE_START, NONE,
                                             new Insets(4, 4, 2, 0),
                                             NONE, NONE));
    keyboardPanel.add(menuKeyChoice,
                      new GridBagConstraints(1, 1,
                                             1, 1,
                                             HEAVY, LIGHT,
                                             LINE_START, NONE,
                                             new Insets(4, 5, 2, 0),
                                             NONE, NONE));

    JPanel clipboardPanel = new JPanel(new GridBagLayout());
    clipboardPanel.setBorder(createSectionBorder("Clipboard"));

    acceptClipboardCheckbox = new JCheckBox("Accept clipboard from server");
    sendClipboardCheckbox = new JCheckBox("Send clipboard to server");

    clipboardPanel.add(acceptClipboardCheckbox,
                       new GridBagConstraints(0, 0,
                                              REMAINDER, 1,
                                              HEAVY, LIGHT,
                                              LINE_START, NONE,
                                              new Insets(2, 0, 2, 0),
                                              NONE, NONE));
    clipboardPanel.add(sendClipboardCheckbox,
                       new GridBagConstraints(0, 1,
                                              REMAINDER, 1,
                                              HEAVY, LIGHT,
                                              LINE_START, NONE,
                                              new Insets(2, 0, 2, 0),
                                              NONE, NONE));

    inputPanel.add(viewOnlyCheckbox,
                   new GridBagConstraints(0, 0,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, NONE,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(mousePanel,
                   new GridBagConstraints(0, 1,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, HORIZONTAL,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(keyboardPanel,
                   new GridBagConstraints(0, 2,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, HORIZONTAL,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(clipboardPanel,
                   new GridBagConstraints(0, 3,
                                          REMAINDER, 1,
                                          HEAVY, LIGHT,
                                          LINE_START, HORIZONTAL,
                                          new Insets(0, 0, 4, 0),
                                          NONE, NONE));
    inputPanel.add(Box.createRigidArea(new Dimension(0, 0)),
                   new GridBagConstraints(0, 4,
                                          REMAINDER, REMAINDER,
                                          HEAVY, HEAVY,
                                          LINE_START, BOTH,
                                          new Insets(0, 0, 0, 0),
                                          NONE, NONE));
    return inputPanel;
  }

  private JPanel createScreenPanel() {
    int indent;
    JPanel ScreenPanel = new JPanel(new GridBagLayout());
    ScreenPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 0, 5));

    JPanel SizingPanel = new JPanel(new GridBagLayout());
    SizingPanel.setBorder(createSectionBorder("Desktop Sizing"));
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
    JPanel displayModePanel = new JPanel(new GridBagLayout());
    displayModePanel.setBorder(createSectionBorder("Display Mode"));

    displayModeGroup = new ButtonGroup();
    modeWindowedButton = new JRadioButton("Windowed");
    modeCurrentMonitorButton = new JRadioButton("Full screen on current monitor");
    modeAllMonitorsButton = new JRadioButton("Full screen on all monitors");
    modeSelectedMonitorsButton = new JRadioButton("Full screen on selected monitor(s)");

    displayModeGroup.add(modeWindowedButton);
    displayModeGroup.add(modeCurrentMonitorButton);
    displayModeGroup.add(modeAllMonitorsButton);
    displayModeGroup.add(modeSelectedMonitorsButton);

    monitorArrangement = new MonitorArrangement();

    ItemListener modeListener = new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        monitorArrangement.setEnabled(modeSelectedMonitorsButton.isSelected());
      }
    };
    modeWindowedButton.addItemListener(modeListener);
    modeCurrentMonitorButton.addItemListener(modeListener);
    modeAllMonitorsButton.addItemListener(modeListener);
    modeSelectedMonitorsButton.addItemListener(modeListener);

    displayModePanel.add(modeWindowedButton,
                         new GridBagConstraints(0, 0,
                                                REMAINDER, 1,
                                                LIGHT, LIGHT,
                                                LINE_START, NONE,
                                                new Insets(2, 0, 2, 0),
                                                NONE, NONE));
    displayModePanel.add(modeCurrentMonitorButton,
                         new GridBagConstraints(0, 1,
                                                REMAINDER, 1,
                                                LIGHT, LIGHT,
                                                LINE_START, NONE,
                                                new Insets(2, 0, 2, 0),
                                                NONE, NONE));
    displayModePanel.add(modeAllMonitorsButton,
                         new GridBagConstraints(0, 2,
                                                REMAINDER, 1,
                                                LIGHT, LIGHT,
                                                LINE_START, NONE,
                                                new Insets(2, 0, 2, 0),
                                                NONE, NONE));
    displayModePanel.add(modeSelectedMonitorsButton,
                         new GridBagConstraints(0, 3,
                                                REMAINDER, 1,
                                                LIGHT, LIGHT,
                                                LINE_START, NONE,
                                                new Insets(2, 0, 2, 0),
                                                NONE, NONE));

    indent = getButtonLabelInset(modeSelectedMonitorsButton);
    displayModePanel.add(monitorArrangement,
                         new GridBagConstraints(0, 4,
                                                REMAINDER, 1,
                                                HEAVY, LIGHT,
                                                LINE_START, BOTH,
                                                new Insets(2, indent, 4, 0),
                                                NONE, NONE));

    SizingPanel.add(desktopSizeCheckbox,
                    new GridBagConstraints(0, 0,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, NONE,
                                           new Insets(0, 0, 0, 0),
                                           NONE, NONE));
    indent = getButtonLabelInset(desktopSizeCheckbox);
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
    ScreenPanel.add(displayModePanel,
                    new GridBagConstraints(0, 1,
                                           REMAINDER, 1,
                                           LIGHT, LIGHT,
                                           LINE_START, HORIZONTAL,
                                           new Insets(0, 0, 4, 0),
                                           NONE, NONE));
    ScreenPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                    new GridBagConstraints(0, 2,
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
    acceptBellCheckbox = new JCheckBox("Beep when requested by the server");
    MiscPanel.add(sharedCheckbox,
                  new GridBagConstraints(0, 0,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(acceptBellCheckbox,
                  new GridBagConstraints(0, 1,
                                         REMAINDER, 1,
                                         LIGHT, LIGHT,
                                         LINE_START, NONE,
                                         new Insets(0, 0, 4, 0),
                                         NONE, NONE));
    MiscPanel.add(Box.createRigidArea(new Dimension(5, 0)),
                  new GridBagConstraints(0, 2,
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
    opensshPanel.setBorder(createSectionBorder("Embedded SSH client configuration"));
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

  private void handleRSAAES()
  {
    authVncCheckbox.setSelected(true);
    authPlainCheckbox.setSelected(true);
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

  private void handleAlwaysCursor()
  {
    cursorTypeChoice.setEnabled(alwaysCursorCheckbox.isSelected());
  }

  static LogWriter vlog = new LogWriter("OptionsDialog");
}
