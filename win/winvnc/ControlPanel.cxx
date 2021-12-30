// ControlPanel.cxx: implementation of the ControlPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ControlPanel.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

using namespace winvnc;

bool ControlPanel::showDialog()
{
  return Dialog::showDialog(MAKEINTRESOURCE(IDD_CONTROL_PANEL), NULL);
}

void ControlPanel::initDialog()
{
  TCHAR *ColumnsStrings[] = {
    (TCHAR *) "IP address",
    (TCHAR *) "Status"
  };
  InitLVColumns(IDC_LIST_CONNECTIONS, handle, 120, 2, ColumnsStrings,
                LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
                LVS_EX_FULLROWSELECT, LVCFMT_LEFT);
  SendCommand(4, -1);
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
  case IDC_KILL_ALL:
    {
      SendCommand(2, -1);
      return false;
    }
  case IDC_KILL_SEL_CLIENT:
    {     
      SendCommand(3, 3);
      return false;
    }
  case IDC_VIEW_ONLY:
    {     
      SendCommand(3, 1);
      return false;
    }
  case IDC_FULL_CONTROL:
    {     
      SendCommand(3, 0);
      return false;
    }
  case IDC_STOP_UPDATE:
    {     
      stop_updating = true;
      EndDialog(handle, 0);
      return false;
    }
  case IDC_DISABLE_CLIENTS:
    {   
      ListConnStatus.setDisable(isItemChecked(IDC_DISABLE_CLIENTS));
      SendCommand(3, -1);
      return false;
    }
  }
  return false;
  
}

void ControlPanel::UpdateListView(ListConnInfo* LCInfo)
{
  getSelConnInfo();
  DeleteAllLVItem(IDC_LIST_CONNECTIONS, handle);
  setItemChecked(IDC_DISABLE_CLIENTS, LCInfo->getDisable());

  if(LCInfo->Empty()) 
    return;

  ListConn.Copy(LCInfo);

  char* ItemString[2];
  int i = 0;

  for (ListConn.iBegin(); !ListConn.iEnd(); ListConn.iNext()) {
    ListConn.iGetCharInfo(ItemString);
    InsertLVItem(IDC_LIST_CONNECTIONS, handle, i, (TCHAR **) ItemString, 2);
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
  case WM_DESTROY:
    if (stop_updating) {
      stop_updating = false;
      SendCommand(3, 2);
    }
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

void ControlPanel::SendCommand(DWORD command, int data)
{
  COPYDATASTRUCT copyData;
  copyData.dwData = command;
  copyData.cbData = 0;
  copyData.lpData = 0;
  getSelConnInfo();
  if (data != -1) {
    ListConnStatus.Copy(&ListSelConn);
    ListConnStatus.setAllStatus(data);
    ListConnStatus.setDisable(isItemChecked(IDC_DISABLE_CLIENTS));
  } else {
    ListConnStatus.Clear();
  }
  SendMessage(m_hSTIcon, WM_COPYDATA, 0, (LPARAM)&copyData);
}

ControlPanel::~ControlPanel()
{
  
}
