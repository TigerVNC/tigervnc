//
//  Copyright (C) 2001 HorizonLive.com, Inc.  All Rights Reserved.
//  Copyright (C) 2001 Constantin Kaplinsky.  All Rights Reserved.
//  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
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
// Options frame.
//
// This deals with all the options the user can play with.
// It sets the encodings array and some booleans.
//

import java.awt.*;
import java.awt.event.*;

class OptionsFrame extends Frame
  implements WindowListener, ActionListener, ItemListener {

  static String[] names = {
    "Encoding",
    "Compression level",
    "JPEG image quality",
    "Cursor shape updates",
    "Use CopyRect",
    "Restricted colors",
    "Mouse buttons 2 and 3",
    "View only",
    "Share desktop",
  };

  static String[][] values = {
    { "Raw", "RRE", "CoRRE", "Hextile", "Zlib", "Tight" },
    { "Default", "1", "2", "3", "4", "5", "6", "7", "8", "9" },
    { "JPEG off", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" },
    { "Enable", "Ignore", "Disable" },
    { "Yes", "No" },
    { "Yes", "No" },
    { "Normal", "Reversed" },
    { "Yes", "No" },
    { "Yes", "No" },
  };

  final int
    encodingIndex        = 0,
    compressLevelIndex   = 1,
    jpegQualityIndex     = 2,
    cursorUpdatesIndex   = 3,
    useCopyRectIndex     = 4,
    eightBitColorsIndex  = 5,
    mouseButtonIndex     = 6,
    viewOnlyIndex        = 7,
    shareDesktopIndex    = 8;

  Label[] labels = new Label[names.length];
  Choice[] choices = new Choice[names.length];
  Button closeButton;
  VncViewer viewer;


  //
  // The actual data which other classes look at:
  //

  int[] encodings = new int[20];
  int nEncodings;

  int compressLevel;
  int jpegQuality;

  boolean eightBitColors;

  boolean requestCursorUpdates;
  boolean ignoreCursorUpdates;

  boolean reverseMouseButtons2And3;
  boolean shareDesktop;
  boolean viewOnly;
  boolean showControls;

  //
  // Constructor.  Set up the labels and choices from the names and values
  // arrays.
  //

  OptionsFrame(VncViewer v) {
    super("TightVNC Options");

    viewer = v;

    GridBagLayout gridbag = new GridBagLayout();
    setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.fill = GridBagConstraints.BOTH;

    for (int i = 0; i < names.length; i++) {
      labels[i] = new Label(names[i]);
      gbc.gridwidth = 1;
      gridbag.setConstraints(labels[i],gbc);
      add(labels[i]);

      choices[i] = new Choice();
      gbc.gridwidth = GridBagConstraints.REMAINDER;
      gridbag.setConstraints(choices[i],gbc);
      add(choices[i]);
      choices[i].addItemListener(this);

      for (int j = 0; j < values[i].length; j++) {
	choices[i].addItem(values[i][j]);
      }
    }

    closeButton = new Button("Close");
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gridbag.setConstraints(closeButton, gbc);
    add(closeButton);
    closeButton.addActionListener(this);

    pack();

    addWindowListener(this);

    // Set up defaults

    choices[encodingIndex].select("Tight");
    choices[compressLevelIndex].select("Default");
    choices[jpegQualityIndex].select("6");
    choices[cursorUpdatesIndex].select("Enable");
    choices[useCopyRectIndex].select("Yes");
    choices[eightBitColorsIndex].select("No");
    choices[mouseButtonIndex].select("Normal");
    choices[viewOnlyIndex].select("No");
    choices[shareDesktopIndex].select("Yes");

    // But let them be overridden by parameters

    for (int i = 0; i < names.length; i++) {
      String s = viewer.readParameter(names[i], false);
      if (s != null) {
	for (int j = 0; j < values[i].length; j++) {
	  if (s.equalsIgnoreCase(values[i][j])) {
	    choices[i].select(j);
	  }
	}
      }
    }

    // "Show Controls" setting does not have associated GUI option

    showControls = true;
    String s = viewer.readParameter("Show Controls", false);
    if (s != null && s.equalsIgnoreCase("No"))
      showControls = false;

    // Make the booleans and encodings array correspond to the state of the GUI

    setEncodings();
    setColorFormat();
    setOtherOptions();
  }


  //
  // Disable the shareDesktop option
  //

  void disableShareDesktop() {
    labels[shareDesktopIndex].setEnabled(false);
    choices[shareDesktopIndex].setEnabled(false);
  }


  //
  // setEncodings looks at the encoding, compression level, JPEG
  // quality level, cursor shape updates and copyRect choices and sets
  // the encodings array appropriately. It also calls the VncViewer's
  // setEncodings method to send a message to the RFB server if
  // necessary.
  //

  void setEncodings() {
    nEncodings = 0;
    if (choices[useCopyRectIndex].getSelectedItem().equals("Yes")) {
      encodings[nEncodings++] = RfbProto.EncodingCopyRect;
    }

    int preferredEncoding = RfbProto.EncodingRaw;
    boolean enableCompressLevel = false;

    if (choices[encodingIndex].getSelectedItem().equals("RRE")) {
      preferredEncoding = RfbProto.EncodingRRE;
    } else if (choices[encodingIndex].getSelectedItem().equals("CoRRE")) {
      preferredEncoding = RfbProto.EncodingCoRRE;
    } else if (choices[encodingIndex].getSelectedItem().equals("Hextile")) {
      preferredEncoding = RfbProto.EncodingHextile;
    } else if (choices[encodingIndex].getSelectedItem().equals("Zlib")) {
      preferredEncoding = RfbProto.EncodingZlib;
      enableCompressLevel = true;
    } else if (choices[encodingIndex].getSelectedItem().equals("Tight")) {
      preferredEncoding = RfbProto.EncodingTight;
      enableCompressLevel = true;
    }

    encodings[nEncodings++] = preferredEncoding;
    if (preferredEncoding != RfbProto.EncodingHextile) {
      encodings[nEncodings++] = RfbProto.EncodingHextile;
    }
    if (preferredEncoding != RfbProto.EncodingTight) {
      encodings[nEncodings++] = RfbProto.EncodingTight;
    }
    if (preferredEncoding != RfbProto.EncodingZlib) {
      encodings[nEncodings++] = RfbProto.EncodingZlib;
    }
    if (preferredEncoding != RfbProto.EncodingCoRRE) {
      encodings[nEncodings++] = RfbProto.EncodingCoRRE;
    }
    if (preferredEncoding != RfbProto.EncodingRRE) {
      encodings[nEncodings++] = RfbProto.EncodingRRE;
    }

    // Handle compression level setting.

    if (enableCompressLevel) {
      labels[compressLevelIndex].setEnabled(true);
      choices[compressLevelIndex].setEnabled(true);
      try {
	compressLevel =
	  Integer.parseInt(choices[compressLevelIndex].getSelectedItem());
      }
      catch (NumberFormatException e) {
	compressLevel = -1;
      }
      if (compressLevel >= 1 && compressLevel <= 9) {
	encodings[nEncodings++] =
	  RfbProto.EncodingCompressLevel0 + compressLevel;
      } else {
	compressLevel = -1;
      }
    } else {
      labels[compressLevelIndex].setEnabled(false);
      choices[compressLevelIndex].setEnabled(false);
    }

    // Handle JPEG quality setting.

    if (preferredEncoding == RfbProto.EncodingTight) {
      labels[jpegQualityIndex].setEnabled(true);
      choices[jpegQualityIndex].setEnabled(true);
      try {
	jpegQuality =
	  Integer.parseInt(choices[jpegQualityIndex].getSelectedItem());
      }
      catch (NumberFormatException e) {
	jpegQuality = -1;
      }
      if (jpegQuality >= 0 && jpegQuality <= 9) {
	encodings[nEncodings++] =
	  RfbProto.EncodingQualityLevel0 + jpegQuality;
      } else {
	jpegQuality = -1;
      }
    } else {
      labels[jpegQualityIndex].setEnabled(false);
      choices[jpegQualityIndex].setEnabled(false);
    }

    // Request cursor shape updates if necessary.

    requestCursorUpdates =
      !choices[cursorUpdatesIndex].getSelectedItem().equals("Disable");

    if (requestCursorUpdates) {
      encodings[nEncodings++] = RfbProto.EncodingXCursor;
      encodings[nEncodings++] = RfbProto.EncodingRichCursor;
      ignoreCursorUpdates =
	choices[cursorUpdatesIndex].getSelectedItem().equals("Ignore");
    }

    encodings[nEncodings++] = RfbProto.EncodingLastRect;
    encodings[nEncodings++] = RfbProto.EncodingNewFBSize;

    viewer.setEncodings();
  }

  //
  // setColorFormat sets eightBitColors variable depending on the GUI
  // setting, and switches between 8-bit and 24-bit colors mode, if
  // necessary.
  //

  void setColorFormat() {

    eightBitColors
      = choices[eightBitColorsIndex].getSelectedItem().equals("Yes");

    // FIXME: implement dynamic changing of the color mode.

  }

  //
  // setOtherOptions looks at the "other" choices (ones which don't set the
  // encoding or the color format) and sets the boolean flags appropriately.
  //

  void setOtherOptions() {

    reverseMouseButtons2And3
      = choices[mouseButtonIndex].getSelectedItem().equals("Reversed");

    viewOnly 
      = choices[viewOnlyIndex].getSelectedItem().equals("Yes");
    if (viewer.vc != null)
      viewer.vc.enableInput(!viewOnly);

    shareDesktop
      = choices[shareDesktopIndex].getSelectedItem().equals("Yes");
  }


  //
  // Respond to actions on Choice controls
  //

  public void itemStateChanged(ItemEvent evt) {
    Object source = evt.getSource();

    if (source == choices[encodingIndex] ||
        source == choices[compressLevelIndex] ||
        source == choices[jpegQualityIndex] ||
        source == choices[cursorUpdatesIndex] ||
        source == choices[useCopyRectIndex]) {

      setEncodings();

    } else if (source == choices[eightBitColorsIndex]) {

      setColorFormat();

    } else if (source == choices[mouseButtonIndex] ||
	       source == choices[shareDesktopIndex] ||
	       source == choices[viewOnlyIndex]) {

      setOtherOptions();
    }
  }

  //
  // Respond to button press
  //

  public void actionPerformed(ActionEvent evt) {
    if (evt.getSource() == closeButton)
      setVisible(false);
  }

  //
  // Respond to window events
  //

  public void windowClosing(WindowEvent evt) {
    setVisible(false);
  }

  public void windowActivated(WindowEvent evt) {}
  public void windowDeactivated(WindowEvent evt) {}
  public void windowOpened(WindowEvent evt) {}
  public void windowClosed(WindowEvent evt) {}
  public void windowIconified(WindowEvent evt) {}
  public void windowDeiconified(WindowEvent evt) {}
}
