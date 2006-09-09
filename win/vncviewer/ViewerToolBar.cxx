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

// -=- ViewerToolBar.cxx

#include <vncviewer/ViewerToolBar.h>
#include <vncviewer/resource.h>

void ViewerToolBar::create(HWND parentHwnd) {
  // Create the toolbar panel
  ToolBar::create(ID_TOOLBAR, parentHwnd, WS_CHILD | 
    TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NORESIZE);
  addBitmap(4, IDB_TOOLBAR);

  // Create the control buttons
  addButton(0, ID_OPTIONS);
  addButton(1, ID_INFO);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(2, ID_FULLSCREEN);
  addButton(3, ID_REQUEST_REFRESH);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(4, ID_ZOOM_IN);
  addButton(5, ID_ZOOM_OUT);
  addButton(6, ID_ACTUAL_SIZE);
  addButton(7, ID_AUTO_SIZE);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(8, ID_SEND_CAD);
  addButton(9, ID_SEND_CTLESC);
  addButton(10, ID_CTRL_KEY);
  addButton(11, ID_ALT_KEY);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(12, ID_FILE_TRANSFER);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(13, ID_NEW_CONNECTION);
  addButton(14, ID_CONN_SAVE_AS);

  // Resize the toolbar window
  autoSize();
}

LRESULT ViewerToolBar::processWM_NOTIFY(WPARAM wParam, LPARAM lParam) {
  switch (((LPNMHDR)lParam)->code) { 
    // Process tooltips text
  case TTN_NEEDTEXT:
    {
      LPTOOLTIPTEXT TTStr = (LPTOOLTIPTEXT)lParam;
      if (TTStr->hdr.code != TTN_NEEDTEXT)
        return 0;

      switch (TTStr->hdr.idFrom) {
      case ID_OPTIONS:
        TTStr->lpszText = "Connection options...";
        break;
      case ID_INFO:
        TTStr->lpszText = "Connection info";
        break;
      case ID_FULLSCREEN:
        TTStr->lpszText = "Full screen";
        break;
      case ID_REQUEST_REFRESH:
        TTStr->lpszText = "Request screen refresh";
        break;
      case ID_SEND_CAD:
        TTStr->lpszText = "Send Ctrl-Alt-Del";
        break;
      case ID_SEND_CTLESC:
        TTStr->lpszText = "Send Ctrl-Esc";
        break;
      case ID_CTRL_KEY:
        TTStr->lpszText = "Send Ctrl key press/release";
        break;
      case ID_ALT_KEY:
        TTStr->lpszText = "Send Alt key press/release";
        break;
      case ID_FILE_TRANSFER:
        TTStr->lpszText = "Transfer files...";
        break;
      case ID_NEW_CONNECTION:
        TTStr->lpszText = "New connection...";
        break;
      case ID_CONN_SAVE_AS:
        TTStr->lpszText = "Save connection info as...";
        break;
      default:
        break;
      }
    }

  default:
    break;
  }
  return 0;
}
