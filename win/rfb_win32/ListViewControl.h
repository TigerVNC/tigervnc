// ListViewControl.h: interface for the ListViewControl class.
//
//////////////////////////////////////////////////////////////////////

#ifndef AFX_LISTVIEWCONTROL_H__
#define AFX_LISTVIEWCONTROL_H__

#include <windows.h>
#include "commctrl.h"

namespace rfb {
  
  namespace win32 {
    class ListViewControl  
    {
    public:
      ListViewControl();
      bool IsSelectedLVItem(DWORD idListView, HWND hDlg, int numberItem);
      void SelectLVItem(DWORD idListView, HWND hDlg, int numberItem);
      BOOL InitLVColumns(DWORD idListView, HWND hDlg, int width, int columns,
        char * title[], DWORD mask, DWORD style, DWORD format);
      BOOL InsertLVItem(DWORD idListView, HWND hDlg, int number,  char * texts[],
        int columns);
      void SetLVItemText(DWORD idListView, HWND hDlg, int numberItem,
        int namberColumn, char * text);
      void GetLVItemText(DWORD idListView, HWND hDlg, int numberItem,
        int namberColumn, char * text);
      void DeleteLVItem(DWORD idListView, HWND hDlg, int number);
      void DeleteAllLVItem(DWORD idListView, HWND hDlg);
      virtual ~ListViewControl();	
    };
  };
};

#endif
