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
  ToolBar::create(ID_TOOLBAR, parentHwnd, WS_CHILD | WS_VISIBLE | 
    TBSTYLE_FLAT | CCS_NORESIZE);
  addBitmap(4, IDB_TOOLBAR);

  // Create the control buttons
  addButton(0, ID_OPTIONS);
  addButton(1, ID_INFO);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(2, ID_FULLSCREEN);
  addButton(3, ID_REQUEST_REFRESH);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(4, ID_SEND_CAD);
  addButton(5, ID_SEND_CTLESC);
  addButton(6, ID_CTRL_KEY);
  addButton(7, ID_ALT_KEY);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(8, ID_FILE_TRANSFER);
  addButton(0, 0, TBSTATE_ENABLED, TBSTYLE_SEP);
  addButton(9, ID_NEW_CONNECTION);
  addButton(10, ID_CONN_SAVE_AS);
  addButton(11, ID_CLOSE);

  // Resize the toolbar window
  autoSize();
}
