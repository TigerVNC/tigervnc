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
import java.awt.event.*;
import java.io.File;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.filechooser.*;
import javax.swing.text.*;

class Dialog extends JDialog implements ActionListener,
                                        ItemListener,
                                        KeyListener {

  // GridBag weights
  static double HEAVY = 1.0;
  static double LIGHT = 0.0;

  public Dialog(boolean modal) {
    setIconImage(VncViewer.frameIcon);
    if (modal) {
      setModalityType(ModalityType.APPLICATION_MODAL);
    } else {
      setModalityType(ModalityType.MODELESS);
    }
  }

  public void showDialog(Component c) {
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

    if (getModalityType() == ModalityType.APPLICATION_MODAL)
      setAlwaysOnTop(true);
    setVisible(true);
  }

  public void showDialog() {
    showDialog(null);
  }

  public void endDialog() {
    setVisible(false);
    setAlwaysOnTop(false);
  }

  // initDialog() can be overridden in a derived class.  Typically it is used
  // to make sure that checkboxes have the right state, etc.
  public void initDialog() { }

  public void actionPerformed(ActionEvent e) { }
  public void itemStateChanged(ItemEvent e) { }
  public void keyTyped(KeyEvent event) { }
  public void keyReleased(KeyEvent event) { }
  public void keyPressed(KeyEvent event) { }

  protected void addListeners(Container c) {
    for (Component ch : c.getComponents()) {
      if (ch instanceof JCheckBox)
        ((JCheckBox)ch).addItemListener(this);
      else if (ch instanceof JRadioButton)
        ((JRadioButton)ch).addActionListener(this);
      else if (ch instanceof JButton)
        ((JButton)ch).addActionListener(this);
      else if (ch instanceof JComboBox)
        ((JComboBox)ch).addActionListener(this);
      else if (ch instanceof JTextField)
        ((JTextField)ch).addKeyListener(this);
      else if (ch instanceof Container)
        addListeners((Container)ch);
    }
  }

  public int getButtonLabelInset(AbstractButton b) {
    // Aligning components vertically to the label of
    // a JCheckbox is absurdly difficult.  JCheckBox's
    // getIcon() method generally returns null, so we 
    // have to resort to querying the UIManager in 
    // order to determine the width of the checkbox.
    // The default values are based on Nimbus.
    int width = 18;
    int gap = 4;

    Icon ico = b.getIcon();
    if (ico == null) {
      if (b instanceof JCheckBox)
        ico = (Icon)UIManager.get("CheckBox.icon");
      else if (b instanceof JRadioButton)
        ico = (Icon)UIManager.get("RadioButton.icon");
    }
    if (ico != null)
      width = Math.max(width, ico.getIconWidth());
    if (b != null)
      gap = Math.max(gap, b.getIconTextGap());

    return width + gap;
  }

  public static File showChooser(String title, File defFile,
                                 Container c, FileNameExtensionFilter f) {
    JFileChooser fc = new JFileChooser(defFile);
    fc.setDialogTitle(title);
    fc.setApproveButtonText("OK  \u21B5");
    fc.setFileHidingEnabled(false);
    if (f != null)
      fc.setFileFilter(f);
    if (fc.showOpenDialog(c) == JFileChooser.APPROVE_OPTION)
      return fc.getSelectedFile();
    else
      return null;
  }

  public static File showChooser(String title, File defFile, Container c) {
    return showChooser(title, defFile, c, null);
  }

  protected File showChooser(String title, File defFile,
                             FileNameExtensionFilter f) {
    return showChooser(title, defFile, this, f);
  }

  protected File showChooser(String title, File defFile) {
    return showChooser(title, defFile, this);
  }

  protected class GroupedJRadioButton extends JRadioButton {
    public GroupedJRadioButton(String l, ButtonGroup g, JComponent c) {
      super(l);
      c.add(this);
      if (g != null)
        g.add(this);
    }
  }

  protected class MyJComboBox extends JComboBox {
    public MyJComboBox(Object[] items) {
      super(items);
      // Hack to set the left inset on editable JComboBox
      if (UIManager.getLookAndFeel().getID().equals("Windows")) {
        this.setBorder(BorderFactory.createCompoundBorder(this.getBorder(),
          BorderFactory.createEmptyBorder(0,1,0,0)));
      } else if (UIManager.getLookAndFeel().getID().equals("Metal")) {
        ComboBoxEditor editor = this.getEditor();
        JTextField jtf = (JTextField)editor.getEditorComponent();
        jtf.setBorder(new CompoundBorder(jtf.getBorder(), new EmptyBorder(0,2,0,0)));
      }
    }

    public MyJComboBox() {
      new MyJComboBox(null);
    }

    @Override
    public void setPrototypeDisplayValue(Object prototypeDisplayValue) {
      // Even with setPrototypeDisplayValue set JComboxBox resizes 
      // itself when setEditable(true) is called.
      super.setPrototypeDisplayValue(prototypeDisplayValue);
      boolean e = isEditable();
      setEditable(false);
      Dimension d = getPreferredSize();
      setPreferredSize(d);
      setEditable(e);
    }

    public void setDocument(PlainDocument doc) {
      ComboBoxEditor editor = this.getEditor();
      JTextField jtf = (JTextField)editor.getEditorComponent();
      jtf.setDocument(doc);
    }

  }

  private Window fullScreenWindow;

}
