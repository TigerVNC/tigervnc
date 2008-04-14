//
//  Copyright (C) 2001 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// Clipboard frame.
//

import java.awt.*;
import java.awt.event.*;

class ClipboardFrame extends Frame
  implements WindowListener, ActionListener {

  TextArea textArea;
  Button clearButton, closeButton;
  String selection;
  VncViewer viewer;

  //
  // Constructor.
  //

  ClipboardFrame(VncViewer v) {
    super("TightVNC Clipboard");

    viewer = v;

    GridBagLayout gridbag = new GridBagLayout();
    setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.fill = GridBagConstraints.BOTH;
    gbc.weighty = 1.0;

    textArea = new TextArea(5, 40);
    gridbag.setConstraints(textArea, gbc);
    add(textArea);

    gbc.fill = GridBagConstraints.HORIZONTAL;
    gbc.weightx = 1.0;
    gbc.weighty = 0.0;
    gbc.gridwidth = 1;

    clearButton = new Button("Clear");
    gridbag.setConstraints(clearButton, gbc);
    add(clearButton);
    clearButton.addActionListener(this);

    closeButton = new Button("Close");
    gridbag.setConstraints(closeButton, gbc);
    add(closeButton);
    closeButton.addActionListener(this);

    pack();

    addWindowListener(this);
  }


  //
  // Set the cut text from the RFB server.
  //

  void setCutText(String text) {
    selection = text;
    textArea.setText(text);
    if (isVisible()) {
      textArea.selectAll();
    }
  }


  //
  // When the focus leaves the window, see if we have new cut text and
  // if so send it to the RFB server.
  //

  public void windowDeactivated (WindowEvent evt) {
    if (selection != null && !selection.equals(textArea.getText())) {
      selection = textArea.getText();
      viewer.setCutText(selection);
    }
  }

  //
  // Close our window properly.
  //

  public void windowClosing(WindowEvent evt) {
    setVisible(false);
  }

  //
  // Ignore window events we're not interested in.
  //

  public void windowActivated(WindowEvent evt) {}
  public void windowOpened(WindowEvent evt) {}
  public void windowClosed(WindowEvent evt) {}
  public void windowIconified(WindowEvent evt) {}
  public void windowDeiconified(WindowEvent evt) {}


  //
  // Respond to button presses
  //

  public void actionPerformed(ActionEvent evt) {
    if (evt.getSource() == clearButton) {
      textArea.setText("");
    } else if (evt.getSource() == closeButton) {
      setVisible(false);
    }
  }
}
