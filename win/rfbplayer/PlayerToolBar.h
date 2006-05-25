/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- PlayerToolBar.h

// ToolBar for the RfbPlayer

#include <rfb_win32/ToolBar.h>

using namespace rfb::win32;

#define ID_TOOLBAR 500
#define ID_PLAY 510
#define ID_PAUSE 520
#define ID_TIME_STATIC 530
#define ID_SPEED_STATIC 540
#define ID_SPEED_EDIT 550
#define ID_POS_TRACKBAR 560
#define ID_SPEED_UPDOWN 570

#define MAX_SPEED 10.00
#define CALCULATION_ERROR MAX_SPEED / 1000
#define MAX_POS_TRACKBAR_RANGE 50

class RfbPlayer;

class PlayerToolBar : public ToolBar {
public:
  PlayerToolBar();
  ~PlayerToolBar() {}

  void create(RfbPlayer *player, HWND parentHwnd);

  void init(long sessionTimeMs);

  void enable();
  void disable();

  LRESULT processWM_COMMAND(WPARAM wParam, LPARAM lParam);
  LRESULT processWM_HSCROLL(WPARAM wParam, LPARAM lParam);
  LRESULT processWM_NOTIFY(WPARAM wParam, LPARAM lParam);

  HWND getSpeedEditHwnd() { return speedEdit; }
  HWND getSpeedUpDownHwnd() { return speedUpDown; }

  bool isPosSliderDragging() { return sliderDragging; };
  void updatePos(long newPos);
  void setSessionTimeStr(long sessionTimeMs);
  void setTimePos(long newPos);

protected:
  RfbPlayer *player;
  HFONT hFont;
  HWND timeStatic;
  HWND speedEdit;
  HWND posTrackBar;
  HWND speedUpDown;
  char fullSessionTimeStr[20];
  long sessionTimeMs;
  bool sliderDragging;
  long sliderStepMs;
};
