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

// -=- CommonControls.h

#include <windows.h>
#include <commctrl.h>

#include <list>

using namespace std;

// Implementation of ToolBar control class.

class ToolBar {

  // -=- ToolBar construction
  public:

    ToolBar();
    virtual bool create(int tbID, HWND parentHwnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT);
    virtual bool createEx(int tbID, HWND parentHwnd, DWORD dwExStyle = 0, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_FLAT);

  // -=- ToolBar operations
  public:

    // addBitmap() adds one or more images to the list of button images
    // available for a toolbar. Returns the index of the first new image 
    // if successful, or -1 otherwise. nButtons is number of button images 
    // in the bitmap. If nButtons > 0 then it loads bitmap from resource 
    // by resID. If nButtons = -1 then it adds system-defined button bitmap
    // which value specifying in bitmapID.
    int addBitmap(int nButtons, UINT bitmapID);

    // addButton() adds one button.
    bool addButton(int iBitmap, int idCommand, BYTE state=TBSTATE_ENABLED, BYTE style=TBSTYLE_BUTTON, UINT dwData=0, int iString=0);

    // addNButton() adds nButtons buttons to a toolbar.
    // Returns TRUE if successful, or FALSE otherwise.
    bool addNButton(int nButtons, LPTBBUTTON tbb);

    // addString() adds a string from resource, to the toolbar's internal 
    // list of strings. Returns the index of the first new string if successful, 
    // or -1 otherwise.
    int addString(UINT strID);

    // addString() is adds a string or strings to the list of strings 
    // available for a toolbar. lpStrings - address of a buffer that contains 
    // null-terminated strings to add to the toolbar's string list. The last 
    // string must be terminated with two null characters. Returns the index 
    // of the first new string if successful, or -1 otherwise.
    int addStrings(LPCTSTR lpStrings);

    // autoSize() resizes the entire toolbar control.
    void autoSize();

    // checkButton() checks or unchecks a given button in a toolbar control.
    bool checkButton(int idButton, bool check);

    // commandToIndex retrieves the zero-based index for the button associated
    // with the specified command identifier.
    UINT commandToIndex(int idButton);

    // customize() displays the customize ToolBar dialog box. The ToolBar 
    // object must have been created with the CCS_ADJUSTABLE style.
    void customize();

    // deleteButton() removes a button from the toolbar.
    bool deleteButton(int nIndex);

    // enableButton() enables or disables the specified button in the toolbar.
    bool enableButton(int idButton, bool enable);

    // getButtonInfo() retrieves extended information about a toolbar's button.
    // It returns index of the button if successful, or -1 otherwise.
    int getButtonInfo(int idButton, TBBUTTONINFO *btnInfo);

    // getHeight() retrieves ToolBar's window height.
    int getHeight();

    // getWidth() retrieves ToolBar's window width.
    int getWidth();

    // hideButton() hides or shows the specified button in the toolbar.
    bool hideButton(int idButton, bool hide);

    // indeterminateButton() sets or clears the indeterminate state of 
    // the specified button in the toolbar.
    bool indeterminateButton(int idButton, bool indeterminate);

    // insertButton() inserts a button in a toolbar control by index.
    bool insertButton(int nIndex, TBBUTTON tbb);

    // loadImages() loads bitmap into a toolbar control's image list from 
    // resources by setting bitmapID to the bitmap resource ID. It's also can 
    // loads system-defined bitmap by setting hinst to 0 and bitmapID to ID of 
    // a system-defined button image list.
    void loadImages(int bitmapID, HINSTANCE hinst=0);

    // markButton() sets the highlight state of a given button in the toolbar.
    bool markButton(int idButton, bool fHighlight);

    // pressButton() presses or releases the specified button in the toolbar.
    bool pressButton(int idButton, bool press);

    // restoreState() restores the state of the toolbar from the location 
    // in the registry specified by the parameters.
    void restoreState(HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValueName);

    // saveState() saves the state of the toolbar to the location 
    // in the registry specified by the parameters.
    void saveState(HKEY hKeyRoot, LPCTSTR lpszSubKey, LPCTSTR lpszValueName);

    // setButtonInfo() sets the information for an existing button in a toolbar.
    bool setButtonInfo(int idButton, TBBUTTONINFO* ptbbi);

    // setDrawTextFlags() sets the flags in Win32 function DrawText, which is 
    // used to draw the text in the specified rectangle. dwMask is a 
    // combination of DT_ flags, that indicates which bits in dwDTFlags will 
    // be used when drawing the text. dwDTFlags is a combination DT_ flags, 
    // that indicate how the button text will be drawn.
    DWORD setDrawTextFlags(DWORD dwMask, DWORD dwDTFlags);

    // setSize() sets ToolBar's window size.
    void setSize(int width, int height);

  // -=- ToolBar attributes
  public:
    
    // getAnchorHighlight() retrieves thr anchor highlight setting for toolbar.
    bool getAnchorHighlight();

    // getBitmap() gets the index of the bitmap associated with a button.
    int getBitmap(int idButton);

    // getBitmapFlags() retrieves the flags that describe the type of bitmap 
    // to be used. If it returns value has the TBBF_LARGE flag set, application
    // should use large bitmaps(24 x 24); otherwise, use small bitmaps(16 x 16).
    DWORD getBitmapFlags();

    // getButton() gets information about the specified toolbar's button.
    bool getButton(int nIndex, LPTBBUTTON lpButton);

    // getButtonCount() retrieves a count of the buttons in the toolbar.
    int getButtonCount();

    // getButtonHeight() gets toolbar's button height.
    int getButtonHeight();
    
    // getButtonWidth() gets toolbar's button width.
    int getButtonWidth();

    // getDisabledImageList() gets the image list for inactive buttons.
    HIMAGELIST getDisabledImageList();

    // getDropTarget() retrieves the IDropTarget interface for the toolbar.
    // ppDropTarget is a pointer to IDropTarget interface pointer. If an error 
    // occurs, a NULL pointer is placed in this address.
    HRESULT getDropTarget(IDropTarget** ppDropTarget);
    
    // getExtendedStyle() retrieves the extended styles for a toolbar control.
    DWORD getExtendedStyle();

    // getHandle() returns handle to a toolbar window.
    HWND getHandle() { return hwndToolBar; }

    // getToolBarId() returns command-identifier of a ToolBar control.
    int getToolBarId() { return tbID; }

    // getHotImageList() retrieves the image list that a toolbar uses to display 
    // hot buttons. Toolbar must have the TBSTYLE_FLAT or TBSTYLE_LIST style to 
    // have hot items. It returns NULL if no hot image list is set.
    HIMAGELIST getHotImageList();

    // getHotItem() retrieves the index of the hot item in a toolbar.
    int getHotItem();

    // getImageList() gets the image list that a toolbar control uses to display 
    // buttons in their default state.
    HIMAGELIST getImageList();

    // getInsertMark() retrieves the current insertion mark for the toolbar.
    void getInsertMark(TBINSERTMARK* ptbim);

    // getInsertMarkColor() gets the color used to draw the insertion mark.
    COLORREF getInsertMarkColor();

    // getItemRect() gets the bounding rectangle of a button in the toolbar.
    bool getItemRect(int nIndex, LPRECT lpRect);

    // getMaxSize() retrieves the total size of all of the visible buttons 
    // and separators in the toolbar.
    bool getMaxSize(LPSIZE pSize);

    // getMaxTextRows() gets the maximum number of text rows displayed 
    // on a toolbar button.
    int getMaxTextRows();

    // getRect() gets the bounding rectangle for a specified toolbar button.
    bool getRect(int idButton, LPRECT lpRect);

    // getRows() returns the number of rows of buttons in a toolbar 
    // with the TBSTYLE_WRAPABLE style.
    int getRows();

    // getButtonState() retrieves information about the state of the specified 
    // button in a toolbar, such as whether it is enabled, pressed, or checked.
    int getButtonState(int idButton);

    // getStyle() gets the styles currently applied to a toolbar control.
    DWORD getStyle();

    // getToolTips() retrieves the handle of the tool tip control, 
    // if any, associated with the toolbar control.
    HWND getToolTips();

    // hitTest() determines where a point lies in a toolbar control. If it 
    // returns positive value, this return index of the nonseparator item in 
    // which the point lies. Otherwise, it returns negative value.
    int hitTest(LPPOINT pptHitTest);

    // insertMarkHitTest() retrieves the insertion mark information for a point 
    // in a toolbar. Returns TRUE if the point is an insertion mark.
    bool insertMarkHitTest(LPPOINT ppt, LPTBINSERTMARK ptbim);

    // isButtonChecked() determines whether the button in a toolbar is checked.
    bool isButtonChecked(int idButton);

    // isButtonEnabled() determines whether the button in a toolbar is enabled.
    bool isButtonEnabled(int idButton);

    // isButtonHidded() determines whether the button in a toolbar is hidden.
    bool isButtonHidden(int idButton);

    // isButtonHighlighted() checks the highlight state of a toolbar button.
    bool isButtonHighlighted(int idButton);

    // isButtonIndeterminate() determines whether the specified button in a toolbar
    // control is indeterminate.
    bool isButtonIndeterminate(int idButton);
   
    // isButtonPressed() checks whether the button in the toolbar is pressed.
    bool isButtonPressed(int idButton);

    // mapAccelerator() determines the ID of the button that corresponds to 
    // the specified accelerator character.
    bool mapAccelerator(TCHAR chAccel, UINT* pIDBtn);

    // moveButton() moves a button from one index to another.
    bool moveButton(UINT oldPos, UINT newPos);

    // setAnchorHighlight() sets the anchor highlight setting for a toolbar.
    bool setAnchorHighlight(bool anchor);

    // setButtonId() sets the command identifier for the specified button.
    bool setButtonId(int nIndex, UINT idButton);
    
    // setBitmapSize() sets the size of the actual bitmapped images to be added 
    // to a toolbar control.
    bool setBitmapSize(int widht, int height);

    // setButtonSize() sets the size of the buttons to be added to a toolbar.
    // Button size must be largen the button bitmap.
    bool setButtonSize(int width, int height);

    // setButtonWidth() sets the min and max button widths in the toolbar.
    bool setButtonWidth(int cxMin, int cxMax);

    // setDisabledImageList() sets the image list that the toolbar will use to 
    // display disabled buttons. It returns handle to the image list previously
    // used, or NULL if no image list was previously set.
    HIMAGELIST setDisabledImageList(HIMAGELIST pImageList);

    // setExtendedStyle() sets the extended styles for a toolbar control.
    // dwExStyle is a combination of the toolbar extended styles. This function
    // returns the previous extended styles.
    DWORD setExtendedStyle(DWORD tbExStyle);

    // setHotImageList() sets the image list that the toolbar will use 
    // to display "hot" buttons. It returns handle to the image list previously
    // used, or NULL if no image list was previously set.
    HIMAGELIST setHotImageList(HIMAGELIST pImageList);

    // setHotItem() sets the hot item in a toolbar. If nIndex sets to -1, 
    // none of the items will be hot. It returns index of the previous hot
    // item, or -1 if there was no hot item.
    int setHotItem(int nIndex);

    // setImageList() sets the image list that the toolbar will use to display 
    // buttons that are in their default state. It returns handle to the image 
    // list previously used, or NULL if no image list was previously set.
    // DON'T combine this function with addBitmap()
    HIMAGELIST setImageList(HIMAGELIST pImageList);

    // setIndent() sets the indentation for the first button in a toolbar.
    bool setIndent(int nPixel);
   
    // setInsertMark() sets the current insertion mark for the toolbar.
    void setInsertMark(TBINSERTMARK* ptbim);

    // setInsertMarkColor() sets the color used to draw the insertion mark 
    // for the toolbar. It returns the previous insertion mark color.
    COLORREF setInsertMarkColor(COLORREF clrNew);

    // setMaxTextRows() sets the maximum number of text rows displayed 
    // on a toolbar button.
    bool setMaxTextRows(int iMaxRows);

    // setOwner() sets the window to which the toolbar control sends 
    // notification messages. It returns a handle to the previous 
    // notification window, or NULL if there is no previous notification window.
    HWND setOwner(HWND owner);

    // setRows() sets the number of rows of buttons in a toolbar. fLarger tells
    // whether to use more rows or fewer rows if the toolbar cannot be resized 
    // to the requested number of rows. lpRect is a pointer to RECT that 
    // receives the bounding rectangle of the toolbar after the rows are set.
    void setRows(int nRows, bool fLarger, LPRECT lpRect);

    // setButtonState() sets the state for the specified button in a toolbar.
    bool setButtonState(int idButton, int btnState);

    // setStyle() sets the styles for a toolbar control.
    // dwStyle is a combination of toolbar control styles.
    void setStyle(DWORD tbStyle);

    // setToolTips() associates a ToolTip control with a toolbar.
    void setToolTips(HWND hToolTip);

    // show() shows Tolbar's window.
    int show() { return ShowWindow(getHandle(), SW_SHOW); };
    
    // hide() hides Tolbar's window.
    int hide() { return ShowWindow(getHandle(), SW_HIDE); };

  // -=- ToolBar implementation
  public:
    LRESULT processNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);
    virtual ~ToolBar();

  private:

     // setButtonStructSize() specifies the size of the TBBUTTON structure.
    void setButtonStructSize(int nBytes);

  protected:
    HWND hwndToolBar;
    int tbID;

    list <TBBUTTON> tbButtons;
};
