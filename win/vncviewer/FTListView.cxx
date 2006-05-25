/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- FTListView.cxx

#include <vncviewer/FTListView.h>

using namespace rfb;
using namespace rfb::win32;

FTListView::FTListView(HWND hListView)
{
  m_bInitialized = false;
  m_hListView = hListView;
  m_fileInfo.free();
}

FTListView::~FTListView()
{
  m_fileInfo.free();
}


bool 
FTListView::initialize(HINSTANCE hInst)
{
  if (m_bInitialized) return false;

  initImageList(hInst);
  
  RECT Rect;
  GetClientRect(m_hListView, &Rect);
  Rect.right -= GetSystemMetrics(SM_CXHSCROLL);
  int xwidth0 = (int) (0.35 * Rect.right);
  int xwidth1 = (int) (0.22 * Rect.right);
  int xwidth2 = (int) (0.43 * Rect.right);
  
  addColumn("Name", 0, xwidth0, LVCFMT_LEFT);
  addColumn("Size", 1, xwidth1, LVCFMT_RIGHT);
  addColumn("Data", 2, xwidth2, LVCFMT_LEFT);

  ListView_SetExtendedListViewStyleEx(m_hListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
 
  return true;
}

void 
FTListView::onGetDispInfo(NMLVDISPINFO *pDI)
{
  if (m_fileInfo.getFlagsAt(pDI->item.iItem) & FT_ATTR_DIR) {
    pDI->item.iImage = 0;
  } else {
    pDI->item.iImage = 1;
  }
  
  switch (pDI->item.iSubItem)
  {
  case 0:
    pDI->item.pszText = m_fileInfo.getNameAt(pDI->item.iItem);
    break;
  case 1:
    {
      unsigned int flags = m_fileInfo.getFlagsAt(pDI->item.iItem);
      switch(flags & 0x000000FF)
      {
      case FT_ATTR_FILE:
        {
          char buf[32];
          unsigned int size = m_fileInfo.getSizeAt(pDI->item.iItem);
          sprintf(buf, "%u", size);
          pDI->item.pszText = buf;
        }
        break;
      case FT_ATTR_DIR:
        pDI->item.pszText = "";
        break;
      default:
        pDI->item.pszText = "Unspecified";
      }
    }
    break;
  case 2:
    {
      unsigned int data = m_fileInfo.getDataAt(pDI->item.iItem);
      if (data == 0) {
        pDI->item.pszText = "Unspecified";
      } else {
        FILETIME ft;
        FolderManager fm;
        fm.getFiletime(data, &ft);
        
        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        
        char pDateTimeStr[1024];
        char timeFmt[128];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, timeFmt, 128);
        char dateFmt[128];
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, dateFmt, 128);
        
        char timeStr[128];
        GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, timeFmt, timeStr, 128);
        char dateStr[128];
        GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, dateFmt, dateStr, 128);
        
        sprintf(pDateTimeStr, "%s %s", dateStr, timeStr);
        pDI->item.pszText = pDateTimeStr;
      }
    }
    break;
  default:
    break;
  }
}

void 
FTListView::addItems(FileInfo *pFI)
{
  m_fileInfo.add(pFI);
  LVITEM LVItem;
  LVItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE; 
  LVItem.state = 0; 
  LVItem.stateMask = 0; 
  for (unsigned int i = 0; i < m_fileInfo.getNumEntries(); i++) {
    LVItem.iItem = i;
    LVItem.iSubItem = 0;
    LVItem.iImage = I_IMAGECALLBACK;
    LVItem.pszText = LPSTR_TEXTCALLBACK;
    ListView_InsertItem(m_hListView, &LVItem);
  }
}

void 
FTListView::deleteAllItems()
{
  ListView_DeleteAllItems(m_hListView);
  m_fileInfo.free();
}

char *
FTListView::getActivateItemName(LPNMITEMACTIVATE lpnmia)
{
  return m_fileInfo.getNameAt(lpnmia->iItem);
}

int 
FTListView::getSelectedItems(FileInfo *pFI)
{
  int selCount = ListView_GetSelectedCount(m_hListView);
  int selItem = ListView_GetSelectionMark(m_hListView);
  if ((selCount < 1) || (selItem < 0)) return -1;
  
  selItem = -1;
  selItem = ListView_GetNextItem(m_hListView, selItem, LVNI_SELECTED);
  do {
    pFI->add(m_fileInfo.getFullDataAt(selItem));
    selItem = ListView_GetNextItem(m_hListView, selItem, LVNI_SELECTED);
  } while (selItem >= 0);
  
  return selCount;
}

void
FTListView::initImageList(HINSTANCE hInst)
{
  m_hImageList = ImageList_Create(16, 16, ILC_MASK, 2, 2); 
  
  HICON hiconItem = LoadIcon(hInst, MAKEINTRESOURCE(IDI_FTDIR)); 
  ImageList_AddIcon(m_hImageList, hiconItem);  
  DestroyIcon(hiconItem); 
  
  hiconItem = LoadIcon(hInst, MAKEINTRESOURCE(IDI_FTFILE)); 
  ImageList_AddIcon(m_hImageList, hiconItem); 
  DestroyIcon(hiconItem); 
  
  ListView_SetImageList(m_hListView, m_hImageList, LVSIL_SMALL); 
}

void 
FTListView::addColumn(char *iText, int iOrder, int xWidth, int alignFmt)
{
	LVCOLUMN lvc; 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
	lvc.fmt = alignFmt;
	lvc.iSubItem = iOrder;
	lvc.pszText = iText;	
	lvc.cchTextMax = 32;
	lvc.cx = xWidth;
	lvc.iOrder = iOrder;
	ListView_InsertColumn(m_hListView, iOrder, &lvc);
}
