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

// -=- PlayerToolBar.cxx

#include <rfbplayer/rfbplayer.h>
#include <rfbplayer/resource.h>

PlayerToolBar::PlayerToolBar() 
: ToolBar(), hFont(0), timeStatic(0), speedEdit(0), posTrackBar(0),
  speedUpDown(0), sliderDragging(false), sliderStepMs(0)
{
}

void PlayerToolBar::create(RfbPlayer *player_, HWND parentHwnd_) {
  HDC hdc;
  SIZE sz;
  RECT tRect;
  NONCLIENTMETRICS nonClientMetrics;

  player = player_;

  // Get the default font for the main menu
  nonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS);
  if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &nonClientMetrics, 0))
    MessageBox(0, "Can't access to the system font.", 
               "RfbPlayer", MB_OK | MB_ICONERROR);
  nonClientMetrics.lfMenuFont.lfHeight = 16;
  hFont = CreateFontIndirect(&nonClientMetrics.lfMenuFont);

  // Create the toolbar panel
  ToolBar::create(ID_TOOLBAR, parentHwnd_,
                  WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_NORESIZE);
  addBitmap(4, IDB_TOOLBAR);

  // Create the control buttons
  addButton(0, ID_PLAY);
  addButton(1, ID_PAUSE);
  addButton(2, ID_STOP);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);

  // Create the static control for the time output
  timeStatic = CreateWindowEx(0, "Static", "00m:00s (00m:00s)", 
    WS_CHILD | WS_VISIBLE, 0, 0, 20, 20, getHandle(), (HMENU)ID_TIME_STATIC, 
    GetModuleHandle(0), 0);
  SendMessage(timeStatic, WM_SETFONT,(WPARAM) hFont, TRUE);
  hdc = GetDC(timeStatic);
  SelectObject(hdc, hFont);
  GetTextExtentPoint32(hdc, "00m:00s (00m:00s)", 16, &sz);
  addButton(sz.cx + 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(0, 10, TBSTATE_ENABLED, TBSTYLE_SEP);
  getButtonRect(4, &tRect);
  MoveWindow(timeStatic, tRect.left, tRect.top+2, tRect.right-tRect.left, 
    tRect.bottom-tRect.top, FALSE);
    
  // Create the trackbar control for the time position
  addButton(200, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  getButtonRect(6, &tRect);
  posTrackBar = CreateWindowEx(0, TRACKBAR_CLASS, "Trackbar Control", 
    WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_ENABLESELRANGE,
    tRect.left, tRect.top, tRect.right-tRect.left, tRect.bottom-tRect.top,
    parentHwnd, (HMENU)ID_POS_TRACKBAR, GetModuleHandle(0), 0);
  // It's need to send notify messages to toolbar parent window
  SetParent(posTrackBar, getHandle());
  addButton(0, 10, TBSTATE_ENABLED, TBSTYLE_SEP);

  // Create the label with "Speed:" caption
  HWND speedStatic = CreateWindowEx(0, "Static", "Speed:", WS_CHILD | WS_VISIBLE, 
    0, 0, 5, 5, getHandle(), (HMENU)ID_SPEED_STATIC, GetModuleHandle(0), 0);
  SendMessage(speedStatic, WM_SETFONT,(WPARAM) hFont, TRUE);
  hdc = GetDC(speedStatic);
  SelectObject(hdc, hFont);
  GetTextExtentPoint32(hdc, "Speed:", 6, &sz);
  addButton(sz.cx + 10, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  getButtonRect(8, &tRect);
  MoveWindow(speedStatic, tRect.left, tRect.top+2, tRect.right-tRect.left, 
    tRect.bottom-tRect.top, FALSE);

  // Create the edit control and the spin for the speed managing
  addButton(60, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  getButtonRect(9, &tRect);
  speedEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "1.00", 
    WS_CHILD | WS_VISIBLE | ES_RIGHT, tRect.left, tRect.top, 
    tRect.right-tRect.left, tRect.bottom-tRect.top, parentHwnd,
    (HMENU)ID_SPEED_EDIT, GetModuleHandle(0), 0);
  SendMessage(speedEdit, WM_SETFONT,(WPARAM) hFont, TRUE);
  // It's need to send notify messages to toolbar parent window
  SetParent(speedEdit, getHandle());

  speedUpDown = CreateUpDownControl(WS_CHILD | WS_VISIBLE  
    | WS_BORDER | UDS_ALIGNRIGHT, 0, 0, 0, 0, getHandle(),
    ID_SPEED_UPDOWN, GetModuleHandle(0), speedEdit, 20, 1, 2);

  // Resize the toolbar window
  autoSize();
}

void PlayerToolBar::init(long sessionTimeMs_) {
  sessionTimeMs = sessionTimeMs_;

  setSessionTimeStr(sessionTimeMs);
  SendMessage(posTrackBar, TBM_SETRANGE, 
    TRUE, MAKELONG(0, min(sessionTimeMs / 1000, MAX_POS_TRACKBAR_RANGE)));
  if (sessionTimeMs == 0) {
    sliderStepMs = 1;
  } else {
    sliderStepMs = sessionTimeMs / SendMessage(posTrackBar, TBM_GETRANGEMAX, 0, 0);
  }
  updatePos(0);
}

void PlayerToolBar::enable() {
  enableButton(ID_PLAY, true);
  enableButton(ID_PAUSE, true);
  enableButton(ID_STOP, true);
  enableButton(ID_FULLSCREEN, true);
  EnableWindow(posTrackBar, true);
  EnableWindow(speedEdit, true);
  EnableWindow(speedUpDown, true);
}

void PlayerToolBar::disable() {
  enableButton(ID_PLAY, false);
  enableButton(ID_PAUSE, false);
  enableButton(ID_STOP, false);
  enableButton(ID_FULLSCREEN, false);
  EnableWindow(posTrackBar, false);
  EnableWindow(speedEdit, false);
  EnableWindow(speedUpDown, false);
}

LRESULT PlayerToolBar::processWM_COMMAND(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {

  case ID_RETURN:
    // Update the speed if return pressed in speedEdit
    if (getSpeedEditHwnd() == GetFocus()) {
      char speedStr[20], *stopStr;
      GetWindowText(getSpeedEditHwnd(), speedStr, sizeof(speedStr));
      double speed = strtod(speedStr, &stopStr);
      if (speed > 0) {
        speed = min(MAX_SPEED, speed);
      } else {
        speed = player->getSpeed();
      }
      player->setSpeed(speed);
      return FALSE;
    }
  }

  return TRUE;
}

LRESULT PlayerToolBar::processWM_HSCROLL(WPARAM wParam, LPARAM lParam) {
  long Pos = SendMessage(posTrackBar, TBM_GETPOS, 0, 0);
  Pos *= sliderStepMs;

  switch (LOWORD(wParam)) { 
  case TB_PAGEUP:
  case TB_PAGEDOWN:
  case TB_LINEUP:
  case TB_LINEDOWN:
  case TB_THUMBTRACK:
    sliderDragging = true;
    updatePos(Pos);
    return FALSE;
  case TB_THUMBPOSITION:
  case TB_ENDTRACK: 
    player->setPos(Pos);
    player->setPaused(player->isPaused());;
    updatePos(Pos);
    sliderDragging = false;
    return FALSE;
  default:
    break;
  }
  
  return TRUE;
}

LRESULT PlayerToolBar::processWM_NOTIFY(WPARAM wParam, LPARAM lParam) {
  switch (((NMHDR*)lParam)->code) {
  case UDN_DELTAPOS:
    if ((int)wParam == ID_SPEED_UPDOWN) {
      char speedStr[20] = "\0";
      DWORD speedRange = SendMessage(speedUpDown, UDM_GETRANGE, 0, 0);
      LPNM_UPDOWN upDown = (LPNM_UPDOWN)lParam;
      double speed;

      // The out of range checking
      if (upDown->iDelta > 0) {
        speed = min(upDown->iPos + upDown->iDelta, LOWORD(speedRange)) * 0.5;
      } else {
        // It's need to round the UpDown position
        if ((upDown->iPos * 0.5) != player->getSpeed()) {
          upDown->iDelta = 0;
        }
        speed = max(upDown->iPos + upDown->iDelta, HIWORD(speedRange)) * 0.5;
      }
      player->setSpeed(speed);
    }
  }

  // We always return TRUE to prevent the change in the updown contol 
  // position. The control's position changes in the RfbPlayer::setSpeed().
  return TRUE;
}

void PlayerToolBar::updatePos(long newPos) {
  // Update time pos in static control
  char timePos[30] = "\0";
  long time = newPos / 1000;
  sprintf(timePos, "%.2um:%.2us (%s)", time/60, time%60, fullSessionTimeStr);
  SetWindowText(timeStatic, timePos);

  // Update the position of slider
  if (!sliderDragging) {
    double error = SendMessage(posTrackBar, TBM_GETPOS, 0, 0) * 
      sliderStepMs / double(newPos);
    if (!((error > 1 - CALCULATION_ERROR) && (error <= 1 + CALCULATION_ERROR))) {
      SendMessage(posTrackBar, TBM_SETPOS, TRUE, newPos / sliderStepMs);
    }
  }
}

void PlayerToolBar::setSessionTimeStr(long sessionTimeMs) {
  sprintf(fullSessionTimeStr, "%.2um:%.2us", 
    sessionTimeMs / 1000 / 60, sessionTimeMs / 1000 % 60);
}

void PlayerToolBar::setTimePos(long pos) {
  SendMessage(posTrackBar, TBM_SETPOS, TRUE, pos);
}