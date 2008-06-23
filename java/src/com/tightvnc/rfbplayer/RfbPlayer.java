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

package com.tightvnc.rfbplayer;

import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class RfbPlayer extends java.applet.Applet
    implements java.lang.Runnable, WindowListener {

  boolean inAnApplet = true;
  boolean inSeparateFrame = false;

  /** current applet width */
  int dispW = 300;
  /** current applet height */
  int dispH = 200;

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

  FbsInputStream fbs;
  RfbProto rfb;
  Thread rfbThread;

  Frame vncFrame;
  Container vncContainer;
  //ScrollPane desktopScrollPane;
  LWScrollPane desktopScrollPane;
  GridBagLayout gridbag;
  ButtonPanel buttonPanel;
  VncCanvas vc;

  String sessionURL;
  String idxPrefix;
  long initialTimeOffset;
  double playbackSpeed;
  boolean autoPlay;
  boolean showControls;
  boolean isQuitting = false;
  int deferScreenUpdates;

  //
  // init()
  //
  public void init() {

    // LiveConnect work-a-round
    RfbSharedStatic.refApplet = this;

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

    rfbThread = new Thread(this, "RfbThread");
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
      vncFrame.setVisible(true);
    } else {
      validate();
    }

    try {
      java.applet.Applet applet = (inAnApplet) ? this : null;
      FbsConnection conn = new FbsConnection(sessionURL, idxPrefix, applet);
      fbs = conn.connect(initialTimeOffset);
      rfb = new RfbProto(fbs);

      vc = new VncCanvas(this);
      gbc.weightx = 1.0;
      gbc.weighty = 1.0;

      // Create a panel which itself is resizeable and can hold
      // non-resizeable VncCanvas component at the top left corner.
      //Panel canvasPanel = new Panel();
      //canvasPanel.setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));
      //canvasPanel.add(vc);

      // Create a ScrollPane which will hold a panel with VncCanvas
      // inside.
      //desktopScrollPane = new ScrollPane(ScrollPane.SCROLLBARS_AS_NEEDED);
      desktopScrollPane = new LWScrollPane();
      gbc.fill = GridBagConstraints.BOTH;
      gridbag.setConstraints(vc, gbc);
      //gridbag.setConstraints(canvasPanel, gbc);
      desktopScrollPane.addComp(vc);
      desktopScrollPane.setSize(dispW, dispH);
      //desktopScrollPane.add(canvasPanel);

      // Now add the scroll bar to the container.
      if (inSeparateFrame) {
        gridbag.setConstraints(desktopScrollPane, gbc);
        vncFrame.add(desktopScrollPane);
        vncFrame.setTitle(rfb.desktopName);
        vc.resizeDesktopFrame();
      } else {
        // Size the scroll pane to display size.
        desktopScrollPane.setSize(dispW, dispH);

        // Just add the VncCanvas component to the Applet.
        gbc.fill = GridBagConstraints.NONE;
        gridbag.setConstraints(desktopScrollPane, gbc);
        add(desktopScrollPane);
        validate();
        vc.resizeEmbeddedApplet();
      }

      while (!isQuitting) {
        try {
          setPaused(!autoPlay);
          fbs.setSpeed(playbackSpeed);
          vc.processNormalProtocol();
        } catch (EOFException e) {
          long newTimeOffset;
          if (e.getMessage() != null && e.getMessage().equals("[JUMP]")) {
            // A special type of EOFException allowing us to close FBS stream
            // and then re-open it for jumping to a different time offset.
            newTimeOffset = fbs.getSeekOffset();
            autoPlay = !fbs.isPaused();
          } else {
            // Return to the beginning after the playback is finished.
            newTimeOffset = 0;
            autoPlay = false;
          }
          fbs.close();
          fbs = conn.connect(newTimeOffset);
          rfb.newSession(fbs);
          vc.updateFramebufferSize();
        } catch (NullPointerException e) {
          // catching this causes a hang with 1.4.1 JVM's under Win32 IE
          throw e;
        }
      }

    } catch (FileNotFoundException e) {
      fatalError(e.toString());
    } catch (Exception e) {
      e.printStackTrace();
      fatalError(e.toString());
    }

  }

  public void setPausedInt(String paused) {
    // default to true (pause)
    int pause = 1;

    try {
      pause = Integer.parseInt(paused);
    } catch (NumberFormatException e) {
    }

    if (pause == 0) {
      setPaused(false);
    } else {
      setPaused(true);
    }
  }

  public void setPaused(boolean paused) {
    if (showControls)
      buttonPanel.setPaused(paused);
    if (paused) {
      fbs.pausePlayback();
    } else {
      fbs.resumePlayback();
    }
  }

  public double getSpeed() {
    return playbackSpeed;
  }

  public void setSpeed(double speed) {
    playbackSpeed = speed;
    fbs.setSpeed(speed);
  }

  public void jumpTo(long pos) {
    long diff = Math.abs(pos - fbs.getTimeOffset());

    // Current threshold is 5 seconds
    if (diff > 5000) {
      fbs.pausePlayback();
      setPos(pos);
      fbs.resumePlayback();
    }
  }

  public void setPos(long pos) {
    fbs.setTimeOffset(pos, true);
  }

  public void updatePos() {
    if (showControls && buttonPanel != null)
      buttonPanel.setPos(fbs.getTimeOffset());
  }

  //
  // readParameters() - read parameters from the html source or from the
  // command line.  On the command line, the arguments are just a sequence of
  // param_name/param_value pairs where the names and values correspond to
  // those expected in the html applet tag source.
  //
  public void readParameters() {

    sessionURL = readParameter("URL", true);
    idxPrefix = readParameter("Index", false);

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
    str = readParameter("Show_Controls", false);
    if (str != null && str.equalsIgnoreCase("No"))
      showControls = false;

    if (inAnApplet) {
      str = readParameter("Open_New_Window", false);
      if (str != null && str.equalsIgnoreCase("Yes"))
        inSeparateFrame = true;
    }

    // Fine tuning options.
    deferScreenUpdates = (int)readLongParameter("Defer_screen_updates", 20);
    if (deferScreenUpdates < 0)
      deferScreenUpdates = 0;	// Just in case.

    // Display width and height.
    dispW = readIntParameter("DISPLAY_WIDTH", dispW);
    dispH = readIntParameter("DISPLAY_HEIGHT", dispH);
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

  int readIntParameter(String name, int defaultValue) {
    String str = readParameter(name, false);
    int result = defaultValue;
    if (str != null) {
      try {
        result = Integer.parseInt(str);
      } catch (NumberFormatException e) {
      }
    }
    return result;
  }

  //
  // fatalError() - print out a fatal error message.
  //
  public void fatalError(String str) {
    System.err.println(str);

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
    isQuitting = true;
    vncContainer.removeAll();
    if (fbs != null) {
      fbs.quit();
      try {
        fbs.close();
      } catch (IOException e) {
      }
    }
    try {
      rfbThread.join();
    } catch (InterruptedException e) {
    }
    if (inSeparateFrame) {
      vncFrame.removeWindowListener(this);
      vncFrame.dispose();
    }
  }

  //
  // Set the new width and height of the applet. Call when browser is resized to 
  // resize the viewer.
  //
  public void displaySize(int width, int height) {
    dispW = width;
    dispH = height;
    if (!inSeparateFrame) {
      vc.resizeEmbeddedApplet();
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
