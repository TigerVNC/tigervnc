//
//  Copyright (C) 2001,2002 HorizonLive.com, Inc.  All Rights Reserved.
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
// VncViewer.java - the VNC viewer applet.  This class mainly just sets up the
// user interface, leaving it to the VncCanvas to do the actual rendering of
// a VNC desktop.
//

import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class VncViewer extends java.applet.Applet
  implements java.lang.Runnable, WindowListener {

  boolean inAnApplet = true;
  boolean inSeparateFrame = false;

  //
  // main() is called when run as a java program from the command line.
  // It simply runs the applet inside a newly-created frame.
  //

  public static void main(String[] argv) {
    VncViewer v = new VncViewer();
    v.mainArgs = argv;
    v.inAnApplet = false;
    v.inSeparateFrame = true;

    v.init();
    v.start();
  }

  String[] mainArgs;

  RfbProto rfb;
  Thread rfbThread;

  Frame vncFrame;
  Container vncContainer;
  ScrollPane desktopScrollPane;
  GridBagLayout gridbag;
  ButtonPanel buttonPanel;
  AuthPanel authenticator;
  VncCanvas vc;
  OptionsFrame options;
  ClipboardFrame clipboard;

  // Variables read from parameter values.
  String host;
  int port;
  String passwordParam;
  int deferScreenUpdates;
  int deferCursorUpdates;
  int deferUpdateRequests;


  //
  // init()
  //

  public void init() {

    readParameters();

    if (inSeparateFrame) {
      vncFrame = new Frame("TightVNC");
      if (!inAnApplet) {
	vncFrame.add("Center", this);
      }
      vncContainer = vncFrame;
    } else {
      vncContainer = this;
    }

    options = new OptionsFrame(this);
    clipboard = new ClipboardFrame(this);
    authenticator = new AuthPanel();

    if (inSeparateFrame)
      vncFrame.addWindowListener(this);

    rfbThread = new Thread(this);
    rfbThread.start();
  }

  public void update(Graphics g) {
  }

  //
  // run() - executed by the rfbThread to deal with the RFB socket.
  //

  public void run() {

    gridbag = new GridBagLayout();
    vncContainer.setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.anchor = GridBagConstraints.NORTHWEST;

    if (options.showControls) {
      buttonPanel = new ButtonPanel(this);
      buttonPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));
      gridbag.setConstraints(buttonPanel, gbc);
      vncContainer.add(buttonPanel);
    }

    try {
      connectAndAuthenticate();

      doProtocolInitialisation();

      vc = new VncCanvas(this);
      gbc.weightx = 1.0;
      gbc.weighty = 1.0;

      if (inSeparateFrame) {

	// Create a panel which itself is resizeable and can hold
	// non-resizeable VncCanvas component at the top left corner.
	Panel canvasPanel = new Panel();
	canvasPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));
	canvasPanel.add(vc);

	// Create a ScrollPane which will hold a panel with VncCanvas
	// inside.
	desktopScrollPane = new ScrollPane(ScrollPane.SCROLLBARS_AS_NEEDED);
	gbc.fill = GridBagConstraints.BOTH;
	gridbag.setConstraints(desktopScrollPane, gbc);
	desktopScrollPane.add(canvasPanel);

	// Finally, add our ScrollPane to the Frame window.
	vncFrame.add(desktopScrollPane);
	vncFrame.setTitle(rfb.desktopName);
	vncFrame.pack();
	vc.resizeDesktopFrame();

      } else {

	// Just add the VncCanvas component to the Applet.
	gridbag.setConstraints(vc, gbc);
	add(vc);
	validate();

      }

      if (options.showControls)
        buttonPanel.enableButtons();

      moveFocusToDesktop();
      vc.processNormalProtocol();

    } catch (Exception e) {
      e.printStackTrace();
      fatalError(e.toString());
    }
    
  }


  //
  // Connect to the RFB server and authenticate the user.
  //

  void connectAndAuthenticate() throws IOException {

    // The simplest case -- don't ask user a password, get it from the
    // "PASSWORD" parameter instead. Authentication failures would be
    // fatal.

    if (passwordParam != null) {
      if (inSeparateFrame) {
	vncFrame.pack();
	vncFrame.show();
      } else {
	validate();
      }
      if (!tryAuthenticate(passwordParam)) {
	throw new IOException("VNC authentication failed");
      }
      return;
    }

    // There is no "PASSWORD" parameter -- ask user for a password,
    // try to authenticate, retry on authentication failures.

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.anchor = GridBagConstraints.NORTHWEST;
    gbc.weightx = 1.0;
    gbc.weighty = 1.0;
    gbc.ipadx = 100;
    gbc.ipady = 50;
    gridbag.setConstraints(authenticator, gbc);
    vncContainer.add(authenticator);

    if (inSeparateFrame) {
      vncFrame.pack();
      vncFrame.show();
    } else {
      validate();
      // FIXME: here moveFocusToPasswordField() does not always work
      // under Netscape 4.7x/Java 1.1.5/Linux. It seems like this call
      // is being executed before the password field of the
      // authenticator is fully drawn and activated, therefore
      // requestFocus() does not work. Currently, I don't know how to
      // solve this problem.
      //   -- const
      authenticator.moveFocusToPasswordField();
    }

    while (true) {
      // Wait for user entering a password.
      synchronized(authenticator) {
	try {
	  authenticator.wait();
	} catch (InterruptedException e) {
	}
      }

      // Try to authenticate with a given password.
      if (tryAuthenticate(authenticator.password.getText()))
	break;

      // Retry on authentication failure.
      authenticator.retry();
    }

    vncContainer.remove(authenticator);
  }


  //
  // Try to authenticate with a given password.
  //

  boolean tryAuthenticate(String pw) throws IOException {

    rfb = new RfbProto(host, port, this);

    rfb.readVersionMsg();

    System.out.println("RFB server supports protocol version " +
		       rfb.serverMajor + "." + rfb.serverMinor);

    rfb.writeVersionMsg();

    int authScheme = rfb.readAuthScheme();

    switch (authScheme) {

    case RfbProto.NoAuth:
      System.out.println("No authentication needed");
      return true;

    case RfbProto.VncAuth:
      byte[] challenge = new byte[16];
      rfb.is.readFully(challenge);

      if (pw.length() > 8)
	pw = pw.substring(0, 8); // Truncate to 8 chars

      byte[] key = {0, 0, 0, 0, 0, 0, 0, 0};
      System.arraycopy(pw.getBytes(), 0, key, 0, pw.length());

      DesCipher des = new DesCipher(key);

      des.encrypt(challenge, 0, challenge, 0);
      des.encrypt(challenge, 8, challenge, 8);

      rfb.os.write(challenge);

      int authResult = rfb.is.readInt();

      switch (authResult) {
      case RfbProto.VncAuthOK:
	System.out.println("VNC authentication succeeded");
	return true;
      case RfbProto.VncAuthFailed:
	System.out.println("VNC authentication failed");
	break;
      case RfbProto.VncAuthTooMany:
	throw new IOException("VNC authentication failed - " +
			      "too many tries");
      default:
	throw new IOException("Unknown VNC authentication result " +
			      authResult);
      }
      break;

    default:
      throw new IOException("Unknown VNC authentication scheme " +
			    authScheme);
    }
    return false;
  }


  //
  // Do the rest of the protocol initialisation.
  //

  void doProtocolInitialisation() throws IOException {

    rfb.writeClientInit();

    rfb.readServerInit();

    System.out.println("Desktop name is " + rfb.desktopName);
    System.out.println("Desktop size is " + rfb.framebufferWidth + " x " +
		       rfb.framebufferHeight);

    setEncodings();
  }


  //
  // Send current encoding list to the RFB server.
  //

  void setEncodings() {
    try {
      if (rfb != null && rfb.inNormalProtocol) {
	rfb.writeSetEncodings(options.encodings, options.nEncodings);
	if (vc != null) {
	  vc.softCursorFree();
	}
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }


  //
  // setCutText() - send the given cut text to the RFB server.
  //

  void setCutText(String text) {
    try {
      if ((rfb != null) && rfb.inNormalProtocol) {
	rfb.writeClientCutText(text);
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }


  //
  // readParameters() - read parameters from the html source or from the
  // command line.  On the command line, the arguments are just a sequence of
  // param_name/param_value pairs where the names and values correspond to
  // those expected in the html applet tag source.
  //

  public void readParameters() {
    host = readParameter("HOST", !inAnApplet);
    if (host == null) {
      host = getCodeBase().getHost();
      if (host.equals("")) {
	fatalError("HOST parameter not specified");
      }
    }

    String str = readParameter("PORT", true);
    port = Integer.parseInt(str);

    if (inAnApplet) {
      str = readParameter("Open New Window", false);
      if (str != null && str.equalsIgnoreCase("Yes"))
	inSeparateFrame = true;
    }

    passwordParam = readParameter("PASSWORD", false);

    // Fine tuning options.
    deferScreenUpdates = readIntParameter("Defer screen updates", 20);
    deferCursorUpdates = readIntParameter("Defer cursor updates", 10);
    deferUpdateRequests = readIntParameter("Defer update requests", 50);
  }

  public String readParameter(String name, boolean required) {
    if (inAnApplet) {
      String s = getParameter(name);
      if ((s == null) && required) {
	fatalError(name + " parameter not specified");
      }
      return s;
    }

    for (int i = 0; i < mainArgs.length; i += 2) {
      if (mainArgs[i].equalsIgnoreCase(name)) {
	try {
	  return mainArgs[i+1];
	} catch (Exception e) {
	  if (required) {
	    fatalError(name + " parameter not specified");
	  }
	  return null;
	}
      }
    }
    if (required) {
      fatalError(name + " parameter not specified");
    }
    return null;
  }

  int readIntParameter(String name, int defaultValue) {
    String str = readParameter(name, false);
    int result = defaultValue;
    if (str != null) {
      try {
	result = Integer.parseInt(str);
      } catch (NumberFormatException e) { }
    }
    return result;
  }

  //
  // moveFocusToDesktop() - move keyboard focus either to the
  // VncCanvas or to the AuthPanel.
  //

  void moveFocusToDesktop() {
    if (vncContainer != null) {
      if (vc != null && vncContainer.isAncestorOf(vc)) {
	vc.requestFocus();
      } else if (vncContainer.isAncestorOf(authenticator)) {
	authenticator.moveFocusToPasswordField();
      }
    }
  }

  //
  // disconnect() - close connection to server.
  //

  public void disconnect() {
    System.out.println("Disconnect");
    options.dispose();
    clipboard.dispose();

    if (inAnApplet) {
      vncContainer.removeAll();
      if (rfb != null) {
	rfb.close();
	rfb = null;
      }
      Label errLabel = new Label("Disconnected");
      errLabel.setFont(new Font("Helvetica", Font.PLAIN, 12));
      vncContainer.setLayout(new FlowLayout(FlowLayout.LEFT, 30, 30));
      vncContainer.add(errLabel);
      if (inSeparateFrame) {
	vncFrame.pack();
      } else {
	validate();
      }
      rfbThread.stop();
    } else {
      System.exit(0);
    }
  }

  //
  // fatalError() - print out a fatal error message.
  //

  public void fatalError(String str) {
    System.out.println(str);

    if (inAnApplet) {
      vncContainer.removeAll();
      if (rfb != null) {
	rfb.close();
	rfb = null;
      }
      Label errLabel = new Label(str);
      errLabel.setFont(new Font("Helvetica", Font.PLAIN, 12));
      vncContainer.setLayout(new FlowLayout(FlowLayout.LEFT, 30, 30));
      vncContainer.add(errLabel);
      if (inSeparateFrame) {
	vncFrame.pack();
      } else {
	validate();
      }
      Thread.currentThread().stop();
    } else {
      System.exit(1);
    }
  }


  //
  // This method is called before the applet is destroyed.
  //

  public void destroy() {
    vncContainer.removeAll();
    options.dispose();
    clipboard.dispose();
    if (rfb != null) {
      rfb.close();
    }
    if (inSeparateFrame) {
      vncFrame.dispose();
    }
  }


  //
  // Close application properly on window close event.
  //

  public void windowClosing(WindowEvent evt) {
    if (rfb != null)
      disconnect();

    vncFrame.dispose();
    if (!inAnApplet) {
      System.exit(0);
    }
  }

  //
  // Move the keyboard focus to the password field on window activation.
  //

  public void windowActivated(WindowEvent evt) {
    if (vncFrame.isAncestorOf(authenticator))
      authenticator.moveFocusToPasswordField();
  }

  //
  // Ignore window events we're not interested in.
  //

  public void windowDeactivated (WindowEvent evt) {}
  public void windowOpened(WindowEvent evt) {}
  public void windowClosed(WindowEvent evt) {}
  public void windowIconified(WindowEvent evt) {}
  public void windowDeiconified(WindowEvent evt) {}
}
