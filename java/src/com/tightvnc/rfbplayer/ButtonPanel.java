//
//  Copyright (C) 2002 HorizonLive.com, Inc.  All Rights Reserved.
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

class ButtonPanel extends Panel implements ActionListener {

  protected RfbPlayer player;
  protected Button playButton;
  protected Button pauseButton;

  ButtonPanel(RfbPlayer player) {
    this.player = player;

    setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));

    playButton = new Button("Play");
    playButton.setEnabled(false);
    add(playButton);
    playButton.addActionListener(this);

    pauseButton = new Button("Pause");
    pauseButton.setEnabled(false);
    add(pauseButton);
    pauseButton.addActionListener(this);
  }

  public void setMode(int mode) {
    switch(mode) {
    case RfbPlayer.MODE_PLAYBACK:
      playButton.setLabel("Stop");
      playButton.setEnabled(true);
      pauseButton.setLabel("Pause");
      pauseButton.setEnabled(true);
      break;
    case RfbPlayer.MODE_PAUSED:
      playButton.setLabel("Stop");
      playButton.setEnabled(true);
      pauseButton.setLabel("Resume");
      pauseButton.setEnabled(true);
      break;
    default:
      // case RfbPlayer.MODE_STOPPED:
      playButton.setLabel("Play");
      playButton.setEnabled(true);
      pauseButton.setLabel("Pause");
      pauseButton.setEnabled(false);
      break;
    }
    player.setMode(mode);
  }

  //
  // Event processing.
  //

  public void actionPerformed(ActionEvent evt) {
    if (evt.getSource() == playButton) {
      setMode((player.getMode() == RfbPlayer.MODE_STOPPED) ?
              RfbPlayer.MODE_PLAYBACK : RfbPlayer.MODE_STOPPED);
    } else if (evt.getSource() == pauseButton) {
      setMode((player.getMode() == RfbPlayer.MODE_PAUSED) ?
              RfbPlayer.MODE_PLAYBACK : RfbPlayer.MODE_PAUSED);
    }
  }
}

