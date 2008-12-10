// ListViewControl.cxx: implementation of the ListViewControl class.
//
//////////////////////////////////////////////////////////////////////
#include <tchar.h>
#include "ListViewControl.h"
#include "commctrl.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
using namespace rfb;
using namespace rfb::win32;

ListViewControl::ListViewControl()
{
}

bool ListViewControl::IsSelectedLVItem(DWORD idListView,
                                      HWND hDlg, int numberItem)
{	
  return (ListView_GetItemState(GetDlgItem(hDlg, idListView),
    numberItem, LVIS_SELECTED) == LVIS_SELECTED);
}

void ListViewControl::SelectLVItem(DWORD idListView, HWND hDlg, int numberItem)
{
  ListView_SetItemState(GetDlgItem(hDlg, idListView),
    numberItem, LVIS_SELECTED, LVIS_SELECTED);
}

BOOL ListViewControl::InitLVColumns(DWORD idListView, HWND hDlg, int width, int columns,
                                    TCHAR *title[], DWORD mask, DWORD LVStyle, DWORD format)
{
  ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, idListView), LVStyle);
  TCHAR szText[256];      
  LVCOLUMN lvc; 
  int iCol;
  
  lvc.mask = mask; 
  
  for (iCol = 0; iCol < columns; iCol++) { 
    lvc.iSubItem = iCol;
    lvc.pszText = szText;	
    lvc.cx = width;           
    lvc.fmt = format;
    
    _tcscpy(szText, title[iCol]); 
    if (ListView_InsertColumn(GetDlgItem(hDlg, idListView), iCol, &lvc) == -1) 
      return FALSE; 
  } 
  return TRUE; 
}

BOOL ListViewControl::InsertLVItem(DWORD idListView, HWND hDlg, int number,  TCHAR * texts[],
                                   int columns)
{
  int i;
  LVITEM lvI;
  lvI.mask = LVIF_TEXT| LVIF_STATE; 
  lvI.state = 0; 
  lvI.stateMask = 0; 
  lvI.iItem = number; 
  lvI.iSubItem = 0; 
  lvI.pszText = texts[0]; 									  
  
  if(ListView_InsertItem(GetDlgItem(hDlg, idListView), &lvI) == -1)
    return FALSE;
  
  for (i =1; i < columns; i++) {	
    SetLVItemText(
      idListView, hDlg, 
      number, i, texts[i]);
  }
  return TRUE;
}

void ListViewControl::SetLVItemText(DWORD idListView, HWND hDlg, int numberItem,
                                    int namberColumn, TCHAR * text)
{
  ListView_SetItemText(
    GetDlgItem(hDlg, idListView), 
    numberItem, namberColumn, text);
}

void ListViewControl::GetLVItemText(DWORD idListView, HWND hDlg, int numberItem,
                                    int namberColumn, TCHAR * text)
{
	 ListView_GetItemText(GetDlgItem(hDlg, idListView), numberItem,
							namberColumn, text, 256);
}

void ListViewControl::DeleteLVItem(DWORD idListView, HWND hDlg, int number)
{
  ListView_DeleteItem(GetDlgItem(hDlg, idListView), number);
}

void ListViewControl::DeleteAllLVItem(DWORD idListView, HWND hDlg)
{
  ListView_DeleteAllItems(GetDlgItem(hDlg, idListView));
}

ListViewControl::~ListViewControl()
{
}
