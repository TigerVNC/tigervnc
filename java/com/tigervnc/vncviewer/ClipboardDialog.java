/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2014 Brian P. Hinz
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
import java.awt.datatransfer.*;
import java.io.*;
import java.nio.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.text.*;

import com.tigervnc.rfb.LogWriter;

class ClipboardDialog extends Dialog implements ActionListener {

  private class VncTransferHandler extends TransferHandler {
    // Custom TransferHandler designed to limit the size of outbound
    // clipboard transfers to VncViewer.maxCutText.getValue() bytes.
    private LogWriter vlog = new LogWriter("VncTransferHandler");

    public boolean importData(JComponent c, Transferable t) {
      if (canImport(c, t.getTransferDataFlavors())) {
        try {
          DataFlavor VncFlavor = null;
          for (DataFlavor f : t.getTransferDataFlavors())
            if (f.isFlavorTextType() && f.isRepresentationClassInputStream())
              VncFlavor = f;
          if (VncFlavor == null) return false;
          Reader reader = (Reader)VncFlavor.getReaderForText(t);
          CharBuffer cbuf =
            CharBuffer.allocate(VncViewer.maxCutText.getValue());
          cbuf.limit(reader.read(cbuf.array(), 0, cbuf.length()));
          reader.close();
          if (c instanceof JTextComponent)
            ((JTextComponent)c).setText(cbuf.toString());
          return true;
        } catch (OutOfMemoryError oome) {
          vlog.error("ERROR: Too much data on local clipboard!");
        } catch (UnsupportedFlavorException ufe) {
          // Skip import
          vlog.info(ufe.toString());
        } catch (IOException ioe) {
          // Skip import
          vlog.info(ioe.toString());
        }
      }
      return false;
    }

    public boolean canImport(JComponent c, DataFlavor[] flavors) {
      for (DataFlavor f : flavors)
        if (f.isFlavorTextType() && f.isRepresentationClassReader())
          return true;
      return false;
    }
  }

  public ClipboardDialog(CConn cc_) {
    super(false);
    setTitle("VNC Clipboard Viewer");
    setPreferredSize(new Dimension(640, 480));
    addWindowFocusListener(new WindowAdapter() {
      // Necessary to ensure that updates from the system clipboard
      // still occur when the ClipboardDialog has the focus.
      public void WindowGainedFocus(WindowEvent e) {
        clientCutText();
      }
    });
    cc = cc_;
    textArea = new JTextArea();
    textArea.setTransferHandler(new VncTransferHandler());
    // If the textArea can receive the focus, then text within the textArea
    // can be selected.  On platforms that don't support separate selection
    // and clipboard buffers, this triggers a replacement of the textAra's
    // contents with the selected text.
    textArea.setFocusable(false);
    textArea.setLineWrap(false);
    textArea.setWrapStyleWord(true);
    JScrollPane sp = new JScrollPane(textArea);
    getContentPane().add(sp, BorderLayout.CENTER);
    // button panel placed below the scrollpane
    JPanel pb = new JPanel();
    clearButton = new JButton("Clear");
    pb.add(clearButton);
    clearButton.addActionListener(this);
    sendButton = new JButton("Send to VNC server");
    pb.add(sendButton);
    sendButton.addActionListener(this);
    cancelButton = new JButton("Cancel");
    pb.add(cancelButton);
    cancelButton.addActionListener(this);
    getContentPane().add("South", pb);
    pack();
  }

  public void serverCutText(String str, int len) {
    textArea.setText(str);
    textArea.selectAll();
    textArea.copy();
  }

  public void clientCutText() {
    int hc = textArea.getText().hashCode();
    textArea.setText("");
    textArea.paste();
    textArea.setCaretPosition(0);
    String text = textArea.getText();
    if (cc.viewer.sendClipboard.getValue())
      if (hc != text.hashCode())
        cc.writeClientCutText(text, text.length());
  }

  public void setSendingEnabled(boolean b) {
    sendButton.setEnabled(b);
  }

  public void actionPerformed(ActionEvent e) {
    Object s = e.getSource();
    if (s instanceof JButton && (JButton)s == clearButton) {
      serverCutText(new String(""), 0);
    } else if (s instanceof JButton && (JButton)s == sendButton) {
      String text = textArea.getText();
      if (cc.viewer.sendClipboard.getValue())
        cc.writeClientCutText(text, text.length());
      endDialog();
    } else if (s instanceof JButton && (JButton)s == cancelButton) {
      endDialog();
    }
  }

  CConn cc;
  JTextArea textArea;
  JButton clearButton, sendButton, cancelButton;
  static LogWriter vlog = new LogWriter("ClipboardDialog");
}
