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

// -=- ToolBar control class.

#include <windows.h>
#include <commctrl.h>
#include <assert.h>

namespace rfb {

  namespace win32 {

    class ToolBar {
    public:
      ToolBar();
      virtual ~ToolBar();

      // create() creates a windows toolbar. dwStyle is a combination of 
      // the toolbar control and button styles. It returns TRUE if successful,
      // or FALSE otherwise.
      bool create(int tbID, HWND parentHwnd, 
                  DWORD dwStyle = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT);

      // -=- Button images operations

      // addBitmap() adds one or more images from resources to
      // the list of button images available for a toolbar.
      // Returns the index of the first new image if successful,
      // or -1 otherwise.
      int addBitmap(int nButtons, UINT bitmapID);

      // addSystemBitmap() adds the system-defined button bitmaps to the list
      // of the toolbar button specifying by stdBitmapID. Returns the index of 
      // the first new image if successful, or -1 otherwise.
      int addSystemBitmap(UINT stdBitmapID);

      // setBitmapSize() sets the size of the bitmapped images to be added
      // to a toolbar. It returns TRUE if successful, or FALSE otherwise.
      // You must call it before addBitmap().
      bool setBitmapSize(int width, int height);

      // -=- Button operations

      // addButton() adds one button.
      bool addButton(int iBitmap, int idCommand, BYTE state=TBSTATE_ENABLED, 
                     BYTE style=TBSTYLE_BUTTON,  UINT dwData=0, int iString=0);

      // addNButton() adds nButtons buttons to a toolbar.
      bool addNButton(int nButtons, LPTBBUTTON tbb);

      // deleteButton() removes a button from the toolbar.
      bool deleteButton(int nIndex);

      // insertButton() inserts a button in a toolbar control by index.
      bool insertButton(int nIndex, LPTBBUTTON tbb);

      // getButtonInfo() retrieves extended information about a toolbar's 
      // button. It returns index of the button if successful, or -1 otherwise.
      int getButtonInfo(int idButton, TBBUTTONINFO *btnInfo);

      // getButtonsHeight() retrieves the height of the toolbar buttons.
      int getButtonsHeight();

      // getButtonsWidth() retrieves the width of the toolbar buttons.
      int getButtonsWidth();

      // setButtonInfo() sets the information for an existing button 
      // in a toolbar.
      bool setButtonInfo(int idButton, TBBUTTONINFO* ptbbi);

      // checkButton() checks or unchecks a given button in a toolbar control.
      bool checkButton(int idButton, bool check);

      // enableButton() enables or disables the specified button 
      // in the toolbar.
      bool enableButton(int idButton, bool enable);

      // pressButton() presses or releases the specified button in the toolbar.
      bool pressButton(int idButton, bool press);

      // getButtonRect() gets the bounding rectangle of a button in a toolbar.
      bool getButtonRect(int nIndex, LPRECT buttonRect);
  
      // setButtonSize() sets the size of the buttons to be added to a toolbar.
      // Button size must be largen the button bitmap.
      bool setButtonSize(int width, int height);

      // -=- ToolBar operations

      // autoSize() resizes the toolbar window.
      void autoSize();

      // getHandle() returns handle to a toolbar window.
      HWND getHandle() { return hwndToolBar; }

      // getHeight() returns the toolbar window height.
      int getHeight();

      // getTotalWidth() returns the total size of all buttons and 
      // separators in the toolbar.
      int getTotalWidth();

      // show() displays the toolbar window.
      void show();

      // hide() hides the toolbar window.
      void hide();

      // isVisible() check the toolbar window on visible.
      bool isVisible();

    protected:
      HWND hwndToolBar;
      HWND parentHwnd;
      int tbID;
    };

  }; // win32

}; // rfb
