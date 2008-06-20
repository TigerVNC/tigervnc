
package com.tightvnc.rfbplayer;

import java.applet.*;

public class PARfbSender extends Applet {

  public void init() {
    Applet receiver = null;
    receiver = RfbSharedStatic.refApplet;
    long time = Long.valueOf(getParameter("time")).longValue();
    boolean pause = (Integer.parseInt(getParameter("pause")) != 0);
    boolean unpause = (Integer.parseInt(getParameter("unpause")) != 0);

    if (receiver != null) {

      if (pause) {
        ((RfbPlayer)receiver).setPaused(true);
      } else if (unpause) {
        ((RfbPlayer)receiver).setPaused(false);
      } else {
        ((RfbPlayer)receiver).jumpTo(time);
      }
    } else
      System.err.println("Couldn't jump to time: " + time + " in RfbPlayer.");
  }

}
