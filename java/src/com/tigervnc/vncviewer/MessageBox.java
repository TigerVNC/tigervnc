/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 TigerVNC Team
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.tigervnc.vncviewer;

import java.awt.*;

public class MessageBox extends com.tigervnc.vncviewer.Dialog {

  public static final int MB_OK = 0;
  public static final int MB_OKAYCANCEL = 1;
  public static final int MB_YESNO = 2;

  public MessageBox(String msg, int flags) {
    super(true);
    GridLayout g = new GridLayout(0,1);
    setLayout(g);
    while (true) {
      int i = msg.indexOf('\n');
      int j = (i==-1) ? msg.length() : i;
      add(new Label(msg.substring(0, j)));
      if (i==-1) break;
      msg = msg.substring(j+1);
    }
    Panel p2 = new Panel();
    switch (flags & 3) {
    case MB_OKAYCANCEL:
      cancelButton = new Button("Cancel");
      // No break
    case MB_OK:
      okButton = new Button("OK");
      break;
    case MB_YESNO:
      okButton = new Button("Yes");
      cancelButton = new Button("No");
      break;
    }
    if (okButton != null) p2.add(okButton);
    if (cancelButton != null) p2.add(cancelButton);
    add("South", p2);
    pack();
    showDialog();
  }

  public MessageBox(String msg) {
    this(msg, MB_OK);
  }


  public boolean action(Event event, Object arg) {
    if (event.target == okButton) {
      ok = true;
      endDialog();
    } else if (event.target == cancelButton) {
      ok = false;
      endDialog();
    }
    return true;
  }

  Button okButton, cancelButton;

  public boolean result() {
    return ok;
  }

}
