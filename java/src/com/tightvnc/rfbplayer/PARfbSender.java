
package com.HorizonLive.RfbPlayer;

import java.applet.*;

public class PARfbSender extends Applet {

  public void init() {
    Applet receiver = null;
    receiver = RfbSharedStatic.refApplet;
    long time = Long.valueOf(getParameter("time")).longValue();

    if (receiver != null) {
      ((RfbPlayer)receiver).jumpTo(time);
    } else
      System.out.println("Couldn't jump to time: " + time + " in RfbPlayer.");
  }

}
