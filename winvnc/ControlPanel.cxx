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
      m_server->disconnectClients();
      return false;
    }
  case IDC_DISABLE_CLIENTS:
    {     
      
      return false;
    }
  }
  return false;
  
}

void ControlPanel::UpdateListView()
{
  
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

void ControlPanel::getSelectedConn(std::list<network::Socket*>* selsockets)
{
  
}

ControlPanel::~ControlPanel()
{
  
}
