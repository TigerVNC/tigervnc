/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
//
// This Dialog class implements a pop-up dialog.  This is needed because
// apparently you can't use the standard AWT Dialog from within an applet.  The
// dialog can be made visible by calling its showDialog() method.  Dialogs can
// be modal or non-modal.  For a modal dialog box, the showDialog() method must
// be called from a thread other than the GUI thread, and it only returns when
// the dialog box has been dismissed.  For a non-modal dialog box, the
// showDialog() method returns immediately.

package com.tigervnc.vncviewer;

import java.awt.*;

class Dialog extends Frame {

  public Dialog(boolean modal_) { modal = modal_; }

  public boolean showDialog() {
    ok = false;
    done = false;
    initDialog();
    Dimension dpySize = getToolkit().getScreenSize();
    Dimension mySize = getSize();
    int x = (dpySize.width - mySize.width) / 2;
    int y = (dpySize.height - mySize.height) / 2;
    setLocation(x, y);
    show();
    if (!modal) return true;
    synchronized(this) {
      try {
        while (!done)
          wait();
      } catch (InterruptedException e) {
      }
    }
    return ok;
  }

  public void endDialog() {
    done = true;
    hide();
    if (modal) {
      synchronized (this) {
        notify();
      }
    }
  }

  // initDialog() can be overridden in a derived class.  Typically it is used
  // to make sure that checkboxes have the right state, etc.
  public void initDialog() {}

  public boolean handleEvent(Event event) {
    if (event.id == Event.WINDOW_DESTROY) {
      ok = false;
      endDialog();
    }   
    return super.handleEvent(event);
  }

  protected boolean ok, done;
  boolean modal;
}
