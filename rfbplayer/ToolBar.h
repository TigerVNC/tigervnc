/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- ToolBar.h

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

class ToolBar {
public:
  ToolBar();
  virtual ~ToolBar();

  // create() creates a windows toolbar. dwStyle is a combination of 
  // the toolbar control and button styles. It returns TRUE if successful,
  // or FALSE otherwise.
  bool create(int tbID, HWND parentHwnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT);

  // -=- Button images operations

  // addBitmap() adds one or more images from resources to the list of button
  // images available for a toolbar. Returns the index of the first new image 
  // if successful, or -1 otherwise.
  int addBitmap(int nButtons, UINT bitmapID);

  // addSystemBitmap() adds the system-defined button bitmaps to the list
  // of the toolbar button specifying by stdBitmapID. Returns the index of 
  // the first new image if successful, or -1 otherwise.
  int addSystemBitmap(UINT stdBitmapID);

  // setBitmapSize() sets the size of the bitmapped images to be added
  // to a toolbar. It returns TRUE if successful, or FALSE otherwise.
  // You must call it before addBitmap().
  bool setBitmapSize(int width, int height);

  // getHandle() returns handle to a toolbar window.
  HWND getHandle() { return hwndToolBar; }

protected:
  HWND hwndToolBar;
  int tbID;
};
