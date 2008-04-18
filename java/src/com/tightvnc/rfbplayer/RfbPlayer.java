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

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;

public class RfbPlayer extends java.applet.Applet
    implements java.lang.Runnable, WindowListener {

  boolean inAnApplet = true;
  boolean inSeparateFrame = false;

  //
  // main() is called when run as a java program from the command line.
  // It simply runs the applet inside a newly-created frame.
  //
  public static void main(String[] argv) {
    RfbPlayer p = new RfbPlayer();
    p.mainArgs = argv;
    p.inAnApplet = false;
    p.inSeparateFrame = true;

    p.init();
    p.start();
  }

  String[] mainArgs;

  RfbProto rfb;
  Thread rfbThread;

  Frame vncFrame;
  Container vncContainer;
  ScrollPane desktopScrollPane;
  GridBagLayout gridbag;
  ButtonPanel buttonPanel;
  VncCanvas vc;

  String sessionURL;
  URL url;
  long initialTimeOffset;
  double playbackSpeed;
  boolean autoPlay;
  boolean showControls;
  int deferScreenUpdates;

  //
  // init()
  //
  public void init() {

    readParameters();

    if (inSeparateFrame) {
      vncFrame = new Frame("RFB Session Player");
      if (!inAnApplet) {
        vncFrame.add("Center", this);
      }
      vncContainer = vncFrame;
    } else {
      vncContainer = this;
    }

    if (inSeparateFrame)
      vncFrame.addWindowListener(this);

    rfbThread = new Thread(this);
    rfbThread.start();
  }

  public void update(Graphics g) {
  }

  //
  // run() - executed by the rfbThread to read RFB data.
  //
  public void run() {

    gridbag = new GridBagLayout();
    vncContainer.setLayout(gridbag);

    GridBagConstraints gbc = new GridBagConstraints();
    gbc.gridwidth = GridBagConstraints.REMAINDER;
    gbc.anchor = GridBagConstraints.NORTHWEST;

    if (showControls) {
      buttonPanel = new ButtonPanel(this);
      buttonPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));
      gridbag.setConstraints(buttonPanel, gbc);
      vncContainer.add(buttonPanel);
    }

    if (inSeparateFrame) {
      vncFrame.pack();
      vncFrame.show();
    } else {
      validate();
    }

    try {
      if (inAnApplet) {
        url = new URL(getCodeBase(), sessionURL);
      } else {
        url = new URL(sessionURL);
      }
      rfb = new RfbProto(url);

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

      while (true) {
        try {
          setPaused(!autoPlay);
          rfb.fbs.setSpeed(playbackSpeed);
          if (initialTimeOffset > rfb.fbs.getTimeOffset())
            setPos(initialTimeOffset); // don't seek backwards here
          vc.processNormalProtocol();
        } catch (EOFException e) {
          if (e.getMessage() != null && e.getMessage().equals("[REWIND]")) {
            // A special type of EOFException allowing us to seek backwards.
            initialTimeOffset = rfb.fbs.getSeekOffset();
            autoPlay = !rfb.fbs.isPaused();
          } else {
            // Return to the beginning after the playback is finished.
            initialTimeOffset = 0;
            autoPlay = false;
          }
          rfb.newSession(url);
          vc.updateFramebufferSize();
        }
      }

    } catch (FileNotFoundException e) {
      fatalError(e.toString());
    } catch (Exception e) {
      e.printStackTrace();
      fatalError(e.toString());
    }

  }

  public void setPaused(boolean paused) {
    if (showControls)
      buttonPanel.setPaused(paused);
    if (paused) {
      rfb.fbs.pausePlayback();
    } else {
      rfb.fbs.resumePlayback();
    }
  }

  public double getSpeed() {
    return playbackSpeed;
  }

  public void setSpeed(double speed) {
    playbackSpeed = speed;
    rfb.fbs.setSpeed(speed);
  }

  public void setPos(long pos) {
    rfb.fbs.setTimeOffset(pos);
  }

  public void updatePos() {
    if (showControls)
      buttonPanel.setPos(rfb.fbs.getTimeOffset());
  }

  //
  // readParameters() - read parameters from the html source or from the
  // command line.  On the command line, the arguments are just a sequence of
  // param_name/param_value pairs where the names and values correspond to
  // those expected in the html applet tag source.
  //
  public void readParameters() {

    sessionURL = readParameter("URL", true);

    initialTimeOffset = readLongParameter("Position", 0);
    if (initialTimeOffset < 0)
      initialTimeOffset = 0;

    playbackSpeed = readDoubleParameter("Speed", 1.0);
    if (playbackSpeed <= 0.0)
      playbackSpeed = 1.0;

    autoPlay = false;
    String str = readParameter("Autoplay", false);
    if (str != null && str.equalsIgnoreCase("Yes"))
      autoPlay = true;

    showControls = true;
    str = readParameter("Show Controls", false);
    if (str != null && str.equalsIgnoreCase("No"))
      showControls = false;

    if (inAnApplet) {
      str = readParameter("Open New Window", false);
      if (str != null && str.equalsIgnoreCase("Yes"))
        inSeparateFrame = true;
    }

    // Fine tuning options.
    deferScreenUpdates = (int)readLongParameter("Defer screen updates", 20);
    if (deferScreenUpdates < 0)
      deferScreenUpdates = 0;	// Just in case.
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
          return mainArgs[i + 1];
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

  long readLongParameter(String name, long defaultValue) {
    String str = readParameter(name, false);
    long result = defaultValue;
    if (str != null) {
      try {
        result = Long.parseLong(str);
      } catch (NumberFormatException e) {
      }
    }
    return result;
  }

  double readDoubleParameter(String name, double defaultValue) {
    String str = readParameter(name, false);
    double result = defaultValue;
    if (str != null) {
      try {
        result = Double.valueOf(str).doubleValue();
      } catch (NumberFormatException e) {
      }
    }
    return result;
  }

  //
  // fatalError() - print out a fatal error message.
  //
  public void fatalError(String str) {
    System.out.println(str);

    if (inAnApplet) {
      vncContainer.removeAll();
      if (rfb != null) {
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
    if (rfb != null) {
      rfb = null;
    }
    if (inSeparateFrame) {
      vncFrame.dispose();
    }
  }


  //
  // Close application properly on window close event.
  //
  public void windowClosing(WindowEvent evt) {
    vncContainer.removeAll();
    if (rfb != null)
      rfb = null;

    vncFrame.dispose();
    if (!inAnApplet) {
      System.exit(0);
    }
  }

  //
  // Ignore window events we're not interested in.
  //
  public void windowActivated(WindowEvent evt) {
  }

  public void windowDeactivated(WindowEvent evt) {
  }

  public void windowOpened(WindowEvent evt) {
  }

  public void windowClosed(WindowEvent evt) {
  }

  public void windowIconified(WindowEvent evt) {
  }

  public void windowDeiconified(WindowEvent evt) {
  }

}
