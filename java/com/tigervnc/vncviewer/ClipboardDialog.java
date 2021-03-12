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
import javax.swing.event.*;
import javax.swing.text.*;

import com.tigervnc.rfb.LogWriter;

import static com.tigervnc.vncviewer.Parameters.*;

class ClipboardDialog extends Dialog {

  protected static class MyJTextArea extends JTextArea {

	  private class VncTransferHandler extends TransferHandler {
	    // Custom TransferHandler designed to limit the size of outbound
	    // clipboard transfers to VncViewer.maxCutText.getValue() bytes.
	    private LogWriter vlog = new LogWriter("VncTransferHandler");
      private long start;

	    public void exportToClipboard(JComponent comp, Clipboard clip, int action)
	        throws IllegalStateException {
        if (action != TransferHandler.COPY) return;
        if (start == 0)
          start = System.currentTimeMillis();
	      StringSelection selection = new StringSelection(((JTextArea)comp).getText());
        try {
	        clip.setContents(selection, selection);
        } catch(IllegalStateException e) {
          // something else has the clipboard, keep trying for at most 50ms
          if ((System.currentTimeMillis() - start) < 50)
            exportToClipboard(comp, clip, action);
          else
            vlog.info("Couldn't access system clipboard for > 50ms");
        }
        start = 0;
	    }

	    public boolean importData(JComponent c, Transferable t) {
	      if (canImport(c, t.getTransferDataFlavors())) {
	        try {
	          DataFlavor VncFlavor = null;
	          for (DataFlavor f : t.getTransferDataFlavors()) {
	            if (f.isMimeTypeEqual("text/plain") &&
                  f.isRepresentationClassInputStream()) {
	              VncFlavor = f;
                break;
              }
            }
	          if (VncFlavor == null) return false;
	          CharBuffer cbuf =
	            CharBuffer.allocate(maxCutText.getValue());
	          Reader reader = (Reader)VncFlavor.getReaderForText(t);
            int n = reader.read(cbuf.array(), 0, cbuf.length());
	          reader.close();
            // reader returns -1 (EOF) for empty clipboard
            cbuf.limit(n < 0 ? 0 : n);
	          if (c instanceof JTextComponent)
              if (!cbuf.toString().equals(((JTextComponent)c).getText()))
	              ((JTextComponent)c).setText(cbuf.toString());
	          return true;
	        } catch (OutOfMemoryError e) {
	          vlog.error("ERROR: Too much data on local clipboard!");
	        } catch (UnsupportedFlavorException e) {
	          // Skip import
	          vlog.info(e.toString());
	        } catch (IOException e) {
	          // Skip import
	          vlog.info(e.toString());
	        }
	      }
	      return false;
	    }

	    public boolean canImport(JComponent c, DataFlavor[] flavors) {
	      for (DataFlavor f : flavors)
	        if (f.isMimeTypeEqual("text/plain") &&
              f.isRepresentationClassReader())
	          return true;
	      return false;
	    }
	  }

	  private class MyTextListener implements DocumentListener {
	    public MyTextListener() { }

	    public void changedUpdate(DocumentEvent e) { } 

	    public void insertUpdate(DocumentEvent e) {
        if (!listen) return;
	      String text = textArea.getText();
	      if (sendClipboard.getValue())
	        VncViewer.cc.writeClientCutText(text, text.length());
	    }

	    public void removeUpdate(DocumentEvent e) { }
	  }

    public MyJTextArea() {
      super();
      setTransferHandler(new VncTransferHandler());
      getDocument().addDocumentListener(new MyTextListener());
      // If the textArea can receive the focus, then text within the textArea
      // can be selected.  On platforms that don't support separate selection
      // and clipboard buffers, this triggers a replacement of the textAra's
      // contents with the selected text.
      setFocusable(false);
      setLineWrap(false);
      setWrapStyleWord(true);
    }
  }

  public ClipboardDialog() {
    super(false);
    setTitle("VNC Clipboard Viewer");
    setPreferredSize(new Dimension(640, 480));
    addWindowFocusListener(new WindowFocusListener() {
      // Necessary to ensure that updates from the system clipboard
      // are propagated to the textArea when the dialog is visible.
      public void windowGainedFocus(WindowEvent e) {
        clientCutText();
      }
      public void windowLostFocus(WindowEvent e) { }
    });
    JScrollPane sp = new JScrollPane(textArea);
    getContentPane().add(sp, BorderLayout.CENTER);
    // button panel placed below the scrollpane
    JPanel pb = new JPanel();
    clearButton = new JButton("Clear");
    pb.add(clearButton);
    sendButton = new JButton("Send to VNC server");
    pb.add(sendButton);
    cancelButton = new JButton("Cancel");
    pb.add(cancelButton);
    getContentPane().add("South", pb);
    addListeners(this);
    pack();
  }

  public static void showDialog(Container c) {
    if (dialog == null)
      dialog = new ClipboardDialog();
    dialog.show(c);
  }

  public void show(Container c) {
    super.showDialog(c);
  }

  public void endDialog() {
    super.endDialog();
    dialog.dispose();
  }

  public static void serverCutText(String str) {
    if (textArea.getText().equals(str))
      return;
    // Update the text area with incoming serverCutText.  We need to diable
    // the DocumentListener temporarily to prevent an clientCutText msg from
    // being sent back to the server when the textArea is updated.
    listen = false;
    textArea.setText(str);
    textArea.copy();
    listen = true;
  }

  public static void clientCutText() {
    // Update the textArea with the current contents of the system clipboard.
    // The TransferHandler ensures that the textArea's contents are only 
    // changed when they differ from the clipboard's.  If the textArea is
    // updated, the DocumentListener will trigger an RFB clientCutText msg.
    textArea.paste();
    textArea.setCaretPosition(0);
  }

  public void setSendingEnabled(boolean b) {
    sendButton.setEnabled(b);
  }

  public void actionPerformed(ActionEvent e) {
    Object s = e.getSource();
    if (s instanceof JButton && (JButton)s == clearButton) {
      serverCutText(new String(""));
    } else if (s instanceof JButton && (JButton)s == sendButton) {
      String text = textArea.getText();
      VncViewer.cc.writeClientCutText(text, text.length());
      endDialog();
    } else if (s instanceof JButton && (JButton)s == cancelButton) {
      endDialog();
    }
  }

  private JButton clearButton, sendButton, cancelButton;
  private static boolean listen = true;
  static ClipboardDialog dialog;
  static MyJTextArea textArea = new MyJTextArea();
  static LogWriter vlog = new LogWriter("ClipboardDialog");
}
