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
  protected TextField posText;
  protected TextField timeScaleText;

  protected int lastPos = -1;

  ButtonPanel(RfbPlayer player) {
    this.player = player;

    setLayout(new FlowLayout(FlowLayout.LEFT, 0, 0));

    playButton = new Button("Play");
    playButton.setEnabled(false);
    add(playButton);
    playButton.addActionListener(this);

    add(new Label(" Position:"));
    posText = new TextField(5);
    add(posText);
    posText.addActionListener(this);

    add(new Label(" Speed:"));
    timeScaleText = new TextField(5);
    timeScaleText.setText(String.valueOf(player.getSpeed()));
    add(timeScaleText);
    timeScaleText.addActionListener(this);
  }

  public void setPaused(boolean paused)
  {
    if (paused) {
      playButton.setLabel("Play");
      posText.setEditable(true);
    } else {
      playButton.setLabel("Pause");
      posText.setEditable(false);
    }
    playButton.setEnabled(true);
  }

  public void setPos(int pos) {
    if (pos != lastPos) {
      lastPos = pos;
      char[] zeroes = {'0', '0', '0', '0'};
      String text = String.valueOf(pos);
      if (text.length() < 4) {
	text = new String(zeroes, 0, 4 - text.length()) + text;
      }
      posText.setText(text);
      posText.setCaretPosition(text.length());
    }
  }

  //
  // Event processing.
  //

  public void actionPerformed(ActionEvent evt) {
    if (evt.getSource() == playButton) {
      player.setPaused(playButton.getLabel().equals("Pause"));
    } else if (evt.getSource() == posText) {
      player.setPos(Integer.parseInt(posText.getText()));
    } else if (evt.getSource() == timeScaleText) {
      double speed = Double.parseDouble(timeScaleText.getText());
      if (speed <= 0.0)
	speed = 1.0;
      timeScaleText.setText(String.valueOf(speed));
      player.setSpeed(speed);
    }
  }
}

