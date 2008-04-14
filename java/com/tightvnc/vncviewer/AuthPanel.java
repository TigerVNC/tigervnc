//
//  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
//  Copyright (C) 2002-2006 Constantin Kaplinsky.  All Rights Reserved.
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

import java.awt.*;
import java.awt.event.*;

//
// The panel which implements the user authentication scheme
//

class AuthPanel extends Panel implements ActionListener {

  TextField passwordField;
  Button okButton;

  //
  // Constructor.
  //

  public AuthPanel(VncViewer viewer)
  {
    Label titleLabel = new Label("VNC Authentication", Label.CENTER);
    titleLabel.setFont(new Font("Helvetica", Font.BOLD, 18));

    Label promptLabel = new Label("Password:", Label.CENTER);

    passwordField = new TextField(10);
    passwordField.setForeground(Color.black);
    passwordField.setBackground(Color.white);
    passwordField.setEchoChar('*');

    okButton = new Button("OK");

    GridBagLayout gridbag = new GridBagLayout();
    GridBagConstraints gbc = new GridBagConstraints();

    setLayout(gridbag);

    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.insets = new Insets(0,0,20,0);
    gridbag.setConstraints(titleLabel,gbc);
    add(titleLabel);

    gbc.fill = GridBagConstraints.NONE;
    gbc.gridwidth = 1;
    gbc.insets = new Insets(0,0,0,0);
    gridbag.setConstraints(promptLabel,gbc);
    add(promptLabel);

    gridbag.setConstraints(passwordField,gbc);
    add(passwordField);
    passwordField.addActionListener(this);

    // gbc.ipady = 10;
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.fill = GridBagConstraints.BOTH;
    gbc.insets = new Insets(0,20,0,0);
    gbc.ipadx = 30;
    gridbag.setConstraints(okButton,gbc);
    add(okButton);
    okButton.addActionListener(this);
  }

  //
  // Move keyboard focus to the default object, that is, the password
  // text field.
  //

  public void moveFocusToDefaultField()
  {
    passwordField.requestFocus();
  }

  //
  // This method is called when a button is pressed or return is
  // pressed in the password text field.
  //

  public synchronized void actionPerformed(ActionEvent evt)
  {
    if (evt.getSource() == passwordField || evt.getSource() == okButton) {
      passwordField.setEnabled(false);
      notify();
    }
  }

  //
  // Wait for user entering a password, and return it as String.
  //

  public synchronized String getPassword() throws Exception
  {
    try {
      wait();
    } catch (InterruptedException e) { }
    return passwordField.getText();
  }

}
