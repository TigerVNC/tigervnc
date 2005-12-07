// ControlPanel.cxx: implementation of the ControlPanel class.
//
//////////////////////////////////////////////////////////////////////

#include "ControlPanel.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//using namespace rfb_win32;
using namespace winvnc;

bool ControlPanel::showDialog()
{
  return Dialog::showDialog(MAKEINTRESOURCE(IDD_CONTROL_PANEL), NULL);
}

void ControlPanel::initDialog()
{
  TCHAR *ColumnsStrings[] = {
    "IP address",
    "Time connected",
    "Status"
  };
  InitLVColumns(IDC_LIST_CONNECTIONS, handle, 120, 3, ColumnsStrings,
                LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
                LVS_EX_FULLROWSELECT, LVCFMT_LEFT);
}

bool ControlPanel::onCommand(int cmd)
{
  switch (cmd) {
  case IDC_PROPERTIES:
    SendMessage(m_hSTIcon, WM_COMMAND, ID_OPTIONS, 0);
    return false;
  case IDC_ADD_CLIENT:
    SendMessage(m_hSTIcon, WM_COMMAND, ID_CONNECT, 0);
    return false;
  case IDC_KILL_SEL_CLIENT:
    {
      
      return false;
    }
  case IDC_KILL_ALL:
    {
      COPYDATASTRUCT copyData;
      copyData.dwData = 2;
      copyData.lpData = 0;
      copyData.cbData = 0;
      SendMessage(m_hSTIcon, WM_COPYDATA, 0, (LPARAM)&copyData);
      return false;
    }
  case IDC_DISABLE_CLIENTS:
    {     
      
      return false;
    }
  }
  return false;
  
}

void ControlPanel::UpdateListView(rfb::ListConnInfo* LCInfo)
{
  getSelConnInfo();
  DeleteAllLVItem(IDC_LIST_CONNECTIONS, handle);

  if(LCInfo->Empty()) 
    return;

  ListConn.Copy(LCInfo);

  char* ItemString[3];
  int i = 0;

  for (ListConn.iBegin(); !ListConn.iEnd(); ListConn.iNext()) {
    ListConn.iGetCharInfo(ItemString);
    InsertLVItem(IDC_LIST_CONNECTIONS, handle, i, ItemString, 3);
    for (ListSelConn.iBegin(); !ListSelConn.iEnd(); ListSelConn.iNext()) {
      if (ListSelConn.iGetConn() == ListConn.iGetConn())
        SelectLVItem(IDC_LIST_CONNECTIONS, handle, i);
    }
    i++;
  } 
}

BOOL ControlPanel::dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_INITDIALOG:
    handle = hwnd;
    initDialog();
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDCANCEL:
      handle = NULL;
      EndDialog(hwnd, 0);
      return TRUE;
    default:
      return onCommand(LOWORD(wParam));
    }
  }
  return FALSE;
}

void ControlPanel::getSelConnInfo()
{
  int i = 0;
  ListSelConn.Clear();
  if(ListConn.Empty()) return;
  for (ListConn.iBegin(); !ListConn.iEnd(); ListConn.iNext()) {
    if (IsSelectedLVItem(IDC_LIST_CONNECTIONS, handle, i))
      ListSelConn.iAdd(&ListConn);
    i++;
  }
}

ControlPanel::~ControlPanel()
{
  
}
